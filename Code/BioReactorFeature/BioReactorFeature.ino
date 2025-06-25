#include <Wire.h>
#include <Ezo_i2c.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <SD.h>

const int chipSelect = 10;

Ezo_board orp_sensor(98, "ORP");  
Ezo_board ph_sensor(99, "PH");  
LiquidCrystal_I2C lcd(0x27, 20, 4);
char response[32];              // Buffer for response
String inputString = "";  // For manual commands

unsigned long lastReadTime = 0;
const unsigned long readInterval = 2000;  // 2 seconds

// analog stick
const int SW_pin = 5; // D2: input for detecting whether the jotstick/button is pressed
const int Y_pin = 2; // A1: analog pin connected to Y output 


bool blinkState = true;
unsigned long lastBlinkTime = 0;
const unsigned long blinkInterval = 300;

// Note: Pinout for nano eps32 requires pin number changes

void setup()
{
  Serial.begin(9600);
  Wire.setClock(400000);  // Set I2C speed to 400kHz

  pinMode(SW_pin, INPUT); // Setup SW input
  // pinMode(LIQUID_SENSOR_PIN, INPUT);  
  // pinMode(GAS_SENSOR_PIN, INPUT); // Liquid detection sensor in U-tube
  // pinMode(VALVE_PIN, OUTPUT); // Controls state of 3 way valve

  digitalWrite(SW_pin, HIGH);  // Reading button state: 1 = not pressed, 0 = pressed
  delay(100);

  // SD Card Test
  // while (!Serial) {} // Waits for serial connection to open 
  // if (!SD.begin(chipSelect)) { // if 
  //   Serial.println("Initialization failed!");
  //   return;
  // }
  // SD Card end

  Wire.begin();       
  //lcd.begin(20, 4);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("pH Meter Ready");
  delay(2000);
  lcd.clear();
}

void loop() 
{

  // is_liquid_detected = digitalRead(LIQUID_SENSOR_PIN); // will become true if sensor detects liquid 
  // is_gas_sensor = digitalRead(GAS_SENSOR_PIN); // is_open will be true if high voltage is read
  lcdMenu();
  if (millis() - lastReadTime >= readInterval) {

    float ph_val = readPH();  // Convert char* to float
    float orp_val = readORP();
    printData(ph_val, orp_val);
    lastReadTime = millis();
  }
}
float readPH()
{
  ph_sensor.send_cmd("R");         // Send read command
  delay(900);                      // Wait for the sensor to respond
  ph_sensor.receive_cmd(response, 32);  // Read response into buffer
  return atof(response);
}

float readORP()
{
  orp_sensor.send_cmd("R");
  delay(900);
  orp_sensor.receive_cmd(response, 32);
  return atof(response);
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

      lcd.setCursor(0, 0);
      lcd.print("Calibrate Probe:");

      printMenu(selectedItem);
      isPressed(calibrateMenu, selectedItem);
    }
  }
}

void printMenu(int selectedItem)
{
    printMenuItem(0, 1, "pH 1", 0, selectedItem);
    printMenuItem(0, 2, "pH 2", 1, selectedItem);
    printMenuItem(0, 3, "pH 3", 2, selectedItem);
    printMenuItem(6, 1, "ORP 1", 3, selectedItem);
    printMenuItem(6, 2, "ORP 2", 4, selectedItem);
    printMenuItem(6, 3, "ORP 3", 5, selectedItem);
    printMenuItem(13, 2, "Done", 6, selectedItem);
}

void isPressed(bool &calibrateMenu, int selectedItem)
{
  if (!digitalRead(SW_pin)) 
  {
    delay(200);  // debounce
    while (!digitalRead(SW_pin));  // wait until released
    if (selectedItem == 0)
    {
      calibrateProbe();
    } else if (selectedItem == 6) 
    {
      calibrateMenu = false;
      lcd.clear();
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
    printMenuItem(10, 2, "Done", 4, bufferSelection);
    // lcd.setCursor(10, 3);
    // lcd.print("pH: ");
    // lcd.print(ph_val, 3); // to the thousandths


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
          lcd.print("Returning");
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

void printData(float ph_val, float orp_val)
{
    lcd.setCursor(0, 0);
    lcd.print("pH: ");
    lcd.print(ph_val, 3); // To the thousandths
    lcd.print("     "); // Clear leftover digits

    lcd.setCursor(0, 1);
    lcd.print("ORP: ");
    lcd.print(orp_val, 0);
    lcd.print(" mV     ");

    // lcd.setCursor(0, 2);
    // lcd.print("Count: ");
    // lcd.print(counter);  
    // lcd.print("     ");
}