#pragma once
#include <I2C_LCD.h>
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
    I2C_LCD lcd_;
    Timer blinkTimer_{TimingIntervals::BLINK_INTERVAL};

public:
    Lcd(uint8_t address = 0x27, TwoWire *bus = &Wire1)
    : lcd_(address, bus) {}

    void init();
    void clear();

    void print(const char* str);
    void print(int val);
    void print(float val, int prec);
    void print(const String& text);
    void print(char c);           
    void setCursor(int col, int row);

    void updateBlink(ConfigState& config);
    void printData(const ConfigState& config);
};
