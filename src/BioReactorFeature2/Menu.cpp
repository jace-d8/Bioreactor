#include "Menu.h"
#include "Lcd.h"
#include "ShowMessage.h"

Menu::Menu(ConfigState* config, EzoBoard* phSensor, EzoBoard* orpSensor)
    : config_(config), phSensor_(phSensor), orpSensor_(orpSensor) {}

void Menu::toggle() 
{
    active_ = true;
    selectedItem_ = 0;
    while (active_) 
    {
        analogControl(selectedItem_);
        updateGlobalBlink(*config_);
        printLcdMenu(selectedItem_);
        isPressed(active_);
    }
    
}

void Menu::analogControl(int& selectedItem) {
    int yVal = analogRead(PinConfigurations::PIN_JOYSTICK_Y);
    const int deadZone = 400;
    const int range = 1980;
    if (yVal < range - deadZone) {
        if (selectedItem > 0) selectedItem--;
        delay(200);
    } else if (yVal > range + deadZone) {
        if (selectedItem < MENU_DONE) selectedItem++;
        delay(200);
    }
}

void Menu::isPressed(bool& menuActive) {
    if (!digitalRead(PinConfigurations::PIN_SW)) {
        while (!digitalRead(PinConfigurations::PIN_SW));
        switch (selectedItem_) {
            case MENU_PH1: calibrateProbePH(); break;
            case MENU_ORP1: calibrateProbeORP(); break;
            case MENU_VALVE_TOGGLE:
                config_->valvesDisabled = !config_->valvesDisabled;
                lcd.clear();
                lcd.print(config_->valvesDisabled ? "Valves off" : "Valves on");
                delay(600);
                lcd.clear();
                break;
            case MENU_DONE:
                lcd.clear();
                menuActive = false;
                break;
        }
    }
}

void Menu::printLcdMenu(int selectedItem) {
    lcd.setCursor(COL_LEFT, ROW_TITLE);
    lcd.print("Calibrate Probe:");
    for (const auto& item : menuChoices_) {
        printMenuItem(item.col, item.row, item.label, item.index, selectedItem);
    }
    printMenuItem(COL_FAR_RIGHT, ROW_1, config_->valvesDisabled ? "Vl ON" : "Vl OFF", MENU_VALVE_TOGGLE, selectedItem);
    printMenuItem(COL_FAR_RIGHT, ROW_2, "Done", MENU_DONE, selectedItem);
}

void Menu::printMenuItem(int col, int row, const char* label, int itemIndex, int selectedIndex, int blinkSize) {
    lcd.setCursor(col, row);
    if (itemIndex == selectedIndex && config_->blinkState) {
        for (int i = 0; i < blinkSize; ++i)
            lcd.print(' ');
    } else {
        lcd.print(label);
    }
}

void Menu::displayWarning() {
    updateGlobalBlink(*config_);
    lcd.setCursor(COL_LEFT, ROW_TITLE);
    lcd.print("WARNING:");
    lcd.setCursor(COL_LEFT, ROW_1);
    lcd.print("pH buffer not reacting");
    printMenuItem(COL_LEFT, ROW_2, "Unlock System?", MENU_PH1, MENU_PH1);
}

void Menu::displayPHMenu() {
    lcd.setCursor(COL_LEFT, ROW_TITLE);
    lcd.print("Select Buffer:");

    printMenuItem(COL_LEFT,  ROW_1, "pH 4",   BUFFER_4, phBufferSelection_);
    printMenuItem(COL_LEFT,  ROW_2, "pH 7",   BUFFER_7, phBufferSelection_);
    printMenuItem(COL_LEFT,  ROW_3, "pH 10",  BUFFER_10, phBufferSelection_);
    printMenuItem(COL_RIGHT, ROW_1, "Clear",  BUFFER_CLEAR, phBufferSelection_);
    printMenuItem(COL_RIGHT, ROW_2, "Done",   BUFFER_DONE, phBufferSelection_);

    if (bufferCalibrated_[0]) lcd.setCursor(COL_MID, ROW_1), lcd.print("*");
    if (bufferCalibrated_[1]) lcd.setCursor(COL_MID, ROW_2), lcd.print("*");
    if (bufferCalibrated_[2]) lcd.setCursor(COL_MID, ROW_3), lcd.print("*");
}

void Menu::handlePHCalibrationSelection(int bufferSelection, bool& selecting) {
    switch (bufferSelection) {
        case BUFFER_4:  phSensor_->sendCmd("Cal,low,4.00");  bufferCalibrated_[0] = true; break;
        case BUFFER_7:  phSensor_->sendCmd("Cal,mid,7.00");  bufferCalibrated_[1] = true; break;
        case BUFFER_10: phSensor_->sendCmd("Cal,high,10.00"); bufferCalibrated_[2] = true; break;
        case BUFFER_CLEAR:
            phSensor_->sendCmd("Cal,clear");
            bufferCalibrated_[0] = bufferCalibrated_[1] = bufferCalibrated_[2] = false;
            lcd.clear();
            lcd.print("Calibration Cleared");
            delay(1500);
            lcd.clear();
            break;
        case BUFFER_DONE:
            selecting = false;
            lcd.clear();
            break;
    }
}

bool Menu::allPHBuffersCalibrated() {
    return bufferCalibrated_[0] && bufferCalibrated_[1] && bufferCalibrated_[2];
}

void Menu::calibrateProbePH() {
    phBufferSelection_ = 0;
    bool selecting = true;
    lcd.clear();

    while (selecting) {
        analogControl(phBufferSelection_);
        displayPHMenu();
        updateGlobalBlink(*config_);

        if (!digitalRead(PinConfigurations::PIN_SW)) {
            while (!digitalRead(PinConfigurations::PIN_SW));
            handlePHCalibrationSelection(phBufferSelection_, selecting);
        }

        if (allPHBuffersCalibrated()) {
            lcd.clear();
            lcd.print("All Calibrated!");
            delay(800);
            lcd.clear();
            selecting = false;
        }
    }
}

void Menu::displayORPCalibrationMenu(int selectedItem) {
    lcd.setCursor(COL_LEFT, ROW_TITLE);
    lcd.print("Calibrate when ready");
    printMenuItem(COL_LEFT, ROW_1, "Cal", 0, selectedItem);
    printMenuItem(COL_LEFT, ROW_2, "Done", 1, selectedItem);
}

void Menu::handleORPSelection(int selectedItem, bool& selecting) {
    switch (selectedItem) {
        case 0:
            orpSensor_->sendCmd("Cal,222");
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

void Menu::calibrateProbeORP() {
    int selectedItem = 0;
    bool selecting = true;
    lcd.clear();

    while (selecting) {
        analogControl(selectedItem);
        updateGlobalBlink(*config_);
        displayORPCalibrationMenu(selectedItem);

        if (!digitalRead(PinConfigurations::PIN_SW)) {
            delay(200);
            while (!digitalRead(PinConfigurations::PIN_SW));
            handleORPSelection(selectedItem, selecting);
        }
    }
}
