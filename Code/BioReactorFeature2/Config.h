#pragma once
#include <Arduino.h>

// Pin Config
const int PIN_SW = 5;            // D2
const int PIN_JOYSTICK_Y = 2;    // A1
const int PH_VALVE_PIN = 3;
const int ORP_VALVE_PIN = 4; 

// Thresholds
const int PH_MINIMUM = 6.3;
const int ORP_MINIMUM = -163;

// SD
const int SD_CHIP_SELECT = 10;

// Timing
const unsigned long SENSOR_READ_INTERVAL = 2000;
const unsigned long HOUR_INTERVAL = 3600000UL;
const unsigned long VALVE_COOLDOWN = 60000UL;
const int PH_VALVE_MAX_ACTIVATIONS = 5;
const unsigned long PH_RESET_WINDOW = 15 * 60 * 1000UL;
const unsigned long BLINK_INTERVAL = 300;

// Globals (consider removing these later), encapuslate in struct or something
extern bool valvesDisabled;
extern String serialInput;
extern unsigned long lastSensorReadTime;
extern unsigned long lastHourTrigger;
extern unsigned long lastSdLogTime;
extern unsigned long lastPhValveTime;
extern unsigned long lastOrpValveTime;
extern int phValveActivationCount;
extern unsigned long lastPhActivation;
extern bool lcdCleared;
extern bool blinkState;
extern unsigned long lastBlinkTime;

extern float phValue; 
extern int orpValue;
