#pragma once
#include "Lcd.h"
#include "Config.h"
#include "EzoBoard.h"

struct MenuItem 
{
    int col;
    int row;
    const char* label;
    int index;
};

enum MenuIndices
{
    MENU_PH1 = 0,
    MENU_PH2,
    MENU_PH3,
    MENU_ORP1,
    MENU_ORP2,
    MENU_ORP3,
    MENU_VALVE_TOGGLE,
    MENU_DONE
};

enum PhCalibrationIndices
{
    BUFFER_4 = 0,
    BUFFER_7,
    BUFFER_10,
    BUFFER_CLEAR,
    BUFFER_DONE
};

// -------------------- Globals --------------------
// extern const MenuItem menuChoices[];
// extern const MenuItem calMenuChoices[];

class Menu 
{
private:
    bool active_ = false;
    int selectedItem_ = 0;
    ConfigState* config_;
    EzoBoard* phSensor_;
    EzoBoard* orpSensor_;

    bool bufferCalibrated_[3] = { false, false, false };
    int phBufferSelection_ = 0;
    int orpCalSelection_ = 0;
    bool calibratingPH_ = false;
    bool calibratingORP_ = false;

    const MenuItem menuChoices_[6] = 
    {
        { COL_LEFT,  ROW_1, "pH 1",  MENU_PH1 },
        { COL_LEFT,  ROW_2, "pH 2",  MENU_PH2 },
        { COL_LEFT,  ROW_3, "pH 3",  MENU_PH3 },
        { COL_MID,   ROW_1, "ORP 1", MENU_ORP1 },
        { COL_MID,   ROW_2, "ORP 2", MENU_ORP2 },
        { COL_MID,   ROW_3, "ORP 3", MENU_ORP3 }
    };

    const MenuItem calMenuChoices_[5] = 
    {
        { COL_LEFT,     ROW_1, "pH 4",  0 },
        { COL_LEFT,     ROW_2, "pH 7",  1 },
        { COL_LEFT,     ROW_3, "pH 10", 2 },
        { COL_RIGHT,    ROW_1, "Clear", 3 },
        { COL_RIGHT,    ROW_2, "Done",  4 }
    };

public:
    Menu(ConfigState* config, EzoBoard* phSensor, EzoBoard* orpSensor);
    void toggle();
    void displayWarning();
private:
    void analogControl(int& selectedItem);
    void isPressed(bool& menuActive);
    void printLcdMenu(int selectedItem);
    void printMenuItem(int col, int row, const char* label, int itemIndex, int selectedIndex, int blinkSize = 6);
    void calibrateProbePH();
    void handlePHCalibrationSelection(int bufferSelection, bool& selecting);
    void displayPHMenu();
    bool allPHBuffersCalibrated();
    void calibrateProbeORP();
    void handleORPSelection(int selectedItem, bool& selecting);
    void displayORPCalibrationMenu(int selectedItem);
};