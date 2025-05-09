// LCD library includes
#include <Wire.h>
#include <Adafruit_MCP23X08.h>
#include <Adafruit_MCP23X17.h>
#include <Adafruit_MCP23XXX.h>
#include <Adafruit_RGBLCDShield.h>

// Port definition
#define GAS_SENSOR_PIN 2 
#define VALVE_PIN 9 

// LCD variable
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

// Other variables
int counter = 0;
bool open_valve = false;

void setup() 
{
  // Initialize pin mode
  pinMode(OPTO_PIN, INPUT);
  pinMode(VALVE_PIN, OUTPUT);
  Serial.begin(9600);
  
  // set up the LCD's number of columns and rows: 
  lcd.begin(16, 2);
  lcd.setBacklight(0x7); 
  lcd.setCursor(0,0);
  lcd.print("Bio-Gas Counter");
  lcd.setCursor(0,1);
  lcd.print(counter);
  Serial.println(counter);

}

void loop() 
{
  // lcd.setCursor(0, 1);
  // Read buttons status
  uint8_t buttons = lcd.readButtons();
  if (buttons & BUTTON_SELECT)
  {
    counter = 0;
    // lcd.print(counter);
    Serial.print(counter);
  }
  if (buttons & BUTTON_LEFT) {
    digitalWrite(VALVE_PIN,HIGH);
  } else {
    digitalWrite(VALVE_PIN,LOW);
  }

  gas_counter();

  if(open_valve)
  {
    digitalWrite(VALVE_PIN,HIGH);
    delay(2000);
    digitalWrite(VALVE_PIN,LOW);
    open_valve = false;
  }
}

void gas_counter()
{
  if(!digitalRead(OPTO_PIN))
  {
    counter++;
    // lcd.print(counter);
    Serial.println(counter);
    open_valve = true;
  }
}
