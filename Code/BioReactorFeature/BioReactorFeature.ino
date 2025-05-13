#include <Adafruit_ADS1X15.h>
#include <LiquidCrystal_I2C.h>
#define LIQUID_SENSOR_PIN 52
#define GAS_SENSOR_PIN 2 
#define VALVE_PIN 9 
#define PH_SENSOR A1
#define ADC_CONV_FACTOR (4.096 / 32768.0) // ADS1115 GAIN_ONE

Adafruit_ADS1115 ads; 
LiquidCrystal_I2C lcd(0x27, 16, 2);

float ph1 = 4.00, v1 = 1.715;
float ph2 = 7.00, v2 = 1.951;
float ph3 = 10.00, v3 = 2.187;
float m, b;

// int pHsensor = A1;
float buffer_arr[20], temp;
float avgval;
float ph_act;
float volt = 0;
float valveStartTime = 0;
unsigned long lastPrint = 0;
const unsigned long interval = 2000; // milliseconds
int counter = 0; // Count of liquid contact (in seconds)
bool is_gas_sensor = false;
bool is_liquid_detected = false; 
bool valveOpen = false;

void calculateSlope()
{
    // Calculate slope (m) and intercept (b) using least squares
  float x_mean = (v1 + v2 + v3) / 3.0;
  float y_mean = (ph1 + ph2 + ph3) / 3.0;

  float numerator = (v1 - x_mean)*(ph1 - y_mean) +
                    (v2 - x_mean)*(ph2 - y_mean) +
                    (v3 - x_mean)*(ph3 - y_mean);

  float denominator = (v1 - x_mean)*(v1 - x_mean) +
                      (v2 - x_mean)*(v2 - x_mean) +
                      (v3 - x_mean)*(v3 - x_mean);

  m = numerator / denominator;
  b = y_mean - m * x_mean;   
}

void setup() 
{
  ads.begin();                 
  ads.setGain(GAIN_ONE);      
  lcd.begin(16, 2);
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("pH Meter Ready");
  delay(2000);
  lcd.clear();
  calculateSlope();

  pinMode(LIQUID_SENSOR_PIN, INPUT);
  pinMode(GAS_SENSOR_PIN, INPUT); // Liquid detection sensor in U-tubd
  pinMode(VALVE_PIN, OUTPUT); // Controls state of 3 way valve
  Serial.begin(9600);
}

void loop() 
{
  is_liquid_detected = digitalRead(LIQUID_SENSOR_PIN); // will become true if sensor detects liquid 
  is_gas_sensor = digitalRead(GAS_SENSOR_PIN); // is_open will be true if high voltage is read

  measurePH();
  printData(ph_act, volt);

  gasCounter(is_gas_sensor);
  liquidLevelCheck(is_liquid_detected);
}


void measurePH()
{
    // Sample and calculate pH every 1000ms
  if (millis() - lastPrint >= interval) 
  {
    lastPrint = millis();


    // Read, Sort, Filter, Avg 
    for (int i = 0; i < 20; i++) // Get 20 readings
    {
      // buffer_arr[i] = ads.readADC_SingleEnded(0); 
      buffer_arr[i] = (float)ads.readADC_Differential_0_1(); // need y split for differential
      delay(30); // wait between readings 
    }
    // Sort buffer for median filtering
    for (int i = 0; i < 19; i++)
    {
      for (int j = i + 1; j < 20; j++) 
      {
        if (buffer_arr[i] > buffer_arr[j]) 
        {
          temp = buffer_arr[i];
          buffer_arr[i] = buffer_arr[j];
          buffer_arr[j] = temp;
        }
      }
    }
    avgval = 0;
    for (int i = 4; i < 16; i++) // find the avg, discard outliers
    {
      avgval += buffer_arr[i];
    }
    avgval = avgval/12;
    volt = avgval * ADC_CONV_FACTOR; // for GAIN_ONE
    ph_act = m * volt + b; 
  }
}

void liquidLevelCheck(bool is_liquid_detected)
{
  if(is_liquid_detected)
  {
    Serial.println("Liquid detected");
  }else
  {
    Serial.println("No liquid detected");
  }
}

void gasCounter(bool is_gas_sensor)
{
  if (!is_gas_sensor && !valveOpen)
  {
    counter++;
    Serial.println(counter);
    digitalWrite(VALVE_PIN, HIGH);
    valveStartTime = millis();
    valveOpen = true;
  }

  if (valveOpen && millis() - valveStartTime >= interval) // maybe replace millis - valve with (lastprint)
  {
    digitalWrite(VALVE_PIN, LOW);
    valveOpen = false;
  }
}

void printData(float ph_act, float volt)
{
    Serial.print("pH Val: ");
    Serial.println(ph_act, 2);
    Serial.print("Volt Val: ");
    Serial.println(volt, 4);
  
    lcd.setCursor(0, 0);
    lcd.print("pH: ");
    lcd.print(ph_act, 2);
    lcd.print("     "); // Clear leftover digits

    lcd.setCursor(0, 1);
    lcd.print("V: ");
    lcd.print(volt, 3);  // Show voltage to 3 decimal places
    lcd.print("     ");

    lcd.setCursor(0, 2);
    lcd.print("Count: ");
    lcd.print(counter);  
    lcd.print("     ");
}