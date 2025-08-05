#pragma once
#include <LiquidCrystal_I2C.h>
#include "Config.h"

enum LcdPos
{
  ROW_TITLE = 0,
  ROW_1 = 1,
  ROW_2 = 2,
  ROW_3 = 3,
  COL_LEFT = 0,
  COL_MID = 6,
  COL_RIGHT = 10,
  COL_FAR_RIGHT = 13
};

// turn into a class with fsm

extern LiquidCrystal_I2C lcd; // fix later

void initLcd();
void updateGlobalBlink(ConfigState& config);
void printData(ConfigState& config);





