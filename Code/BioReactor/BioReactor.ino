

// Port definition
#define LIQUID_SENSOR_PIN 52
#define GAS_SENSOR_PIN 2 
#define VALVE_PIN 9 

int counter = 0; // Count of liquid contact (in seconds)
bool is_gas_sensor = false;
bool is_liquid_detected = false; 

void setup() 
{
  // Initialize pin mode
  pinMode(LIQUID_SENSOR_PIN, INPUT);
  pinMode(GAS_SENSOR_PIN, INPUT); // Liquid detection sensor in U-tubd
  pinMode(VALVE_PIN, OUTPUT); // Controls state of 3 way valve
  Serial.begin(9600);
}

void loop() 
{
  is_liquid_detected = digitalRead(LIQUID_SENSOR_PIN); // will become true if sensor detects liquid 
  is_gas_sensor = digitalRead(GAS_SENSOR_PIN); // is_open will be true if high voltage is read

  gasCounter(is_gas_sensor);
  liquidLevelCheck(is_liquid_detected);
}

void liquidLevelCheck(bool is_liquid_detected)
{
  if(is_liquid_detected)
  {
    Serial.println("Liquid detected");
  }else
  {
    Serial.println("Nothing");
  }
}

void gasCounter(bool is_gas_sensor)
{
  if(!is_gas_sensor) // if detection is positive 
  {
    counter++; // increment counter
    Serial.println(counter);
    digitalWrite(VALVE_PIN, HIGH); // open valve
    delay(2000); // for two seconds 
    digitalWrite(VALVE_PIN,LOW); // then close the valve 
  }
}