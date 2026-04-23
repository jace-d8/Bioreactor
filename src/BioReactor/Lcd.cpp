#include "Lcd.h"
#include "Timer.h"
#include <Arduino.h>
#include <stdio.h>

namespace {
void printProbeValue(I2C_LCD& lcd, int col, int row, bool locked, float value, bool isPh)
{
    char buffer[16];
    lcd.setCursor(col, row);
    if (locked)
    {
        snprintf(buffer, sizeof(buffer), "%6s", "err");
    }
    else if (isPh)
    {
        snprintf(buffer, sizeof(buffer), "%6.2f", value);
    }
    else
    {
        snprintf(buffer, sizeof(buffer), "%6.0f", value);
    }
    lcd.print(buffer);
}
}

void Lcd::init()
{
    delay(1000);

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
{
    const int PH_LABEL_COL = COL_LEFT;
    const int PH_VALUE_COL = COL_LEFT + 3;
    const int ORP_LABEL_COL = COL_RIGHT;
    const int ORP_VALUE_COL = COL_RIGHT + 3;

    lcd_.setCursor(PH_LABEL_COL, ROW_TITLE);
    lcd_.print("p1:");
    printProbeValue(lcd_, PH_VALUE_COL, ROW_TITLE, config.valveErrorLocked[0], config.phValues[0], true);

    lcd_.setCursor(PH_LABEL_COL, ROW_1);
    lcd_.print("p2:");
    printProbeValue(lcd_, PH_VALUE_COL, ROW_1, config.valveErrorLocked[2], config.phValues[1], true);

    lcd_.setCursor(PH_LABEL_COL, ROW_2);
    lcd_.print("p3:");
    printProbeValue(lcd_, PH_VALUE_COL, ROW_2, config.valveErrorLocked[4], config.phValues[2], true);

    lcd_.setCursor(ORP_LABEL_COL, ROW_TITLE);
    lcd_.print("o1:");
    printProbeValue(lcd_, ORP_VALUE_COL, ROW_TITLE, config.valveErrorLocked[1], config.orpValues[0], false);

    lcd_.setCursor(ORP_LABEL_COL, ROW_1);
    lcd_.print("o2:");
    printProbeValue(lcd_, ORP_VALUE_COL, ROW_1, config.valveErrorLocked[3], config.orpValues[1], false);

    lcd_.setCursor(ORP_LABEL_COL, ROW_2);
    lcd_.print("o3:");
    printProbeValue(lcd_, ORP_VALUE_COL, ROW_2, config.valveErrorLocked[5], config.orpValues[2], false);
}
