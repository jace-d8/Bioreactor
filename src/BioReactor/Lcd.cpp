#include "Lcd.h"
#include "Timer.h"

// void Lcd::init()
// {
//     lcd_.begin(20, 4);
//     lcd_.setBacklightPin(3, 1);
//     lcd_.backlight();
//     lcd_.setCursor(COL_LEFT, ROW_TITLE);
//     lcd_.print("LCD initialized");
// }

// tmp test
void Lcd::init()
{
    // Give the backpack time to power up
    delay(1000);  // same as your test sketch; you can try 500ms later

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
// Row 0: pH 1–3
    lcd_.setCursor(COL_LEFT, ROW_TITLE);   // col 0
    lcd_.print("p1:");
    lcd_.print(config.phValues[0], 2);

    lcd_.setCursor(COL_LEFT, ROW_1);    // col 6
    lcd_.print("p2:");
    lcd_.print(config.phValues[1], 2);

    lcd_.setCursor(COL_LEFT, ROW_2);  // col 10
    lcd_.print("p3:");
    lcd_.print(config.phValues[2], 2);

    // Row 1: ORP 1–3
    lcd_.setCursor(COL_RIGHT, ROW_TITLE);
    lcd_.print("o1:");
    lcd_.print(config.orpValues[0], 0);

    lcd_.setCursor(COL_RIGHT, ROW_1);
    lcd_.print("o2:");
    lcd_.print(config.orpValues[1], 0);

    lcd_.setCursor(COL_RIGHT, ROW_2);
    lcd_.print("o3:");
    lcd_.print(config.orpValues[2], 0);

}
