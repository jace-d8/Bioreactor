#pragma once
#include "Lcd.h"
#include "Config.h"
#include "EzoBoard.h"
#include "Joystick.h"
#include "Timer.h"

struct MenuItem {
    int col;
    int row;
    const char* label;
    int index;
};

enum MenuIndices {
    MENU_PH1 = 0,
    MENU_PH2,
    MENU_PH3,
    MENU_ORP1,
    MENU_ORP2,
    MENU_ORP3,
    MENU_DONE
};

enum PhCalibrationIndices {
    BUFFER_4 = 0,
    BUFFER_7,
    BUFFER_10,
    BUFFER_CLEAR,
    BUFFER_DONE
};

class Menu {
private:
    // Core state / deps
    ConfigState* config_;
    EzoBoard* phSensor_;
    EzoBoard* orpSensor_;
    Lcd& lcd;
    Joystick joystick{PinConfigurations::PIN_Y, PinConfigurations::PIN_SW, MENU_DONE};

    // Menu state
    enum class MenuState { Idle, Calibrating, Valves, PhCalibration, OrpCalibration, Off };
    MenuState state_ = MenuState::Off;

    // Calibration progress
    bool bufferCalibrated_[3] = { false, false, false };

    // UI throttling + dirty tracking (Option 2)
    int lastSelectedItem_ = -1;
    MenuState lastDrawnState_ = MenuState::Off;
    bool needsFullRedraw_ = true;
    Timer uiTimer_{80};  // ~12.5 FPS render cap

    // Static menu layouts
    const MenuItem calMenuChoices_[6] = {
        { COL_LEFT,  ROW_1, "pH 1",  MENU_PH1 },
        { COL_LEFT,  ROW_2, "pH 2",  MENU_PH2 },
        { COL_LEFT,  ROW_3, "pH 3",  MENU_PH3 },
        { COL_MID,   ROW_1, "ORP 1", MENU_ORP1 },
        { COL_MID,   ROW_2, "ORP 2", MENU_ORP2 },
        { COL_MID,   ROW_3, "ORP 3", MENU_ORP3 }
    };

    const MenuItem phMenuChoices_[5] = {
        { COL_LEFT,     ROW_1, "pH 4",   BUFFER_4 },
        { COL_LEFT,     ROW_2, "pH 7",   BUFFER_7 },
        { COL_LEFT,     ROW_3, "pH 10",  BUFFER_10 },
        { COL_RIGHT,    ROW_1, "Clear",  BUFFER_CLEAR },
        { COL_RIGHT,    ROW_2, "Done",   BUFFER_DONE }
    };

public:
    Menu(ConfigState* config, EzoBoard* phSensor, EzoBoard* orpSensor, Lcd& lcd);
    void enter();
    void update();
    void draw();
    void displayError(const String& error);
    bool isActive() const { return state_ != MenuState::Off; }

private:
    // Flow handlers
    void handleMainMenu();
    void handleProbesMenu();
    void handleValvesMenu();
    void handlePhCalibrationSelection();
    void handleORPSelection();

    // Screens
    void displayMainMenu();
    void displayProbesMenu();
    void displayValvesMenu();
    void displayPhMenu();
    void displayORPCalibrationMenu();

    // Helpers
    bool allPHBuffersCalibrated();
    void printMenuItem(int col, int row, const char* label, int itemIndex);
};
