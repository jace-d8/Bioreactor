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
Valve phValve(PH_VALVE_PIN, 10000);
Valve orpValve(ORP_VALVE_PIN, 4000);

// Probe Values
float phValue = 0.0;
int orpValue = 0;

void setup()
{
  Serial.begin(9600);              // Init serial comms at 9600 baud
  Wire.begin();                    // Init I2C
  Wire.setClock(400000);           // Set I2C speed to 400 kHz (Fast Mode)

  pinMode(PH_VALVE_PIN, OUTPUT);
  pinMode(ORP_VALVE_PIN, OUTPUT);

  pinMode(PIN_SW, INPUT_PULLUP);

  initLcd();
  lcd.setCursor(0, 1);
  if (SD.begin(SD_CHIP_SELECT))
  {
    lcd.print("SD initialized");
  }
  else
  {
    lcd.print("SD failed");
  }

  lcd.setCursor(0, 2);
  configTime(UTC_OFFSET, 0, "");
  setTimeFromBuild();
}

// Try and reduce delay blocks


void loop() // Loop has mixed responsibilty, encapuslate
{ 
  // Periodic sensor reads
  if (isCooldownOver(SENSOR_READ_INTERVAL, lastSensorReadTime))
  {
    phValue = phSensor.read();
    orpValue = orpSensor.read();
    if (!lcdCleared)
    {
      lcd.clear();
      lcdCleared = true;
    }
    printData();
    lastSensorReadTime = millis();
  }

  // Periodic SD logging
  if (isCooldownOver(120000, lastSdLogTime)) // 2 min
  {
    logToSD();
    lastSdLogTime = millis();
  }

  // PH Valve Control
  if ((phValue < PH_MINIMUM) && isCooldownOver(VALVE_COOLDOWN, lastPhValveTime))
  {
    phValve.open();
    lastPhValveTime = millis();

    // Valve activation count logic
    if (millis() - lastPhActivation > PH_RESET_WINDOW)
    {
      phValveActivationCount = 1;
    }
    else
    {
      phValveActivationCount++;
    }
    lastPhActivation = millis();

    if (phValveActivationCount >= PH_VALVE_MAX_ACTIVATIONS)
    {
      displayWarning();
      phValveActivationCount = 0;
    }
  }

  // ORP Valve Control
  if ((orpValue < ORP_MINIMUM) && isCooldownOver(VALVE_COOLDOWN, lastOrpValveTime))
  {
    orpValve.open();
    lastOrpValveTime = millis();
  }

  // Hourly ORP flush
  if (isCooldownOver(HOUR_INTERVAL, lastHourTrigger))
  {
    orpValve.open();
    lastHourTrigger = millis();
  }

  phValve.update();
  orpValve.update();
  toggleMenu();
}
bool isCooldownOver(unsigned long cooldown, unsigned long lastTime)
{
  return (millis() - lastTime >= cooldown);
}

void isPressed(bool &calibrateMenu, int selectedItem)
{
  if (!digitalRead(PIN_SW))
  {
    delay(200);  // debounce
    while (!digitalRead(PIN_SW));  // wait until released

    if (selectedItem == 0)
    {
      calibrateProbePH();
    }
    else if (selectedItem == 3)
    {
      calibrateProbeORP();
    }
    else if (selectedItem == 6)
    {
      valvesDisabled = !valvesDisabled;
      phValve.switchValve();
      orpValve.switchValve();
      lcd.clear();
      lcd.print(valvesDisabled ? "Valves off" : "Valves on");
      delay(600);
      lcd.clear();
    }
    else if (selectedItem == 7)
    {
      calibrateMenu = false;
      lcd.clear();
    }
  }
}

void analogControl(int &selectedItem)
{
  int yVal = analogRead(PIN_JOYSTICK_Y);
  const int deadZone = 400;

  if (yVal < 1980 - deadZone)  // Up
  {
    if (selectedItem > 0) selectedItem--;
    delay(200); // Debounce
  }
  else if (yVal > 1980 + deadZone)  // Down
  {
    if (selectedItem < 7) selectedItem++;
    delay(200); // Debounce
  }
}

void calibrateProbePH()
{
  int bufferSelection = 0;
  bool bufferCalibrated[3] = {false, false, false};
  bool selecting = true;
  lcd.clear();

  while (selecting)
  {
    analogControl(bufferSelection);
    updateGlobalBlink();

    lcd.setCursor(COL_LEFT, ROW_TITLE);
    lcd.print("Select Buffer:");

    if (bufferCalibrated[0]) lcd.setCursor(6, 1), lcd.print("*");
    if (bufferCalibrated[1]) lcd.setCursor(6, 2), lcd.print("*");
    if (bufferCalibrated[2]) lcd.setCursor(6, 3), lcd.print("*");

    if (!digitalRead(PIN_SW))
    {
      while (!digitalRead(PIN_SW));

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

void calibrateProbeORP()
{
  int selectedItem = 0;
  bool selecting = true;
  lcd.clear();

  while (selecting)
  {
    updateGlobalBlink();
    lcd.setCursor(0, 0);
    lcd.print("Calibrate when ready");
    printMenuItem(0, 1, "Cal", 0, selectedItem);
    printMenuItem(0, 2, "Done", 1, selectedItem);

    if (!digitalRead(PIN_SW))
    {
      delay(200);
      while (!digitalRead(PIN_SW));
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

void displayWarning()
{
  bool bypassWarning = false;
  lcd.clear();
  while (!bypassWarning)
  {
    updateGlobalBlink();
    lcd.setCursor(0, 0);
    lcd.print("WARNING:");
    lcd.setCursor(0, 1);
    lcd.print("pH buffer not reacting");
    printMenuItem(0, 2, "Unlock System?", 0, 0);

    if (!digitalRead(PIN_SW))
    {
      delay(200);
      while (!digitalRead(PIN_SW));
      bypassWarning = true;
      lcd.clear();
      lcd.print("System Unlocked");
      delay(700);
      lcd.clear();
    }
  }
}

void printData()
{
  lcd.setCursor(0, 0);
  lcd.print("pH: ");
  lcd.print(phValue, 3);
  lcd.print("     ");
  lcd.setCursor(0, 1);
  lcd.print("ORP: ");
  lcd.print(orpValue, 0);
  lcd.print(" mV     ");
}
 