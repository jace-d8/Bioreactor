#pragma once
#include <LiquidCrystal_I2C.h>
#include "Config.h"
#include "Timer.h"

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

class Lcd
{
private:
    LiquidCrystal_I2C lcd_;
    Timer blinkTimer_{TimingIntervals::BLINK_INTERVAL};

public:
    Lcd() : lcd_(0x27, 20, 4) {}

    void init();
    void clear();

    void print(const char* str);
    void print(float val, int prec);
    void print(const String& text);
    void print(char c);           
    void setCursor(int col, int row);

    void updateBlink(ConfigState& config);
    void printData(const ConfigState& config);
};
