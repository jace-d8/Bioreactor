#pragma once

const int SW_PIN = D2;
const int Y_PIN = A1;
const int PH_VALVE_PIN = 3;
const int ORP_VALVE_PIN = 4; 
const int PH_MINIMUM = 6.3;
const int ORP_MINIMUM = -163;
const int SD_CHIP_SELECT = 10;
const unsigned long readInterval = 2000;
const unsigned long hourInterval = 3600000UL;
const unsigned long cooldownPeriod = 60000UL;
const int phValveMaxActivations = 5;
const unsigned long phResetWindow = 15 * 60 * 1000UL;
const unsigned long blinkInterval = 300;

// Adjust variables in config.cpp. // Consider getting rid of all these globals 
extern bool disableValves;
extern String inputString;
extern unsigned long lastReadTime;
extern unsigned long lastHourTime;
extern unsigned long sDread;
extern unsigned long pHread;
extern unsigned long eLread;
extern int phValveActivationCount;
extern unsigned long lastPHactivation;
extern bool isCleared;
extern bool blinkState;
extern unsigned long lastBlinkTime;

extern float ph_val; 
extern int orp_val; 