#include "Lcd.h"

void toggleMenu() // the if(!digitalRead(SW_pin)) needs to become external to the function 
{
  bool calibrateMenu = false;
  int selectedItem = -1;  // 0 = pH Probe 1, 1 = pH Probe 2...
  if(!digitalRead(SW_pin))
  {
    delay(200);  // debounce
    calibrateMenu = true; 
    selectedItem = 0;
    lcd.clear();
    while(calibrateMenu)
    {
      analogControl(selectedItem);
      updateGlobalBlink();

      printMenu(selectedItem);
      isPressed(calibrateMenu, selectedItem);
    }
  }
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
    if(disableValves)
    {
      printMenuItem(13, 1, "Vl ON", 6, selectedItem);
    }else
    {
      printMenuItem(13, 1, "Vl OFF", 6, selectedItem);
    }
    printMenuItem(13, 2, "Done", 7, selectedItem);
}

void updateGlobalBlink() 
{
  unsigned long currentMillis = millis();
  if (currentMillis - lastBlinkTime >= blinkInterval)
  {
    blinkState = !blinkState;
    lastBlinkTime = currentMillis;
  }
}

void printMenuItem(int col, int row, const char* label, int itemIndex, int selectedIndex) 
{
  lcd.setCursor(col, row);
  if (itemIndex == selectedIndex && blinkState) {
    lcd.print("      ");  // Blank line to simulate blinking
  } else {
    lcd.print(label);
  }
}



