#include "Menu.h"
#include "Lcd.h"
#include "Config.h"
#include "EzoBoard.h"

MenuItem menuChoices[] = {
    { COL_LEFT,  ROW_1, "pH 1",  MENU_PH1 },
    { COL_LEFT,  ROW_2, "pH 2",  MENU_PH2 },
    { COL_LEFT,  ROW_3, "pH 3",  MENU_PH3 },
    { COL_MID,   ROW_1, "ORP 1", MENU_ORP1 },
    { COL_MID,   ROW_2, "ORP 2", MENU_ORP2 },
    { COL_MID,   ROW_3, "ORP 3", MENU_ORP3 }
};

MenuItem calMenuChoices[] = {
    { COL_LEFT,     ROW_1, "pH 4",  0 },
    { COL_LEFT,     ROW_2, "pH 7",  1 },
    { COL_LEFT,     ROW_3, "pH 10", 2 },
    { COL_RIGHT,    ROW_1, "Clear", 3 },
    { COL_RIGHT,    ROW_2, "Done",  4 }
};

// -------------------- Menu Navigation --------------------
void analogControl(int& selectedItem)
{
    int yVal = analogRead(PinConfigurations::PIN_JOYSTICK_Y);
    const int deadZone = 400;
    if (yVal < 1980 - deadZone)  // Up
    {
        if (selectedItem > 0) selectedItem--;
        delay(200); // Debounce
    }
    else if (yVal > 1980 + deadZone) // Down
    {
        if (selectedItem < MENU_DONE) selectedItem++;
        delay(200); // Debounce
    }
}

void isPressed(bool &calibrateMenu, int selectedItem, ConfigState& config, EzoBoard& phSensor, EzoBoard& orpSensor)
{
    if (!digitalRead(PinConfigurations::PIN_SW))
    {
        delay(200);
        while (!digitalRead(PinConfigurations::PIN_SW));
        switch (selectedItem)
        {
            case MENU_PH1: calibrateProbePH(config, phSensor); break;
            case MENU_ORP1: calibrateProbeORP(config, orpSensor); break;
            case MENU_VALVE_TOGGLE:
                config.valvesDisabled = !config.valvesDisabled;
                lcd.clear();
                lcd.print(config.valvesDisabled ? "Valves off" : "Valves on");
                delay(600);
                lcd.clear();
                break;
            case MENU_DONE:
                calibrateMenu = false;
                lcd.clear();
                break;
        }
    }
}

void toggleMenu(ConfigState& config, EzoBoard& phSensor, EzoBoard& orpSensor)
{
    bool calibrateMenu = false;
    int selectedItem = -1;
    if (!digitalRead(PinConfigurations::PIN_SW))
    {
        delay(200);
        calibrateMenu = true;
        selectedItem = 0;
        lcd.clear();
        while (calibrateMenu)
        {
            analogControl(selectedItem);
            updateGlobalBlink(config);
            printLcdMenu(config, selectedItem);
            isPressed(calibrateMenu, selectedItem, config, phSensor, orpSensor);
        }
    }
}

void printLcdMenu(ConfigState& config, int selectedItem)
{
  lcd.setCursor(COL_LEFT, ROW_TITLE);
  lcd.print("Calibrate Probe:");

  printMenuItem(COL_LEFT,  ROW_1, "pH 1",  MENU_PH1, selectedItem);
  printMenuItem(COL_LEFT,  ROW_2, "pH 2",  MENU_PH2, selectedItem);
  printMenuItem(COL_LEFT,  ROW_3, "pH 3",  MENU_PH3, selectedItem);
  printMenuItem(COL_MID,   ROW_1, "ORP 1", MENU_ORP1, selectedItem);
  printMenuItem(COL_MID,   ROW_2, "ORP 2", MENU_ORP2, selectedItem);
  printMenuItem(COL_MID,   ROW_3, "ORP 3", MENU_ORP3, selectedItem);

  if (config.valvesDisabled)
    printMenuItem(COL_FAR_RIGHT, ROW_1, "Vl ON", MENU_VALVE_TOGGLE, selectedItem);
  else
    printMenuItem(COL_FAR_RIGHT, ROW_1, "Vl OFF", MENU_VALVE_TOGGLE, selectedItem);

  printMenuItem(COL_FAR_RIGHT, ROW_2, "Done", MENU_DONE, selectedItem);
}

void printMenuItem(int col, int row, const char* label, int itemIndex, int selectedIndex)
{
  lcd.setCursor(col, row);
  if (itemIndex == selectedIndex)
    lcd.print("      ");
  else
    lcd.print(label);
}

void displayWarning(ConfigState& config)
{
  updateGlobalBlink(config);
  lcd.setCursor(COL_LEFT, ROW_TITLE);
  lcd.print("WARNING:");
  lcd.setCursor(COL_LEFT, ROW_1);
  lcd.print("pH buffer not reacting");
  printMenuItem(COL_LEFT, ROW_2, "Unlock System?", MENU_PH1, MENU_PH1);
}

// -------------------- PH CALIBRATION HELPERS --------------------
void displayPHMenu(bool bufferCalibrated[3], int bufferSelection)
{
    analogControl(bufferSelection);
    lcd.setCursor(COL_LEFT, ROW_TITLE);
    lcd.print("Select Buffer:");

    if (bufferCalibrated[0]) lcd.setCursor(COL_MID, ROW_1), lcd.print("*");
    if (bufferCalibrated[1]) lcd.setCursor(COL_MID, ROW_2), lcd.print("*");
    if (bufferCalibrated[2]) lcd.setCursor(COL_MID, ROW_3), lcd.print("*");
}

void handlePHCalibrationSelection(int bufferSelection, bool bufferCalibrated[3], EzoBoard& phSensor)
{
    switch (bufferSelection)
    {
        case 0: phSensor.sendCmd("Cal,low,4.00");  bufferCalibrated[0] = true; break;
        case 1: phSensor.sendCmd("Cal,mid,7.00");  bufferCalibrated[1] = true; break;
        case 2: phSensor.sendCmd("Cal,high,10.00"); bufferCalibrated[2] = true; break;
        case 3:
            phSensor.sendCmd("Cal,clear");
            bufferCalibrated[0] = bufferCalibrated[1] = bufferCalibrated[2] = false;
            lcd.clear();
            lcd.print("Calibration Cleared");
            delay(1500);
            lcd.clear();
            break;
    }
}

bool allPHBuffersCalibrated(bool bufferCalibrated[3])
{
    return bufferCalibrated[0] && bufferCalibrated[1] && bufferCalibrated[2];
}

void calibrateProbePH(ConfigState& config, EzoBoard& phSensor)
{
    int bufferSelection = 0;
    bool bufferCalibrated[3] = {false, false, false};
    bool selecting = true;
    lcd.clear();

    while (selecting)
    {
        displayPHMenu(bufferCalibrated, bufferSelection);
        updateGlobalBlink(config);

        if (!digitalRead(PinConfigurations::PIN_SW))
        {
            while (!digitalRead(PinConfigurations::PIN_SW));
            handlePHCalibrationSelection(bufferSelection, bufferCalibrated, phSensor);
        }

        if (allPHBuffersCalibrated(bufferCalibrated))
        {
            lcd.clear();
            lcd.print("All Calibrated!");
            delay(800);
            lcd.clear();
            selecting = false;
        }
    }
}

// -------------------- ORP CALIBRATION HELPERS --------------------
void handleORPSelection(int selectedItem, bool &selecting, EzoBoard& orpSensor)
{
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
            break;
    }
}

void displayORPCalibrationMenu(int selectedItem)
{
    lcd.setCursor(COL_LEFT, ROW_TITLE);
    lcd.print("Calibrate when ready");
    printMenuItem(COL_LEFT, ROW_1, "Cal", 0, selectedItem);
    printMenuItem(COL_LEFT, ROW_2, "Done", 1, selectedItem);
}

void calibrateProbeORP(ConfigState& config, EzoBoard& orpSensor)
{
    int selectedItem = 0;
    bool selecting = true;
    lcd.clear();

    while (selecting)
    {
        updateGlobalBlink(config);
        displayORPCalibrationMenu(selectedItem);

        if (!digitalRead(PinConfigurations::PIN_SW))
        {
            delay(200);
            while (!digitalRead(PinConfigurations::PIN_SW));
            handleORPSelection(selectedItem, selecting, orpSensor);
        }
    }
}
