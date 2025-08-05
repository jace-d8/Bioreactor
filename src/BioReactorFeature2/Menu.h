//
// Created by Jace Dunn on 7/29/25.
//
#pragma once
#include "Config.h"
#include "EzoBoard.h"


struct MenuItem {
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

extern MenuItem menuChoices[];
extern MenuItem calMenuChoices[];

void toggleMenu(ConfigState& config, EzoBoard& phSensor, EzoBoard& orpSensor);
void analogControl(int& selectedItem);
void isPressed(bool &calibrateMenu, int selectedItem, ConfigState& config, EzoBoard& phSensor, EzoBoard& orpSensor);

// Menu display
void printLcdMenu(ConfigState& config, int selectedItem);
void printMenuItem(int col, int row, const char* label, int itemIndex, int selectedIndex, const ConfigState& config);
void displayWarning(ConfigState& config);

// Calibration
void calibrateProbePH(ConfigState& config, EzoBoard& phSensor);
void calibrateProbeORP(ConfigState& config, EzoBoard& orpSensor);

// PH Calibration Helpers
void handlePHCalibrationSelection(int bufferSelection, bool bufferCalibrated[3], EzoBoard& phSensor, bool& selecting);
void displayPHMenu(bool bufferCalibrated[3], int bufferSelection, const ConfigState& config);
bool allPHBuffersCalibrated(bool bufferCalibrated[3]);

// ORP Calibration Helpers
void handleORPSelection(int selectedItem, bool& selecting, EzoBoard& orpSensor);
void displayORPCalibrationMenu(int selectedItem, const ConfigState& config);


