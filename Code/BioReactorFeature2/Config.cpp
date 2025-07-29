#include "Config.h"

// Define the variables here
bool disableValves = false;
String inputString = "";

unsigned long lastReadTime = 0;
unsigned long lastHourTime = 0;
unsigned long sDread = 0;
unsigned long pHread = 0;
unsigned long eLread = 0;

int phValveActivationCount = 0;
unsigned long lastPHactivation = 0;

bool isCleared = false;
bool blinkState = true;
unsigned long lastBlinkTime = 0;
