//
// Created by Jace Dunn on 7/28/25.
//
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
  COL_RIGHT = 10
};

struct MenuItem
{
  int col;
  int row;
  const char* label;
  int index;
};

extern LiquidCrystal_I2C lcd;
extern MenuItem menuChoices[];
extern MenuItem calMenuChoices[];

void initLcd();
void toggleMenu(ConfigState& config);
void printLcdMenu(ConfigState& config, int selectedItem);
void updateGlobalBlink(ConfigState& config);
void printMenuItem(int col, int row, const char* label, int itemIndex, int selectedIndex);

