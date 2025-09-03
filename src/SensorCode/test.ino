#define LIQUID_SENSOR_PIN 5

void setup() 
{
  pinMode(LIQUID_SENSOR_PIN, INPUT); 
  Serial.begin(9600);

}

void loop() 
{
  bool no_liquid = digitalRead(LIQUID_SENSOR_PIN); 
  if(no_liquid)
  {
    Serial.println("No Liquid Detected");
  }else
  {
    Serial.println("Liquid Detected");
  }
  delay(500);
}
// working sensor code ^^^^


