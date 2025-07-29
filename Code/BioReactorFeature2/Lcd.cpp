#include "Lcd.h"
#include "Config.h"

LiquidCrystal_I2C lcd(0x27, 20, 4);

MenuItem menuChoices[] = {
    { COL_LEFT, ROW_1, "pH 1",  0 },
    { COL_LEFT, ROW_2, "pH 2",  1 },
    { COL_LEFT, ROW_3, "pH 3",  2 },
    { COL_MID,  ROW_1, "ORP 1", 3 },
    { COL_MID,  ROW_2, "ORP 2", 4 },
    { COL_MID,  ROW_3, "ORP 3", 5 }
};

MenuItem calMenuChoices[] = {
    { COL_LEFT,  ROW_1, "pH 4",  0 },
    { COL_LEFT,  ROW_2, "pH 7",  1 },
    { COL_LEFT,  ROW_3, "pH 10", 2 },
    { COL_RIGHT, ROW_1, "Clear", 3 },
    { COL_RIGHT, ROW_2, "Done",  4 }
};

void initLcd()
{
  lcd.init();
  lcd.backlight();
  lcd.setCursor(COL_LEFT, ROW_TITLE);
  lcd.print("LCD initialized; Reading Probes");
}

void toggleMenu()
{
  // Implementation TBD (existing logic kept unchanged)
}

void printLcdMenu(int selectedItem)
{
  lcd.setCursor(COL_LEFT, ROW_TITLE);
  lcd.print("Calibrate Probe:");
  printMenuItem(0, 1, "pH 1", 0, selectedItem);
  printMenuItem(0, 2, "pH 2", 1, selectedItem);
  printMenuItem(0, 3, "pH 3", 2, selectedItem);
  printMenuItem(6, 1, "ORP 1", 3, selectedItem);
  printMenuItem(6, 2, "ORP 2", 4, selectedItem);
  printMenuItem(6, 3, "ORP 3", 5, selectedItem);
  if (valvesDisabled)
    printMenuItem(13, 1, "Vl ON", 6, selectedItem);
  else
    printMenuItem(13, 1, "Vl OFF", 6, selectedItem);
  printMenuItem(13, 2, "Done", 7, selectedItem);
}

void updateGlobalBlink()
{
  unsigned long currentMillis = millis();
  if (currentMillis - lastBlinkTime >= BLINK_INTERVAL)
  {
    blinkState = !blinkState;
    lastBlinkTime = currentMillis;
  }
}

void printMenuItem(int col, int row, const char* label, int itemIndex, int selectedIndex)
{
  lcd.setCursor(col, row);
  if (itemIndex == selectedIndex && blinkState)
    lcd.print("      ");
  else
    lcd.print(label);
}
