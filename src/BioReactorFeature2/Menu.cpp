#include "Menu.h"

Menu::Menu(ConfigState* config, EzoBoard* phSensor, EzoBoard* orpSensor, Lcd& lcd)
    : config_(config), phSensor_(phSensor), orpSensor_(orpSensor), lcd(lcd) {}

void Menu::enter() 
{
    if (state_ == MenuState::Off) 
    {
        state_ = MenuState::Idle;
        needsFullRedraw_ = true;
        joystick.setSelectedItem(0);
        joystick.setMaxIndex(2); // Main menu 0..2
    }
}

void Menu::update() 
{
    bool pressed = joystick.isPressed();     

    int before = joystick.getSelectedItem();
    if (!pressed)
    {                         
        joystick.move();
        if (joystick.getSelectedItem() != before)
            needsFullRedraw_ = true;
    }

    switch (state_)
    {
        case MenuState::Idle:
            handleMainMenu();
            break;
        case MenuState::Calibrating:
            handleProbesMenu();
            break;
        case MenuState::PhCalibration:
            if (joystick.isPressed()) 
            {
                handlePhCalibrationSelection();
            }
            break;
        case MenuState::OrpCalibration:
            if (joystick.isPressed()) 
            {
                handleORPSelection();
            }
            break;
        case MenuState::Valves:
            handleValvesMenu();
            break;
        case MenuState::Off:             
            break;
    }
}

void Menu::draw() 
{
    lcd.updateBlink(*config_);

    static bool lastBlink = true;
    bool blinkToggled = (config_->blinkState != lastBlink);
    if (blinkToggled) lastBlink = config_->blinkState;

    if (!needsFullRedraw_ && !blinkToggled) 
    {
        if (!uiTimer_.isReady()) return;
    }

    if (state_ != lastDrawnState_) 
    {
        needsFullRedraw_ = true;
        lastDrawnState_ = state_;
    }

    switch (state_)
    {
        case MenuState::Idle:
            displayMainMenu();
            break;
        case MenuState::Calibrating:
            displayProbesMenu();
            break;
        case MenuState::PhCalibration:
            displayPhMenu();
            break;
        case MenuState::OrpCalibration:
            displayORPCalibrationMenu();
            break;
        case MenuState::Valves:
            displayValvesMenu();
            break;
        case MenuState::Off:
            break;
    }

    uiTimer_.reset();
    needsFullRedraw_ = false;
    lastSelectedItem_ = joystick.getSelectedItem();
}

void Menu::handleMainMenu() 
{
    if (!joystick.isPressed()) return;

    switch (joystick.getSelectedItem()) 
    {
        case 0:
            joystick.setSelectedItem(0);
            joystick.setMaxIndex(MENU_DONE);
            state_ = MenuState::Calibrating;
            break;
        case 1:
            joystick.setSelectedItem(0);
            joystick.setMaxIndex(1);
            state_ = MenuState::Valves;
            break;
        case 2:
            state_ = MenuState::Off;
            lcd.clear();                 
            needsFullRedraw_ = true;    
            break;
    }
    needsFullRedraw_ = true;
}

void Menu::handleProbesMenu() 
{
    if (!joystick.isPressed()) return;

    switch (joystick.getSelectedItem()) 
    {
        case MENU_PH1:
        case MENU_PH2:
        case MENU_PH3:
            joystick.setSelectedItem(BUFFER_4);
            joystick.setMaxIndex(4);
            state_ = MenuState::PhCalibration;
            break;

        case MENU_ORP1:
        case MENU_ORP2:
        case MENU_ORP3:
            joystick.setSelectedItem(0);
            joystick.setMaxIndex(1);
            state_ = MenuState::OrpCalibration;
            break;

        case MENU_DONE:
            joystick.setSelectedItem(0);
            joystick.setMaxIndex(2);
            state_ = MenuState::Idle;
            break;
    }
    needsFullRedraw_ = true;
}

void Menu::handleValvesMenu() 
{
    if (!joystick.isPressed()) return;

    switch (joystick.getSelectedItem()) 
    {
        case 0:
            config_->valvesDisabled = !config_->valvesDisabled;
            break;
        case 1:
            joystick.setSelectedItem(0);
            joystick.setMaxIndex(2);
            state_ = MenuState::Idle;
            break;
    }
    needsFullRedraw_ = true;
}

void Menu::handlePhCalibrationSelection() 
{
    switch (joystick.getSelectedItem()) 
    {
        case BUFFER_4:
            phSensor_->sendCmd("Cal,low,4.00");
            bufferCalibrated_[0] = true;
            break;
        case BUFFER_7:
            phSensor_->sendCmd("Cal,mid,7.00");
            bufferCalibrated_[1] = true;
            break;
        case BUFFER_10:
            phSensor_->sendCmd("Cal,high,10.00");
            bufferCalibrated_[2] = true;
            break;
        case BUFFER_CLEAR:
            phSensor_->sendCmd("Cal,clear");
            bufferCalibrated_[0] = bufferCalibrated_[1] = bufferCalibrated_[2] = false;
            break;
        case BUFFER_DONE:
            joystick.setSelectedItem(0);
            joystick.setMaxIndex(MENU_DONE);
            state_ = MenuState::Calibrating;
            break;
    }
    needsFullRedraw_ = true;
}

void Menu::handleORPSelection() 
{
    switch (joystick.getSelectedItem()) 
    {
        case 0:
            orpSensor_->sendCmd("Cal,222");
            lcd.setCursor(COL_LEFT, ROW_3);
            lcd.print("Calibrated @222mV   ");
            delay(350);
            joystick.setSelectedItem(0);
            joystick.setMaxIndex(MENU_DONE);
            state_ = MenuState::Calibrating;
            break;
        case 1:
            joystick.setSelectedItem(0);
            joystick.setMaxIndex(MENU_DONE);
            state_ = MenuState::Calibrating;
            break;
    }
    needsFullRedraw_ = true;
}

/* ===== Screens ===== */

void Menu::displayMainMenu() 
{
    if (needsFullRedraw_) 
    {
        lcd.clear();
        lcd.setCursor(COL_LEFT, ROW_TITLE);
        lcd.print("MAIN MENU");
        lcd.setCursor(COL_LEFT, ROW_1); lcd.print("Calibrate Probes");
        lcd.setCursor(COL_LEFT, ROW_2); lcd.print("Valve Control");
        lcd.setCursor(COL_LEFT, ROW_3); lcd.print("Exit");
    }

    if (!needsFullRedraw_ && lastSelectedItem_ != -1 && lastSelectedItem_ != joystick.getSelectedItem()) 
    {
        switch (lastSelectedItem_) 
        {
            case 0: lcd.setCursor(COL_LEFT, ROW_1); lcd.print("Calibrate Probes"); break;
            case 1: lcd.setCursor(COL_LEFT, ROW_2); lcd.print("Valve Control   "); break;
            case 2: lcd.setCursor(COL_LEFT, ROW_3); lcd.print("Exit            "); break;
        }
    }

    switch (joystick.getSelectedItem()) 
    {
        case 0: printMenuItem(COL_LEFT, ROW_1, "Calibrate Probes", 0); break;
        case 1: printMenuItem(COL_LEFT, ROW_2, "Valve Control",    1); break;
        case 2: printMenuItem(COL_LEFT, ROW_3, "Exit",             2); break;
    }
}

void Menu::displayProbesMenu() 
{
    if (needsFullRedraw_) 
    {
        lcd.clear();
        lcd.setCursor(COL_LEFT, ROW_TITLE);
        lcd.print("Calibrate Probe:");
        for (const auto& item : calMenuChoices_) 
        {
            lcd.setCursor(item.col, item.row);
            lcd.print(item.label);
        }
        lcd.setCursor(COL_FAR_RIGHT, ROW_2);
        lcd.print("Done");
        joystick.setMaxIndex(MENU_DONE);
    }

    if (!needsFullRedraw_ && lastSelectedItem_ != -1 && lastSelectedItem_ != joystick.getSelectedItem()) 
    {
        if (lastSelectedItem_ == MENU_DONE) 
        {
            lcd.setCursor(COL_FAR_RIGHT, ROW_2); lcd.print("Done");
        } 
        else if (lastSelectedItem_ >= 0 && lastSelectedItem_ < 6) 
        {
            const MenuItem& prev = calMenuChoices_[lastSelectedItem_];
            lcd.setCursor(prev.col, prev.row);
            lcd.print(prev.label);
        }
    }

    switch (joystick.getSelectedItem()) 
    {
        case MENU_DONE:
            printMenuItem(COL_FAR_RIGHT, ROW_2, "Done", MENU_DONE);
            break;
        default:
            if (joystick.getSelectedItem() >= 0 && joystick.getSelectedItem() < 6) 
            {
                const MenuItem& it = calMenuChoices_[joystick.getSelectedItem()];
                printMenuItem(it.col, it.row, it.label, it.index);
            }
            break;
    }
}

void Menu::displayValvesMenu() 
{
    if (needsFullRedraw_) 
    {
        lcd.clear();
        lcd.setCursor(COL_LEFT, ROW_TITLE);
        lcd.print(config_->valvesDisabled ? "Valves are: OFF" : "Valves are: ON ");
        lcd.setCursor(COL_LEFT, ROW_1); lcd.print("Switch valves");
        lcd.setCursor(COL_LEFT, ROW_2); lcd.print("Return");
        joystick.setMaxIndex(1);
    }

    if (!needsFullRedraw_ && lastSelectedItem_ != -1 && lastSelectedItem_ != joystick.getSelectedItem()) 
    {
        switch (lastSelectedItem_) 
        {
            case 0: lcd.setCursor(COL_LEFT, ROW_1); lcd.print("Switch valves"); break;
            case 1: lcd.setCursor(COL_LEFT, ROW_2); lcd.print("Return       "); break;
        }
    }

    switch (joystick.getSelectedItem()) 
    {
        case 0: printMenuItem(COL_LEFT, ROW_1, "Switch valves", 0); break;
        case 1: printMenuItem(COL_LEFT, ROW_2, "Return",        1); break;
    }
}

void Menu::displayPhMenu() 
{
    if (needsFullRedraw_) 
    {
        lcd.clear();
        lcd.setCursor(COL_LEFT, ROW_TITLE);
        lcd.print("Select Buffer:");
        for (const auto& item : phMenuChoices_) 
        {
            lcd.setCursor(item.col, item.row);
            lcd.print(item.label);
        }
        if (bufferCalibrated_[0]) { lcd.setCursor(COL_MID, ROW_1); lcd.print("*"); }
        if (bufferCalibrated_[1]) { lcd.setCursor(COL_MID, ROW_2); lcd.print("*"); }
        if (bufferCalibrated_[2]) { lcd.setCursor(COL_MID, ROW_3); lcd.print("*"); }
        joystick.setMaxIndex(4);
    }

    if (!needsFullRedraw_ && lastSelectedItem_ != -1 && lastSelectedItem_ != joystick.getSelectedItem()) 
    {
        const MenuItem& prev = phMenuChoices_[lastSelectedItem_];
        lcd.setCursor(prev.col, prev.row);
        lcd.print(prev.label);
    }

    const MenuItem& cur = phMenuChoices_[joystick.getSelectedItem()];
    printMenuItem(cur.col, cur.row, cur.label, cur.index);
}

void Menu::displayORPCalibrationMenu() 
{
    if (needsFullRedraw_) 
    {
        lcd.clear();
        lcd.setCursor(COL_LEFT, ROW_TITLE);
        lcd.print("Calibrate when ready");
        lcd.setCursor(COL_LEFT, ROW_1); lcd.print("Calibrate at 222mV");
        lcd.setCursor(COL_LEFT, ROW_2); lcd.print("Done");
        joystick.setMaxIndex(1);
    }

    if (!needsFullRedraw_ && lastSelectedItem_ != -1 && lastSelectedItem_ != joystick.getSelectedItem()) 
    {
        switch (lastSelectedItem_) 
        {
            case 0: lcd.setCursor(COL_LEFT, ROW_1); lcd.print("Calibrate at 222mV"); break;
            case 1: lcd.setCursor(COL_LEFT, ROW_2); lcd.print("Done               "); break;
        }
    }

    switch (joystick.getSelectedItem()) 
    {
        case 0: printMenuItem(COL_LEFT, ROW_1, "Calibrate at 222mV", 0); break;
        case 1: printMenuItem(COL_LEFT, ROW_2, "Done",               1); break;
    }
}

/* ===== Helpers ===== */

bool Menu::allPHBuffersCalibrated() 
{
    return bufferCalibrated_[0] && bufferCalibrated_[1] && bufferCalibrated_[2];
}

void Menu::displayError(const String& error) 
{
    lcd.setCursor(COL_LEFT, ROW_TITLE); lcd.print("WARNING:           ");
    lcd.setCursor(COL_LEFT, ROW_1);     lcd.print(error);
    lcd.setCursor(COL_LEFT, ROW_2);     lcd.print("Unlock System?    ");
}

void Menu::printMenuItem(int col, int row, const char* label, int itemIndex) 
{
    lcd.setCursor(col, row);
    if (itemIndex == joystick.getSelectedItem() && config_->blinkState) 
    {
        for (int i = 0; i < (int)strlen(label); ++i) lcd.print(' ');
    } 
    else 
    {
        lcd.print(label);
    }
}
