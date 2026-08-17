// ============================================================================
// Lcd.h
//
// PLAIN-ENGLISH SUMMARY:
// This defines a "wrapper" class around the physical 20x4 character LCD
// screen, playing the exact same role as the bioreactor project's Lcd
// class -- see there for a fuller explanation of why a wrapper class is
// used at all (in short: so every other file talks to THIS class instead
// of the underlying screen library directly, making it easy to change
// the physical screen later without touching every other file). The only
// real difference from the bioreactor's version is what printData() draws:
// 6 gas-channel volumes here, instead of 3 reactors' pH/ORP readings.
// ============================================================================

#pragma once
#include <I2C_LCD.h>
#include "Config.h"
#include "Timer.h"

enum LcdPos
// A named set of screen coordinates, so the rest of the code can write
// "ROW_1" instead of the plain number "1" -- easier to read, much harder
// to mistype.
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
    // The actual underlying LCD driver object that does the real work of
    // sending characters to the screen over I2C.

    Timer blinkTimer_{TimingIntervals::BLINK_INTERVAL};
    // A Timer (see Timer.h) used to flip a "should things be blinking
    // right now" flag on and off at a steady rate -- this is what makes a
    // selected menu item visibly blink.

public:
    Lcd(uint8_t address = 0x27, TwoWire *bus = &Wire1)
    : lcd_(address, bus) {}
    // Constructor. "uint8_t address = 0x27" takes a small number, the
    // screen's I2C address, defaulting to 0x27 (hexadecimal for 39) if
    // none is given -- the standard address most of these LCD backpack
    // boards ship with. "TwoWire *bus = &Wire1" takes a POINTER to the
    // I2C bus object to use, defaulting to this board's second I2C bus
    // (Wire1) -- see Config.h's LCD_PIN_SDA/LCD_PIN_SCL for which
    // physical pins that maps to.

    void init();
    // Call once at startup: turns on the screen's backlight and shows an
    // initial "LCD initialized" message.
    void clear();
    // Wipes everything off the screen.

    // Several versions of "print", one per type of value -- C++ lets you
    // define multiple functions with the SAME name as long as they take
    // DIFFERENT types of input ("overloading"); the compiler picks the
    // right one automatically based on what you pass in.
    void print(const char* str);
    void print(int val);
    void print(float val, int prec);
    void print(const String& text);
    void print(char c);
    void setCursor(int col, int row);

    void updateBlink(ConfigState& config);
    // Call this once per loop iteration; it checks blinkTimer_ and flips
    // config.blinkState true/false at a steady rate, which the menu
    // drawing code (Menu.cpp) uses to make the highlighted item blink.

    void printData(const ConfigState& config);
    // Draws the main "idle" readout screen: 6 channels' cumulative gas
    // volume (mL), 2 per row across 3 rows (mirroring the bioreactor
    // project's 3-reactor pH/ORP layout, just with 6 gas values instead
    // of 3 pH + 3 ORP values).
};
