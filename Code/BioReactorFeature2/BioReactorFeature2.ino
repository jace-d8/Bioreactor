#include <Wire.h>
#include "Config.h"
#include "Valve.h"
#include "EzoBoard.h"
#include "Lcd.h"
#include "Sd.h"

// Probes
EzoBoard orpSensor(98, "ORP");
EzoBoard phSensor(99, "PH");

// Valves
Valve phValve(PinConfigurations::PH_VALVE_PIN, 10000);
Valve orpValve(PinConfigurations::ORP_VALVE_PIN, 4000);

// Single runtime config
ConfigState config;

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

  // Needs to be moved 
  if (SD.begin(SD_CHIP_SELECT))
  {
    lcd.print("SD initialized");
  }
  else
  {
    lcd.print("SD failed");
  }

  // lcd.setCursor(COL_LEFT, ROW_2); Idk why this was here


  configTime(UTC_OFFSET, 0, ""); // This should be either computer time or last counted time, whichever is "higher"
  setTimeFromBuild(); // This too 
}

void loop() // minimize delay() and cut down on non modularized logic
{ 
  // Periodic sensor reads
  if (isCooldownOver(TimingIntervals::SENSOR_READ_INTERVAL, config.lastSensorReadTime))
  {
    config.phValue = phSensor.read();
    config.orpValue = orpSensor.read();
    if (!config.lcdCleared)
    {
      lcd.clear();
      config.lcdCleared = true;
    }
    printData(config);
    config.lastSensorReadTime = millis();
  }

  // Periodic SD logging
  if (isCooldownOver(120000, config.lastSdLogTime)) // 2 min
  {
    logToSD(config);
    config.lastSdLogTime = millis();
  }

  // PH Valve Control
  if ((config.phValue < Thresholds::PH_MINIMUM) && isCooldownOver(TimingIntervals::VALVE_COOLDOWN, config.lastPhValveTime))
  {
    phValve.open();
    config.lastPhValveTime = millis();

    // Valve activation count logic
    if (millis() - config.lastPhActivation > TimingIntervals::PH_RESET_WINDOW)
    {
      config.phValveActivationCount = 1;
    }
    else
    {
      config.phValveActivationCount++;
    }
    config.lastPhActivation = millis();

    if (config.phValveActivationCount >= TimingIntervals::PH_VALVE_MAX_ACTIVATIONS)
    {
      displayWarning(config);
      config.phValveActivationCount = 0;
    }
  }

  // ORP Valve Control
  if ((config.orpValue < Thresholds::ORP_MINIMUM) && isCooldownOver(TimingIntervals::VALVE_COOLDOWN, config.lastOrpValveTime))
  {
    orpValve.open();
    config.lastOrpValveTime = millis();
  }

  // Hourly ORP flush
  if (isCooldownOver(TimingIntervals::HOUR_INTERVAL, config.lastHourTrigger))
  {
    orpValve.open();
    config.lastHourTrigger = millis();
  }

  phValve.update();
  orpValve.update();
  toggleMenu(config);
}

// Will stay in the .ino
bool isCooldownOver(unsigned long cooldown, unsigned long lastTime)
{
  return (millis() - lastTime >= cooldown);
}

// Will stay in the .ino
void analogControl(int &selectedItem)
{
  int yVal = analogRead(PinConfigurations::PIN_JOYSTICK_Y);
  const int deadZone = 400;

  if (yVal < 1980 - deadZone)  // Up
  {
    if (selectedItem > MENU_PH1) selectedItem--;
    delay(200); // Debounce
  }
  else if (yVal > 1980 + deadZone)  // Down
  {
    if (selectedItem < MENU_DONE) selectedItem++;
    delay(200); // Debounce
  }
}

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


