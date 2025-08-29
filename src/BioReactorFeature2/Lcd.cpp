#include "Lcd.h"
#include "Timer.h"

void Lcd::init()
{
    lcd_.init();
    lcd_.backlight();
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
    lcd_.setCursor(COL_LEFT, ROW_TITLE);
    lcd_.print("pH: ");
    lcd_.print(config.phValue, 3);
    lcd_.print("     ");
    lcd_.setCursor(COL_LEFT, ROW_1);
    lcd_.print("ORP: ");
    lcd_.print(config.orpValue, 0);
    lcd_.print(" mV     ");
}
