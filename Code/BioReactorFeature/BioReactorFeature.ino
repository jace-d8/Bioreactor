#include <Wire.h>
#include <Ezo_i2c.h>
#include <LiquidCrystal_I2C.h>

Ezo_board ph_sensor(99, "PH");  // Change 99 if your sensor has a different I2C address
LiquidCrystal_I2C lcd(0x27, 20, 4);
char response[32];              // Buffer for response
String inputString = "";  // For manual commands

unsigned long lastReadTime = 0;
const unsigned long readInterval = 2000;  // 2 seconds

// analog stick
const int SW_pin = 53; // input for detecting whether the jotstick/button is pressed
const int Y_pin = A15; // analog pin connected to Y output


bool blinkState = true;
unsigned long lastBlinkTime = 0;
const unsigned long blinkInterval = 300;

void setup()
{
  Serial.begin(9600);
  pinMode(SW_pin, INPUT);      //setup SW input
  digitalWrite(SW_pin, HIGH);  //reading button state:1=not pressed,0=pressed
  delay(100);
  Wire.begin();
  lcd.begin(20, 4);
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("pH Meter Ready");
  delay(2000);
  // pinMode(LIQUID_SENSOR_PIN, INPUT);  
  // pinMode(GAS_SENSOR_PIN, INPUT); // Liquid detection sensor in U-tubd
  // pinMode(VALVE_PIN, OUTPUT); // Controls state of 3 way valve
  lcd.clear();
  Serial.println("Automated pH reading every 2 seconds...");
}

void loop() 
{

  // is_liquid_detected = digitalRead(LIQUID_SENSOR_PIN); // will become true if sensor detects liquid 
  // is_gas_sensor = digitalRead(GAS_SENSOR_PIN); // is_open will be true if high voltage is read
  lcdMenu();
  if (millis() - lastReadTime >= readInterval) {
    ph_sensor.send_cmd("R");         // Send read command
    delay(900);                      // Wait for the sensor to respond
    ph_sensor.receive_cmd(response, 32);  // Read response into buffer
    float ph_value = atof(response);  // Convert char* to float
    printData(ph_value);

    lastReadTime = millis();
  }

    // === Manual command input via Serial ===
  // while (Serial.available()) {
  //   char inChar = (char)Serial.read();
  //   if (inChar == '\n') {
  //     if (inputString.length() > 0) {
  //       Serial.print("Sending command: ");
  //       Serial.println(inputString);

  //       ph_sensor.send_cmd(inputString.c_str());
  //       delay(900);
  //       ph_sensor.receive_cmd(response, 32);

  //       Serial.print("Manual response: ");
  //       Serial.println(response);

  //       inputString = "";
  //     }
  //   } else if (inChar != '\r') {
  //     inputString += inChar;
  //   }
  // }
}


// void liquidLevelCheck(bool is_liquid_detected)
// {
//   if(is_liquid_detected)
//   {
//     Serial.println("Liquid detected");
//   }else
//   {
//     Serial.println("No liquid detected");
//   }
// }

// void gasCounter(bool is_gas_sensor)
// {
//   if (!is_gas_sensor && !valveOpen)
//   {
//     counter++;
//     Serial.println(counter);
//     digitalWrite(VALVE_PIN, HIGH);
//     valveStartTime = millis();
//     valveOpen = true;
//   }

//   if (valveOpen && millis() - valveStartTime >= interval) // maybe replace millis - valve with (lastprint)
//   {
//     digitalWrite(VALVE_PIN, LOW);
//     valveOpen = false;
//   }
// }

void lcdMenu()
{
  bool calibrateMenu = false;
  int selectedItem = -1;  // 0 = pH Probe 1, 1 = pH Probe 2
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

      lcd.setCursor(0, 0);
      lcd.print("Calibrate Probe:");

      printMenuItem(0, 1, "pH 1", 0, selectedItem);
      printMenuItem(0, 2, "pH 2", 1, selectedItem);
      printMenuItem(0, 3, "pH 3", 2, selectedItem);
      printMenuItem(6, 1, "ORP 1", 3, selectedItem);
      printMenuItem(6, 2, "ORP 2", 4, selectedItem);
      printMenuItem(6, 3, "ORP 3", 5, selectedItem);
      printMenuItem(13, 2, "Cancel", 6, selectedItem);

      if (!digitalRead(SW_pin)) {
        delay(200);  // debounce
        while (!digitalRead(SW_pin));  // wait until released
        if (selectedItem == 0) {
          calibrateProbe();
        } else if (selectedItem == 6) {
          calibrateMenu = false;
          lcd.clear();
        }
      }
    }
  }
}

void analogControl(int& selectedItem)
{
  int yVal = analogRead(Y_pin);
  if (yVal < 400)
  {  // Up
    if (selectedItem >= 1)
      selectedItem--;
    delay(200); // Debounce movement
  } else if (yVal > 600)
  {  // Down
    if (selectedItem <= 5)
      selectedItem++;
    delay(200);
  }
}

void updateGlobalBlink() {
  unsigned long currentMillis = millis();
  if (currentMillis - lastBlinkTime >= blinkInterval) {
    blinkState = !blinkState;
    lastBlinkTime = currentMillis;
  }
}

void printMenuItem(int col, int row, const char* label, int itemIndex, int selectedIndex) {
  lcd.setCursor(col, row);
  if (itemIndex == selectedIndex && blinkState) {
    lcd.print("      ");  // Blank line to simulate blinking
  } else {
    lcd.print(label);
  }
}

void calibrateProbe() {
  int bufferSelection = 0;
  bool bufferCalibrated[3] = {false, false, false};
  bool selecting = true;

  lcd.clear();

  while (selecting) {
    analogControl(bufferSelection);
    updateGlobalBlink();

    lcd.setCursor(0, 0);
    lcd.print("Select Buffer:");

    printMenuItem(0, 1, "pH 4", 0, bufferSelection);
    printMenuItem(0, 2, "pH 7", 1, bufferSelection);
    printMenuItem(0, 3, "pH 10", 2, bufferSelection);
    printMenuItem(10, 1, "Clear", 3, bufferSelection);  
    printMenuItem(10, 2, "Cancel", 4, bufferSelection);

    if (bufferCalibrated[0]) lcd.setCursor(6, 1), lcd.print("*");
    if (bufferCalibrated[1]) lcd.setCursor(6, 2), lcd.print("*");
    if (bufferCalibrated[2]) lcd.setCursor(6, 3), lcd.print("*");

    
    if (!digitalRead(SW_pin)) {
      delay(200);
      while (!digitalRead(SW_pin)); 

      switch (bufferSelection) {
        case 0:
          ph_sensor.send_cmd("Cal,low,4.00");
          bufferCalibrated[0] = true;
          break;
        case 1:
          ph_sensor.send_cmd("Cal,mid,7.00");
          bufferCalibrated[1] = true;
          break;
        case 2:
          ph_sensor.send_cmd("Cal,high,10.00");
          bufferCalibrated[2] = true;
          break;
        case 3: 
          ph_sensor.send_cmd("Cal,clear");
          bufferCalibrated[0] = bufferCalibrated[1] = bufferCalibrated[2] = false;
          lcd.clear();
          lcd.print("Calibration Cleared");
          delay(1500);
          lcd.clear();
          break;
        case 4:  
          lcd.clear();
          lcd.print("Cancelled");
          delay(1000);
          lcd.clear();
          selecting = false;
          break;
      }
    }
    if (bufferCalibrated[0] && bufferCalibrated[1] && bufferCalibrated[2]) 
    {
      lcd.clear();
      lcd.print("All Calibrated!");
      delay(1500);
      lcd.clear();
      selecting = false;
    }
  }
}

void printData(float ph_act)
{
  //   float ph_value = atof(ph_str);  // Convert char* to float
    // Serial.print("pH Val: ");
    // Serial.println(ph_act);
  
    lcd.setCursor(0, 0);
    lcd.print("pH: ");
    lcd.print(ph_act);
    lcd.print("     "); // Clear leftover digits

    // lcd.setCursor(0, 2);
    // lcd.print("Count: ");
    // lcd.print(counter);  
    // lcd.print("     ");
}