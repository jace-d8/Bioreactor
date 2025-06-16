const int valvePin = 2; 

void setup() {
  pinMode(valvePin, OUTPUT);
  digitalWrite(valvePin, LOW);  // start with valve off
}

void loop() {
  digitalWrite(valvePin, HIGH);  // turn valve ON
  delay(30000);                  // wait 30 seconds
  digitalWrite(valvePin, LOW);   // turn valve OFF
  delay(30000);                  // wait 30 seconds
}