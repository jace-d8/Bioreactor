#include "Config.h"

bool valvesDisabled = false;
String serialInput = "";

unsigned long lastSensorReadTime = 0;
unsigned long lastHourTrigger = 0;
unsigned long lastSdLogTime = 0;
unsigned long lastPhValveTime = 0;
unsigned long lastOrpValveTime = 0;

int phValveActivationCount = 0;
unsigned long lastPhActivation = 0;

bool lcdCleared = false;
bool blinkState = true;
unsigned long lastBlinkTime = 0;
