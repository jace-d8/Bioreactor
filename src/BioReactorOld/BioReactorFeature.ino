#include <Wire.h>
#include <Ezo_i2c.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <SD.h>
#define UTC_OFFSET (-7 * 3600)  // For Pacific Time (PST). Use 0 for UTC, adjust as needed
#define PH_MIN 6.3
#define ORP_MIN -163


const int chipSelect = 10;
bool disableValves = false; 

File dataFile;


LiquidCrystal_I2C lcd(0x27, 20, 4);
char response[32];              // Buffer for response
String inputString = "";  // For manual commands

// Intervals
unsigned long lastReadTime = 0;
const unsigned long readInterval = 2000;  // 2 seconds
unsigned long lastHourTime = 0;
const unsigned long hourInterval = 3600000UL;  // 1 hour = 3,600,000 ms
unsigned long SDread = 0;
unsigned long pHread = 0;
unsigned long eLread = 0;
const unsigned long cooldownPeriod = 60000UL;

int phValveActivationCount = 0;
const int phValveMaxActivations = 5;
unsigned long lastPHactivation = 0;
const unsigned long phResetWindow = 15 * 60 * 1000UL; // 15 minutes to reset count

// Analog stick
const int SW_pin = D2; // D2: input for detecting whether the jotstick/button is pressed
const int Y_pin = A1; // A1: analog pin connected to Y output 

class Valve{
private: 
  int pin; 
  bool isOpen; 
  bool enable; 
  unsigned int startTime;
  const unsigned long openDuration; 
public: 
  Valve(int p, unsigned long d) : pin(p), openDuration(d), isOpen(false), enable(true), startTime(0) {}

  void open()
  {
    if(!isOpen && enable)
    {
      digitalWrite(pin, HIGH);
      startTime = millis(); 
      isOpen = true;
    } 
  }
  void update()
  {
    if (isOpen && (millis() - startTime >= openDuration))
    {
      digitalWrite(pin, LOW);
      isOpen = false;
    }
  }
  bool isValveOpen()
  {
    return isOpen;
  }
  void switchValve()
  {
    enable = !enable;
  }
  bool isEnabled()
  {
    return enable; 
  }
};

void logToSD(String message = "");


bool isCleared = false;
bool blinkState = true;
unsigned long lastBlinkTime = 0;
const unsigned long blinkInterval = 300;

// Probes
Ezo_board orp_sensor(98, "ORP");  
Ezo_board ph_sensor(99, "PH");  

// Probe Vals
float ph_val = 0.0; // I want these globally avaliable
int orp_val = 0; 

// Valves
Valve pHvalve(3, 10000);
Valve eLvalve(4, 4000);

// Note: I will consider using classes for further modularization

void setup()
{
  Serial.begin(9600);
  Wire.setClock(1000000);  // Set I2C speed to 100kHz


  pinMode(3, OUTPUT);  // For pHvalve
  pinMode(4, OUTPUT);  // For eLvalve
  
  // pinMode(LIQUID_SENSOR_PIN, INPUT);  
  // pinMode(GAS_SENSOR_PIN, INPUT); // Liquid detection sensor in U-tube
  // pinMode(VALVE_PIN, OUTPUT); // Controls state of 3 way valve

  //pinMode(SW_pin, INPUT); // Setup SW input
  //digitalWrite(SW_pin, HIGH);  // Reading button state: 1 = not pressed, 0 = pressed
  // The above two lines are for normal arduino, the below is for the nano esp32
  pinMode(SW_pin, INPUT_PULLUP);  
  //delay(100);

  Wire.begin();       
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Reading Probes");
  lcd.setCursor(0, 1);
  
  if (SD.begin(chipSelect)) 
  {
    lcd.print("SD initilzed");
    dataFile = SD.open("/data.csv", FILE_WRITE);
    dataFile.println("DATE,PH,ORP");
  }else
  {
    lcd.print("SD failed");
  }
  lcd.setCursor(0, 2);
  configTime(UTC_OFFSET, 0, "");  
  setTimeFromBuild();
}

void loop() 
{
  // is_liquid_detected = digitalRead(LIQUID_SENSOR_PIN); // will become true if sensor detects liquid 
  // is_gas_sensor = digitalRead(GAS_SENSOR_PIN); // is_open will be true if high voltage is read
  if (isCooldownOver(readInterval, lastReadTime)) 
  {
    ph_val = readPH();  // Convert char* to float
    orp_val = readORP();
    if(!isCleared)
    {
      lcd.clear(); // consider optimizing
      isCleared = true;
    }
    printData();   
    lastReadTime = millis();
  }

  if(isCooldownOver(SDread, 6000)) // 2 min, will change later
  {
    logToSD(); 
    SDread = millis(); 
  }
  if ((ph_val < PH_MIN) && isCooldownOver(pHread, cooldownPeriod)) 
  {
    pHvalve.open();
    logToSD("pH valve toggled"); 
    pHread = millis(); 

    // Check time since last activation
    if (millis() - lastPHactivation > phResetWindow) 
    {
      phValveActivationCount = 1;  // reset count
    }else 
    {
      phValveActivationCount++;  // count up
    }
    lastPHactivation = millis();
    if (phValveActivationCount >= phValveMaxActivations) 
    {
      displayWarning();  // trigger user intervention
      phValveActivationCount = 0;  // reset after warning
    }
  }
  if((orp_val < ORP_MIN) && isCooldownOver(eLread, cooldownPeriod)) // COOLDOWN DOES NOT CURRENTLY ACCOUNT FOR 10 SECONDS OF ACTIVATION, 50 second cooldown
  {
    eLvalve.open();
    logToSD("ORP valve toggled"); 
    eLread = millis(); 
  }
  if (isCooldownOver(lastHourTime, hourInterval))  
  {
    eLvalve.open();
    lastHourTime = millis();
  }

  pHvalve.update();
  eLvalve.update(); 
  lcdMenu();
}

bool isCooldownOver(unsigned long lastTime, unsigned long cooldown)
{
 return (millis() - lastTime >= cooldown);
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
    if(disableValves)
    {
      printMenuItem(13, 1, "Vl ON", 6, selectedItem);
    }else
    {
      printMenuItem(13, 1, "Vl OFF", 6, selectedItem);
    }

    printMenuItem(13, 2, "Done", 7, selectedItem);
}

void isPressed(bool &calibrateMenu, int selectedItem) // later to be optimized when multiple probes are utilized
{
  if (!digitalRead(SW_pin)) 
  {
    delay(200);  // debounce
    while (!digitalRead(SW_pin));  // wait until released
    if (selectedItem == 0)
    {
      calibrateProbePH();
    }else if(selectedItem == 3)
    {
      calibrateProbeORP();
    }else if(selectedItem == 6)
    {
      disableValves = !disableValves;
      pHvalve.switchValve();
      eLvalve.switchValve();
      lcd.clear();
      if(disableValves)
      {
        lcd.print("Valves off");
      }else
      {
        lcd.print("Valves on");
      }
      delay(600);
      lcd.clear();
    }else if (selectedItem == 7) 
    {
      calibrateMenu = false;
      lcd.clear();
    }
  }
}

void analogControl(int& selectedItem)
{
  int yVal = analogRead(Y_pin);

  // Dead zone around the resting value (~1980)
  const int deadZone = 400;

  if (yVal < 1980 - deadZone) 
  {  // Up
    if (selectedItem > 0)
      selectedItem--;
    delay(200); // Debounce
  } else if (yVal > 1980 + deadZone) 
  {  // Down
    if (selectedItem < 7)  // 6 is your "Done" item
      selectedItem++;
    delay(200); // Debounce
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

void calibrateProbePH() {
  int bufferSelection = 0;
  bool bufferCalibrated[3] = {false, false, false};
  bool selecting = true;

  lcd.clear();

  while (selecting) {
    analogControl(bufferSelection);
    updateGlobalBlink();

    // ph_val = readPH();   putting this on hold

    lcd.setCursor(0, 0);
    lcd.print("Select Buffer:");
    printMenuItem(0, 1, "pH 4", 0, bufferSelection);
    printMenuItem(0, 2, "pH 7", 1, bufferSelection);
    printMenuItem(0, 3, "pH 10", 2, bufferSelection);
    printMenuItem(10, 1, "Clear", 3, bufferSelection);  
    printMenuItem(10, 2, "Done", 4, bufferSelection);
    lcd.setCursor(10, 3);
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
      delay(800);
      lcd.clear();
      selecting = false;
    }
  }
}

void calibrateProbeORP()
{
  int selectedItem = 0;  // Only one item for now, but this keeps it consistent
  bool selecting = true;
  int lastPrint = 0;
  lcd.clear();

  while (selecting) {
    analogControl(selectedItem);
    updateGlobalBlink();

    // if(isCooldownOver(lastPrint, 4000)) // 4 second cooldown
    // {
    //   orp_val = readORP(); putting this idea on hold
    // }
    lcd.setCursor(0, 0);
    lcd.print("Calibrate when ready");

    printMenuItem(0, 1, "Cal", 0, selectedItem);
    printMenuItem(0, 2, "Done", 1, selectedItem);

    lcd.setCursor(5, 1);

    // lcd.print("ORP: ");
    // lcd.print(orp_val, 0);
    // lcd.print(" mV     ");
    


    if (!digitalRead(SW_pin)) {
      delay(200);  // Debounce
      while (!digitalRead(SW_pin));  // Wait until released
      switch(selectedItem)
      {
        case 0:
          orp_sensor.send_cmd("Cal,222");
          lcd.clear();
          lcd.print("Calibrated at 222mV");
          delay(1000);
          selecting = false;
          lcd.clear();
          break;
        case 1: 
          lcd.clear();
          lcd.print("Returning");
          delay(1000);
          selecting = false;
          lcd.clear();
      }
    }
  }
}

void setTimeFromBuild()
{
  struct tm tm; // std C++ time struct
  if (strptime(__DATE__ " " __TIME__, "%b %d %Y %H:%M:%S", &tm)) // Taking compile time and parsing it for tm 
   {
    time_t t = mktime(&tm); // unix timestamp 
    struct timeval now = { .tv_sec = t }; // std C++ time struct, seconds since 1970, so esp32 can count further
    settimeofday(&now, nullptr); // setting esp32 clock to laptop time

    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&t)); // takes seconds and converts to human readable time
    lcd.print(buf);
  } else 
  {
    lcd.print("Failed to parse build time");
  }
}

void logToSD(String message)
{
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now); // reads our updated esp32 time

  if (!dataFile) 
  {
    lcd.print("File failed\n");
    return;
  }
  if (message.length() == 0) 
  {
    dataFile.printf("%04d-%02d-%02d %02d:%02d:%02d,%.3f,%d\n", // logging time from esp32
                    timeinfo->tm_year + 1900,
                    timeinfo->tm_mon + 1,
                    timeinfo->tm_mday,
                    timeinfo->tm_hour,
                    timeinfo->tm_min,
                    timeinfo->tm_sec,
                    ph_val,
                    orp_val);
    dataFile.flush();
  }
  else
  {
    dataFile.printf("%04d-%02d-%02d %02d:%02d:%02d,%s\n",
                    timeinfo->tm_year + 1900,
                    timeinfo->tm_mon + 1,
                    timeinfo->tm_mday,
                    timeinfo->tm_hour,
                    timeinfo->tm_min,
                    timeinfo->tm_sec,
                    message.c_str());   
  }         
}

void displayWarning()
{
  bool bypass_warning = false; 
  lcd.clear();
  pHvalve.switchValve();
  while(!bypass_warning)
  {
    updateGlobalBlink();

    lcd.setCursor(0, 0);
    lcd.print("WARNING:");
    
    lcd.setCursor(0, 1);
    lcd.print("pH valve overuse");

    printMenuItem(0, 2, "Unlock?", 0, 0);

    if (!digitalRead(SW_pin)) 
    {
      delay(200);
      while (!digitalRead(SW_pin)); 
      bypass_warning = true;
      lcd.clear();
      lcd.print("System Unlocked");
      delay(700);
      lcd.clear();
      pHvalve.switchValve();
    }
  }
}

void printData()
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