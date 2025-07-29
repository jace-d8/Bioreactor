#include <Wire.h>
#include "Config.h"
#include "Valve.h"
#include "EzoBoard.h"
#include "Lcd.h"
#include "Sd.h"
#include "Timer.h"

// Probes
EzoBoard orpSensor(98, "ORP");
EzoBoard phSensor(99, "PH");

// Valves
Valve phValve(PinConfigurations::PH_VALVE_PIN, 10000); // Open for 10 seconds
Valve orpValve(PinConfigurations::ORP_VALVE_PIN, 4000); // Open for 4 seconds 
// Recall the open time is counted as cooldown waiting period

// Timers
Timer probeTimer(TimingIntervals::PROBE_READ_INTERVAL);
Timer sdLogTimer(TimingIntervals::SD_LOG_INTERVAL);
Timer orpFlushTimer(TimingIntervals::HOUR_INTERVAL);
Timer phCountResetTimer(TimingIntervals::PH_RESET_WINDOW);
Timer valveCooldownTimer(TimingIntervals::VALVE_COOLDOWN);


// Single runtime config
ConfigState config;
// Consider only passing needed vars to functions instead of full struct 

void setup()
{
  Serial.begin(9600);              // Init serial comms at 9600 baud
  Wire.begin();                    // Init I2C
  Wire.setClock(400000);           // Set I2C speed to 400 kHz (Fast Mode)

  pinMode(PinConfigurations::PH_VALVE_PIN, OUTPUT);
  pinMode(PinConfigurations::ORP_VALVE_PIN, OUTPUT);
  pinMode(PinConfigurations::PIN_SW, INPUT_PULLUP);

  initLcd();
  // lcd.setCursor(COL_LEFT, ROW_1); forgot what this does 
  initSD();
  // lcd.setCursor(COL_LEFT, ROW_2); Idk why this was here

  configTime(UTC_OFFSET, 0, ""); // This should be either computer time or last counted time, whichever is "higher"
  setTimeFromBuild(); // This too 
}

void loop() // minimize delay() and cut down on non modularized logic
{ 
  // Periodic sensor reads
  if (probeTimer.isReady()) handleProbeReads(config);
  // Periodic SD logging
  if (sdLogTimer.isReady()) logToSD(config);
  // PH Valve Control
  if ((config.phValue < Thresholds::PH_MINIMUM) && valveCooldownTimer.isReady()) handlePhValve(); 
  // ORP Valve Control
  if ((config.orpValue < Thresholds::ORP_MINIMUM) && valveCooldownTimer.isReady()) orpValve.open();
  // Hourly ORP flush
  if (orpFlushTimer.isReady()) orpValve.open();

  phValve.update();
  orpValve.update();
  
  toggleMenu(config);
}

// Will stay in the .ino
void analogControl(int &selectedItem) // Consider testing if debounce is needed 
{
  int yVal = analogRead(PinConfigurations::PIN_JOYSTICK_Y);
  const int deadZone = 400;
  const int range = 1980; 

  if (yVal < range - deadZone)  // Up
  {
    if (selectedItem > MENU_PH1) selectedItem--;
    delay(200); // Debounce
  }
  else if (yVal > range + deadZone)  // Down
  {
    if (selectedItem < MENU_DONE) selectedItem++;
    delay(200); // Debounce
  }
}

// tmp... make task class? 
void handleProbeReads(ConfigState& config)
{
  config.phValue = phSensor.read();
  config.orpValue = orpSensor.read();

  // Look into fixes for this lcd.cleared check... 
  if (!config.lcdCleared)
  {
    lcd.clear();
    config.lcdCleared = true;
  }

  printData(config);
}

void handlePhValve()
{
  phValve.open();

  if (phCountResetTimer.isReady())
  {
    config.phValveActivationCount = 1;
  }
  else
  {
    config.phValveActivationCount++;
  }

  if (config.phValveActivationCount >= TimingIntervals::PH_VALVE_MAX_ACTIVATIONS)
  {
    displayWarning(config);
    config.phValveActivationCount = 0;
  }
}

void handleOrpValve()
{
  orpValve.open();
  // Warning logic here...
}
// tmp 



// This needs reworked
void isPressed(ConfigState& config, bool &calibrateMenu, int selectedItem)  
{
  if (!digitalRead(PinConfigurations::PIN_SW))
  {
    delay(200);  // debounce
    while (!digitalRead(PinConfigurations::PIN_SW));  // wait until released

    if (selectedItem == MENU_PH1)
    {
      calibrateProbePH(config);
    }
    else if (selectedItem == MENU_ORP1)
    {
      calibrateProbeORP(config);
    }
    else if (selectedItem == MENU_VALVE_TOGGLE)
    {
      config.valvesDisabled = !config.valvesDisabled;
      phValve.switchValve();
      orpValve.switchValve();
      lcd.clear();
      lcd.print(config.valvesDisabled ? "Valves off" : "Valves on");
      delay(600);
      lcd.clear();
    }
    else if (selectedItem == MENU_DONE)
    {
      calibrateMenu = false;
      lcd.clear();
    }
  }
}



// Break down and move 
void calibrateProbePH(ConfigState& config)
{
  int bufferSelection = 0;
  bool bufferCalibrated[3] = {false, false, false};
  bool selecting = true;
  lcd.clear();

  while (selecting)
  {
    analogControl(bufferSelection);
    updateGlobalBlink(config);

    lcd.setCursor(COL_LEFT, ROW_TITLE);
    lcd.print("Select Buffer:");

    if (bufferCalibrated[0]) lcd.setCursor(COL_MID, ROW_1), lcd.print("*");
    if (bufferCalibrated[1]) lcd.setCursor(COL_MID, ROW_2), lcd.print("*");
    if (bufferCalibrated[2]) lcd.setCursor(COL_MID, ROW_3), lcd.print("*");

    if (!digitalRead(PinConfigurations::PIN_SW))
    {
      while (!digitalRead(PinConfigurations::PIN_SW));

      switch (bufferSelection)
      {
        case 0:
          phSensor.sendCmd("Cal,low,4.00");
          bufferCalibrated[0] = true;
          break;
        case 1:
          phSensor.sendCmd("Cal,mid,7.00");
          bufferCalibrated[1] = true;
          break;
        case 2:
          phSensor.sendCmd("Cal,high,10.00");
          bufferCalibrated[2] = true;
          break;
        case 3:
          phSensor.sendCmd("Cal,clear");
          bufferCalibrated[0] = bufferCalibrated[1] = bufferCalibrated[2] = false;
          lcd.clear();
          lcd.print("Calibration Cleared");
          delay(1500);
          lcd.clear();
          break;
        case 4:
          lcd.clear();
          lcd.print("Returning");
          delay(1000);
          lcd.clear();
          selecting = false;
          break;
      }
    }

    if (bufferCalibrated[0] && bufferCalibrated[1] && bufferCalibrated[2])
    {
      lcd.clear();
      lcd.print("All Calibrated!");
      delay(800);
      lcd.clear();
      selecting = false;
    }
  }
}

void calibrateProbeORP(ConfigState& config)
{
  int selectedItem = 0;
  bool selecting = true;
  lcd.clear();

  while (selecting)
  {
    updateGlobalBlink(config);
    lcd.setCursor(COL_LEFT, ROW_TITLE);
    lcd.print("Calibrate when ready");
    printMenuItem(COL_LEFT, ROW_1, "Cal", 0, selectedItem);
    printMenuItem(COL_LEFT, ROW_2, "Done", 1, selectedItem);

    if (!digitalRead(PinConfigurations::PIN_SW))
    {
      delay(200);
      while (!digitalRead(PinConfigurations::PIN_SW));
      switch (selectedItem)
      {
        case 0:
          orpSensor.sendCmd("Cal,222");
          lcd.clear();
          lcd.print("Calibrated at 222mV");
          delay(1000);
          selecting = false;
          lcd.clear();
          break;
        case 1:
          lcd.clear();
          lcd.print("Returning");
          delay(1000);
          selecting = false;
          lcd.clear();
      }
    }
  }
}


