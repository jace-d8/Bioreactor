#include "Lcd.h"
#include "Timer.h"

LiquidCrystal_I2C lcd(0x27, 20, 4);
Timer blinkTimer(TimingIntervals::BLINK_INTERVAL);

MenuItem menuChoices[] = {
    { COL_LEFT,  ROW_1, "pH 1",  MENU_PH1 },
    { COL_LEFT,  ROW_2, "pH 2",  MENU_PH2 },
    { COL_LEFT,  ROW_3, "pH 3",  MENU_PH3 },
    { COL_MID,   ROW_1, "ORP 1", MENU_ORP1 },
    { COL_MID,   ROW_2, "ORP 2", MENU_ORP2 },
    { COL_MID,   ROW_3, "ORP 3", MENU_ORP3 }
};

MenuItem calMenuChoices[] = {
    { COL_LEFT,     ROW_1, "pH 4",  0 },
    { COL_LEFT,     ROW_2, "pH 7",  1 },
    { COL_LEFT,     ROW_3, "pH 10", 2 },
    { COL_RIGHT,    ROW_1, "Clear", 3 },
    { COL_RIGHT,    ROW_2, "Done",  4 }
};

void initLcd()
{
  lcd.init();
  lcd.backlight();
  lcd.setCursor(COL_LEFT, ROW_TITLE);
  lcd.print("LCD initialized; Reading Probes");
}

void toggleMenu(ConfigState& config)
{
  // Placeholder for menu logic
}

void printLcdMenu(ConfigState& config, int selectedItem)
{
  lcd.setCursor(COL_LEFT, ROW_TITLE);
  lcd.print("Calibrate Probe:");

  printMenuItem(COL_LEFT,  ROW_1, "pH 1",  MENU_PH1, selectedItem);
  printMenuItem(COL_LEFT,  ROW_2, "pH 2",  MENU_PH2, selectedItem);
  printMenuItem(COL_LEFT,  ROW_3, "pH 3",  MENU_PH3, selectedItem);
  printMenuItem(COL_MID,   ROW_1, "ORP 1", MENU_ORP1, selectedItem);
  printMenuItem(COL_MID,   ROW_2, "ORP 2", MENU_ORP2, selectedItem);
  printMenuItem(COL_MID,   ROW_3, "ORP 3", MENU_ORP3, selectedItem);

  if (config.valvesDisabled)
    printMenuItem(COL_FAR_RIGHT, ROW_1, "Vl ON", MENU_VALVE_TOGGLE, selectedItem);
  else
    printMenuItem(COL_FAR_RIGHT, ROW_1, "Vl OFF", MENU_VALVE_TOGGLE, selectedItem);

  printMenuItem(COL_FAR_RIGHT, ROW_2, "Done", MENU_DONE, selectedItem);
}

void updateGlobalBlink(ConfigState& config)
{
  if (blinkTimer.isReady()) config.blinkState = !config.blinkState;
}

void printMenuItem(int col, int row, const char* label, int itemIndex, int selectedIndex)
{
  lcd.setCursor(col, row);
  if (itemIndex == selectedIndex)
    lcd.print("      ");
  else
    lcd.print(label);
}

void printData(ConfigState& config)
{
  lcd.setCursor(COL_LEFT, ROW_TITLE);
  lcd.print("pH: ");
  lcd.print(config.phValue, 3);   // To 3 decimal plaxes 
  lcd.print("     ");
  lcd.setCursor(COL_LEFT, ROW_1);
  lcd.print("ORP: ");
  lcd.print(config.orpValue, 0);
  lcd.print(" mV     ");
}

void displayWarning(ConfigState& config)
{
  updateGlobalBlink(config);
  lcd.setCursor(COL_LEFT, ROW_TITLE);
  lcd.print("WARNING:");
  lcd.setCursor(COL_LEFT, ROW_1);
  lcd.print("pH buffer not reacting");
  printMenuItem(COL_LEFT, ROW_2, "Unlock System?", MENU_PH1, MENU_PH1);
}
