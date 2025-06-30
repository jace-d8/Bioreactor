const int valvePin = 2; 
const int valvePin2 = 3; 

void setup() {
  pinMode(valvePin, OUTPUT);
  pinMode(valvePin2, OUTPUT);
  digitalWrite(valvePin, LOW);  // start with valve off
  digitalWrite(valvePin2, LOW);  // start with valve off
}

void loop() {
  digitalWrite(valvePin, HIGH);  // turn valve ON
  digitalWrite(valvePin2, LOW);  // turn valve ON
  delay(30000);                  // wait 30 seconds
  digitalWrite(valvePin, LOW);   // turn valve OFF
  digitalWrite(valvePin2, HIGH);  // turn valve ON
  delay(30000);                  // wait 30 seconds
}