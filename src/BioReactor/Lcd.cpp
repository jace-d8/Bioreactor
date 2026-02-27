#include "Lcd.h"
#include "Timer.h"
#include <Arduino.h>
#include <stdio.h>

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

// ---- SAFE FIXED WIDTH PRINT ----
void Lcd::printData(const ConfigState& config)
{
    const int PH_LABEL_COL  = COL_LEFT;
    const int PH_VALUE_COL  = COL_LEFT + 3;

    const int ORP_LABEL_COL = COL_RIGHT;
    const int ORP_VALUE_COL = COL_RIGHT + 3;

    const int PH_WIDTH  = 6;   // fits "10.00"
    const int ORP_WIDTH = 6;   // fits up to 4-digit ORP

    char buffer[16];

    // ---------- pH ----------
    lcd_.setCursor(PH_LABEL_COL, ROW_TITLE);
    lcd_.print("p1:");
    lcd_.setCursor(PH_VALUE_COL, ROW_TITLE);
    snprintf(buffer, sizeof(buffer), "%6.2f", config.phValues[0]);
    lcd_.print(buffer);

    lcd_.setCursor(PH_LABEL_COL, ROW_1);
    lcd_.print("p2:");
    lcd_.setCursor(PH_VALUE_COL, ROW_1);
    snprintf(buffer, sizeof(buffer), "%6.2f", config.phValues[1]);
    lcd_.print(buffer);

    lcd_.setCursor(PH_LABEL_COL, ROW_2);
    lcd_.print("p3:");
    lcd_.setCursor(PH_VALUE_COL, ROW_2);
    snprintf(buffer, sizeof(buffer), "%6.2f", config.phValues[2]);
    lcd_.print(buffer);

    // ---------- ORP ----------
    lcd_.setCursor(ORP_LABEL_COL, ROW_TITLE);
    lcd_.print("o1:");
    lcd_.setCursor(ORP_VALUE_COL, ROW_TITLE);
    snprintf(buffer, sizeof(buffer), "%6.0f", config.orpValues[0]);
    lcd_.print(buffer);

    lcd_.setCursor(ORP_LABEL_COL, ROW_1);
    lcd_.print("o2:");
    lcd_.setCursor(ORP_VALUE_COL, ROW_1);
    snprintf(buffer, sizeof(buffer), "%6.0f", config.orpValues[1]);
    lcd_.print(buffer);

    lcd_.setCursor(ORP_LABEL_COL, ROW_2);
    lcd_.print("o3:");
    lcd_.setCursor(ORP_VALUE_COL, ROW_2);
    snprintf(buffer, sizeof(buffer), "%6.0f", config.orpValues[2]);
    lcd_.print(buffer);
}