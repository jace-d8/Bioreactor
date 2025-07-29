#include "Lcd.h"
#include "Timer.h"


LiquidCrystal_I2C lcd(0x27, 20, 4);
Timer blinkTimer(TimingIntervals::BLINK_INTERVAL);

void initLcd()
{
  lcd.init();
  lcd.backlight();
  lcd.setCursor(COL_LEFT, ROW_TITLE);
  lcd.print("LCD initialized; Reading Probes");
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
  lcd.print("     ");
  lcd.setCursor(COL_LEFT, ROW_1);
  lcd.print("ORP: ");
  lcd.print(config.orpValue, 0);
  lcd.print(" mV     ");
}


// Consider this for non blocking messages... 
struct MessageState {
    bool active = false;
    String message;
    Timer timer = Timer(0);
    bool clearAfter = true;
};

static MessageState messageState;

void showMessage(const String& message, unsigned long durationMs, bool clearAfter)
{
    lcd.clear();
    lcd.print(message);
    messageState.message = message;
    messageState.timer = Timer(durationMs);
    messageState.active = true;
    messageState.clearAfter = clearAfter;
}

void updateShowMessage() 
{
    if (messageState.active && messageState.clearAfter && messageState.timer.isReady()) 
    {
        lcd.clear();
        messageState.active = false;
    }
}

