#include <Wire.h>
#include <SPI.h>
#include "Config.h"
#include "Mcp.h"
#include "EzoBoard.h"
#include "Lcd.h"
#include "Sd.h"
#include "Timer.h"
#include "ActionTimer.h"
#include "Menu.h"

#define UTC_OFFSET (-7 * 3600)

ConfigState config;

EzoBoard orpSensors[3] = {  //set up ORP sensors (3 probes)//
  EzoBoard(EzoAddresses::ORP[0], "ORP"),
  EzoBoard(EzoAddresses::ORP[1], "ORP"),
  EzoBoard(EzoAddresses::ORP[2], "ORP")
};

EzoBoard phSensors[3] = {  //set up pH sensors (3 probes)//
  EzoBoard(EzoAddresses::PH[0], "pH"),
  EzoBoard(EzoAddresses::PH[1], "pH"),
  EzoBoard(EzoAddresses::PH[2], "pH")
};

Mcp mcp;  //set up MCP (Multi-Channel Protocol) controller (used to control the valves)//
Lcd lcd(0x27, &Wire1);  //set up LCD display (I2C address 0x27, using Wire1 for I2C communication) (used to display the data)//
SdLogger sd;  //set up SD card logger//
Menu menu(&config, phSensors, orpSensors, lcd);  //set up menu (configuration, pH sensors, ORP sensors, LCD display)//

Timer probeTimer(TimingIntervals::PROBE_READ_INTERVAL);  //set up probe read timer (*2 seconds)//
Timer sdLogTimer(TimingIntervals::SD_LOG_INTERVAL);  //set up SD log timer (*2 seconds)//

bool queued[ValveMappings::TOTAL_VALVES] = { false };  //queued[i] - "valve i is already in the MCP queue waiting to open." Prevents double-queueing.//

ActionTimer cooldown[ValveMappings::TOTAL_VALVES] = {
  ActionTimer(TimingIntervals::VALVE_COOLDOWN),
  ActionTimer(TimingIntervals::VALVE_COOLDOWN),
  ActionTimer(TimingIntervals::VALVE_COOLDOWN),
  ActionTimer(TimingIntervals::VALVE_COOLDOWN),
  ActionTimer(TimingIntervals::VALVE_COOLDOWN),
  ActionTimer(TimingIntervals::VALVE_COOLDOWN)
};  //One 60 s cooldown timer per valve; started when a valve is queued.//
//History of open times for error-locking (too many opens in *1 hour).//
unsigned long valveTriggerTimes[ValveMappings::TOTAL_VALVES][TimingIntervals::MAX_VALVE_ERROR_HISTORY] = {};  //Timestamp of each valve trigger in the last *1 hour.//
int valveTriggerCounts[ValveMappings::TOTAL_VALVES] = {};  //Count of valve triggers in the last *1 hour.//

bool tryQueueValve(int valveId, bool condition)
{
  if (!condition) return false;  //If the sensor/threshold test is false, exit — do nothing.//
  if (config.valvesDisabled) return false;  //If the user turned valves off in the menu, exit.//
  if (queued[valveId]) return false;  //If this valve is already waiting in the queue, exit (no duplicate).//
  if (config.valveErrorLocked[valveId]) return false;  //If this valve was error-locked (too many triggers), exit.//
  if (!cooldown[valveId].done()) return false;  //If the 60 s cooldown from the last queue is still running, exit.//
  if (!mcp.enqueue(valveId)) return false;  //Push valveId onto the MCP’s FIFO queue (fails if queue already has *6 items).//

  cooldown[valveId].start();  //Start the *60 s cooldown (even though the valve may not open for up to *~12 s).//
  queued[valveId] = true;  //Mark this valve as “pending” so line 54 blocks re-queueing.//
  return true;  //Return true = successfully queued (not opened yet).//
}

void recordValveTrigger(int valveId)  //Record the valve trigger in the error-locking history.//
{
  int &count = valveTriggerCounts[valveId];  //Count of valve triggers in the last *1 hour.//

  if (count < TimingIntervals::MAX_VALVE_ERROR_HISTORY)  //If the count is less than the max error history, add the current time to the trigger times array.//
  {
    valveTriggerTimes[valveId][count++] = millis();  //Add the current time to the trigger times array.//
  }
  else  //If the count is greater than the max error history, shift the trigger times array to the left and add the current time to the trigger times array.//
  {
    for (int i = 1; i < TimingIntervals::MAX_VALVE_ERROR_HISTORY; ++i)
      valveTriggerTimes[valveId][i - 1] = valveTriggerTimes[valveId][i];   //Shift the trigger times array to the left.//

    valveTriggerTimes[valveId][TimingIntervals::MAX_VALVE_ERROR_HISTORY - 1] = millis();  //Add the current time to the trigger times array.//
  }

  const bool isPhValve = (valveId % 2 == 0);  //Check if the valve is a pH valve.//
  const int limit = isPhValve
    ? TimingIntervals::PH_VALVE_TRIGGER_LIMIT  //If the valve is a pH valve, use the pH valve trigger limit.//
    : TimingIntervals::ORP_VALVE_TRIGGER_LIMIT;  //If the valve is an ORP valve, use the ORP valve trigger limit.//

  if (count >= limit)  //If the count is greater than or equal to the limit, check if the valve has been error-locked.//
  {
    unsigned long oldest = valveTriggerTimes[valveId][count - limit];  //Get the oldest trigger time.//
    if (millis() - oldest < TimingIntervals::VALVE_ERROR_WINDOW)  //If the oldest trigger time is less than the error window, error-lock the valve.//
    {
      if (!config.valveErrorLocked[valveId])  //If the valve is not already error-locked, error-lock the valve.//
      {
        config.valveErrorLocked[valveId] = true;  //Set the valve error locked flag to true.//
        sd.logValveLocked(valveId);  //Log the valve error to the SD card.//
      }
    }
  }
}

void resetValveErrors()  //Reset the valve error-locking history.//
{
  for (int i = 0; i < ValveMappings::TOTAL_VALVES; ++i)  //Reset the valve error-locking history for all valves.//
  {
    config.valveErrorLocked[i] = false;  //Set the valve error locked flag to false.//
    valveTriggerCounts[i] = 0;  //Set the valve trigger count to 0.//
    queued[i] = false;  //Set the valve queued flag to false, so that it can be queued again.//

    for (int j = 0; j < TimingIntervals::MAX_VALVE_ERROR_HISTORY; ++j)  //Reset the trigger times array for all valves.//
      valveTriggerTimes[i][j] = 0;  //Set the trigger times array to 0.//
  }

  mcp.resetState(queued);  //Reset the MCP state.//

  config.requestResetErrors = false;  //Set the request reset errors flag to false.//
}

void handleProbeReads()  //Handle the probe reads and update the LCD.//
{
  for (int i = 0; i < 3; ++i)  //Read the pH and ORP values from all 3 probes.//
  {
    config.phValues[i] = phSensors[i].read();  //Read the pH value from the probe.//
    config.orpValues[i] = orpSensors[i].read();  //Read the ORP value from the probe.//

    config.phValid[i] = phSensors[i].hasValidReading();  //Check if the pH value is valid.//
    config.orpValid[i] = orpSensors[i].hasValidReading();  //Check if the ORP value is valid.//
  }

  if (!config.lcdCleared)  //If the LCD is not cleared, clear it.//
  {
    lcd.clear();  //Clear the LCD.//
    config.lcdCleared = true;  //Set the LCD cleared flag to true.//
  }

  if (!menu.isActive())  //If the menu is not active, print the data to the LCD.//
    lcd.printData(config);  //Print the data to the LCD.//
}

void setup()  //Setup the hardware and software.//
{
  Wire.begin();  //Initialize the I2C bus.//
  Wire.setClock(BusSpeeds::I2C_HZ);  //Set the I2C bus speed.//

  Wire1.begin(PinConfigurations::LCD_PIN_SDA, PinConfigurations::LCD_PIN_SCL, BusSpeeds::I2C_HZ);  //Initialize the LCD display (I2C address 0x27, using Wire1 for I2C communication).//

  const bool mcpReady = mcp.begin();  //Check if the MCP is ready.//

  lcd.init();  //Initialize the LCD display.//

  SPI.begin();  //Initialize the SPI bus.//

  configTime(UTC_OFFSET, 0, "");  //Set the time from the internet.//

  const bool sdReady = sd.begin(PinConfigurations::SD_CHIP_SELECT, BusSpeeds::SD_SPI_HZ);  //Check if the SD card is ready.//
  sd.setTimeFromBuild();  //Set the time from the build date.//

  if (!mcpReady || !sdReady)  //If the MCP or SD card is not ready, print an error message to the LCD.//
  {
    lcd.clear();  //Clear the LCD.//
    lcd.setCursor(COL_LEFT, ROW_TITLE);  //Set the cursor to the left column and the title row.//
    lcd.print(!mcpReady ? "MCP init failed" : "SD init failed");  //Print the MCP init failed message to the LCD.//
    lcd.setCursor(COL_LEFT, ROW_1);  //Set the cursor to the left column and the first row.//
    lcd.print(!mcpReady && !sdReady ? "SD init failed" : "Check wiring");  //Print the SD init failed message to the LCD.//
  }

  sd.logMessage("BOOT");  //Log the boot message to the SD card.//
}

void loop()  //Main loop.//
{
  if (probeTimer.isReady())
    handleProbeReads();  //Reads and validates pH/ORP data from all 3 probes, updates the LCD, and clears the LCD if needed.Every *2 seconds.//

  if (config.requestResetErrors)
    resetValveErrors();  //Clears locks, queued[], trigger history, and the MCP queue (from the menu).//

  tryQueueValve(0, config.phValid[0] && config.phValues[0] < Thresholds::PH_MINIMUM);
  tryQueueValve(2, config.phValid[1] && config.phValues[1] < Thresholds::PH_MINIMUM);
  tryQueueValve(4, config.phValid[2] && config.phValues[2] < Thresholds::PH_MINIMUM);

  tryQueueValve(1, config.orpValid[0] && config.orpValues[0] < Thresholds::ORP_MINIMUM);
  tryQueueValve(3, config.orpValid[1] && config.orpValues[1] < Thresholds::ORP_MINIMUM);
  tryQueueValve(5, config.orpValid[2] && config.orpValues[2] < Thresholds::ORP_MINIMUM);

  int openedValve = mcp.update(queued);
  if (openedValve >= 0)  //If a valve was opened, log the event and record the trigger.//
  {
    sd.logValveOn(openedValve);  //Log the valve open event to the SD card.//
    recordValveTrigger(openedValve);  //Record the valve trigger in the error-locking history.//
  }

  if (sdLogTimer.isReady())
    sd.logData(config);  //Log the current configuration data to the SD card every *10 seconds.//

  if (!menu.isActive() && menu.joystick.isPressed())
    menu.enter();  //Enter the menu if the joystick is pressed and the menu is not active.//

  if (menu.isActive())  //If the menu is active, update and draw the menu.//
  {
    menu.update();  //Update the menu.//
    menu.draw();  //Draw the menu.//
  }
}
