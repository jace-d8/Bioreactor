// ============================================================================
// Lcd.cpp
//
// PLAIN-ENGLISH SUMMARY:
// The implementation behind the Lcd class. Most of it is short
// "pass-through" functions handing work off to the underlying I2C_LCD
// library. The interesting part is printData() near the bottom, which
// lays out the 6-channel gas readout screen.
// ============================================================================

#include "Lcd.h"
#include "Timer.h"
#include <Arduino.h>
#include <stdio.h>
// stdio.h gives us snprintf(), used below to format numbers into text.

namespace {
// "namespace { ... }" with no name creates an "anonymous namespace" --
// anything declared inside (like printChannelValue below) is only
// visible within THIS file, a common way to make a small private helper
// function that isn't part of any class.

void printChannelValue(I2C_LCD& lcd, int col, int row, bool locked, float valueML)
// Prints one channel's cumulative gas volume, showing "err" instead if
// that channel is currently rate-limit-locked.
//   lcd     - the underlying screen driver to print to (by reference, so
//             we're writing to the SAME screen object, not a copy)
//   col/row - where on the screen to print
//   locked  - true if this channel is currently error-locked
//   valueML - the cumulative gas volume to display, in mL
{
    char buffer[16];
    // A small fixed-size piece of text storage we build our formatted
    // text into before sending it to the screen -- avoids the extra
    // memory allocation a more flexible String would need, which matters
    // on a small embedded chip.
    lcd.setCursor(col, row);
    if (locked)
    {
        snprintf(buffer, sizeof(buffer), "%7s", "err");
        // "%7s" means "print this text, padded with spaces so it's at
        // least 7 characters wide" -- keeps the on-screen columns lined
        // up whether the value is "err" or a real number.
    }
    else
    {
        snprintf(buffer, sizeof(buffer), "%7.0f", valueML);
        // "%7.0f" means "print this number at least 7 characters wide,
        // with NO digits after the decimal point" -- the on-screen
        // readout now shows whole mL only (e.g. "12" instead of "12.3"),
        // per request. The underlying value in ConfigState/the SD log is
        // completely unaffected -- gasVolumeML still accumulates with
        // full float precision (down to calFactor's 0.05 mL step)
        // internally and in the CSV log (Sd.cpp's logData() still uses
        // "%.2f"); this only changes how many digits are shown on the
        // small LCD.
    }
    lcd.print(buffer);
}
}
// End of the anonymous namespace.

void Lcd::init()
{
    delay(1000);
    // Pause 1 full second -- some LCD backpack boards need a brief
    // moment to finish their own power-on startup before responding
    // correctly to commands.

    lcd_.begin(20, 4);
    lcd_.setBacklightPin(3, 1);
    lcd_.backlight();

    lcd_.clear();
    lcd_.setCursor(COL_LEFT, ROW_TITLE);
    lcd_.print("LCD initialized");
}

void Lcd::updateBlink(ConfigState& config)
{
    if (blinkTimer_.isReady())
        config.blinkState = !config.blinkState;
        // Every time the blink timer completes one interval, flip
        // blinkState -- since this runs repeatedly, it ends up toggling
        // on/off at a steady beat, which Menu.cpp uses to make the
        // selected menu item blink.
}

void Lcd::clear()
{
    lcd_.clear();
}

void Lcd::setCursor(int col, int row)
{
    lcd_.setCursor(col, row);
}

void Lcd::print(const char* text)
{
    lcd_.print(text);
}

void Lcd::print(int val)
{
    lcd_.print(val);
}
// NOTE: this int overload exists specifically to avoid a subtle C++ trap
// -- without it, lcd.print(someWholeNumber) could silently be interpreted
// via the print(char) overload below, printing a single ASCII CHARACTER
// instead of the number's digits (e.g. print(65) printing the letter "A"
// instead of "65"). See the bioreactor project's Lcd.cpp for the fuller
// explanation.

void Lcd::print(const String& text)
{
    lcd_.print(text);
}

void Lcd::print(char c)
{
    lcd_.print(c);
}

void Lcd::print(float value, int decimals)
{
    lcd_.print(value, decimals);
}

void Lcd::printData(const ConfigState& config)
// 6 channels across 3 rows, 2 per row: row 0 = channels 1/2, row 1 =
// channels 3/4, row 2 = channels 5/6. Row 3 is left for the main
// sketch's status line (rate-error / SD-error indicator), same
// convention as the bioreactor project.
{
    const int LABEL_COL_LEFT  = COL_LEFT;
    const int VALUE_COL_LEFT  = COL_LEFT + 3;
    const int LABEL_COL_RIGHT = COL_RIGHT;
    const int VALUE_COL_RIGHT = COL_RIGHT + 3;
    // Work out the screen columns for each piece of text. "COL_LEFT + 3"
    // means "3 characters to the right of the left edge" -- enough room
    // for a label like "g1:" before the actual number starts. The full
    // row layout ends up being: cols 0-2 "g1:", cols 3-9 value (7 chars),
    // cols 10-12 "g2:", cols 13-19 value (7 chars) -- exactly filling the
    // 20-character-wide screen with no overflow.

    lcd_.setCursor(LABEL_COL_LEFT, ROW_TITLE);
    lcd_.print("g1:");
    printChannelValue(lcd_, VALUE_COL_LEFT, ROW_TITLE, config.channelErrorLocked[0], config.gasVolumeML[0]);
    // Channel 1: label "g1:", then its cumulative volume from
    // config.gasVolumeML[0] (array index 0 = the first channel), showing
    // "err" instead if config.channelErrorLocked[0] is true.

    lcd_.setCursor(LABEL_COL_RIGHT, ROW_TITLE);
    lcd_.print("g2:");
    printChannelValue(lcd_, VALUE_COL_RIGHT, ROW_TITLE, config.channelErrorLocked[1], config.gasVolumeML[1]);
    // Channel 2, same row, right half of the screen.

    lcd_.setCursor(LABEL_COL_LEFT, ROW_1);
    lcd_.print("g3:");
    printChannelValue(lcd_, VALUE_COL_LEFT, ROW_1, config.channelErrorLocked[2], config.gasVolumeML[2]);

    lcd_.setCursor(LABEL_COL_RIGHT, ROW_1);
    lcd_.print("g4:");
    printChannelValue(lcd_, VALUE_COL_RIGHT, ROW_1, config.channelErrorLocked[3], config.gasVolumeML[3]);

    lcd_.setCursor(LABEL_COL_LEFT, ROW_2);
    lcd_.print("g5:");
    printChannelValue(lcd_, VALUE_COL_LEFT, ROW_2, config.channelErrorLocked[4], config.gasVolumeML[4]);

    lcd_.setCursor(LABEL_COL_RIGHT, ROW_2);
    lcd_.print("g6:");
    printChannelValue(lcd_, VALUE_COL_RIGHT, ROW_2, config.channelErrorLocked[5], config.gasVolumeML[5]);
    // Channels 3-6 follow the identical pattern on rows 1 and 2.

    // Row 3 is deliberately left untouched here -- the main .ino writes
    // status messages (rate errors, SD errors) there instead. Keeping
    // that logic out of this function means printData() can be called
    // repeatedly without ever fighting over that row.
}
