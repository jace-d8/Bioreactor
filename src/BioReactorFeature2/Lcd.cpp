#include "Lcd.h"
#include "Timer.h"


LiquidCrystal_I2C lcd(0x27, 20, 4);
Timer blinkTimer(TimingIntervals::BLINK_INTERVAL);

void initLcd()
{
  lcd.init();
  lcd.backlight();
  lcd.setCursor(COL_LEFT, ROW_TITLE);
  lcd.print("LCD initialized");
}

void updateGlobalBlink(ConfigState& config)
{
  if (blinkTimer.isReady()) config.blinkState = !config.blinkState;
}

void printData(ConfigState& config)
{
  lcd.setCursor(COL_LEFT, ROW_TITLE);
  lcd.print("pH: ");
  lcd.print(config.phValue, 3);   // To 3 decimal plaxes 
  lcd.print("     "); // remove 
  lcd.setCursor(COL_LEFT, ROW_1);
  lcd.print("ORP: ");
  lcd.print(config.orpValue, 0);
  lcd.print(" mV     ");
}





