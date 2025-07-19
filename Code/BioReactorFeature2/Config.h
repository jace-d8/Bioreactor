//
// Created by Jace Dunn on 7/14/25.
//
#ifndef CONFIG_H
#define CONFIG_H
#include <Wire.h>
#include <Ezo_i2c.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <SD.h>
#include "Valve.h"
#define UTC_OFFSET (-7 * 3600)  // For Pacific Time (PST). Use 0 for UTC, adjust as needed


// GPIO PINS
// Analog stick
const int SW_PIN = D2; // D2: input for detecting whether the jotstick/button is pressed
const int Y_PIN = A1; // A1: analog pin connected to Y output 

const int PH_VALVE_PIN = 3;
const int ORP_VALVE_PIN = 4; 

const int PH_MINIMUM = 6.3;
const int ORP_MINIMUM = -163;

const int SD_CHIP_SELECT = 10;

bool disableValves = false; 

File dataFile;

// these names were changed ^^

enum struct LCD_POS
{
  ROW_ONE = 1,
  // ... think about this
}


LiquidCrystal_I2C lcd(0x27, 20, 4);
char response[32];              // Buffer for response
String inputString = "";  // For manual commands

// Intervals
unsigned long lastReadTime = 0;
const unsigned long readInterval = 2000;  // 2 seconds
unsigned long lastHourTime = 0;
const unsigned long hourInterval = 3600000UL;  // 1 hour = 3,600,000 ms
unsigned long sDread = 0;
unsigned long pHread = 0;
unsigned long eLread = 0;
const unsigned long cooldownPeriod = 60000UL;

int phValveActivationCount = 0;
const int phValveMaxActivations = 5;
unsigned long lastPHactivation = 0;
const unsigned long phResetWindow = 15 * 60 * 1000UL; // 15 minutes to reset count




bool isCleared = false;
bool blinkState = true;
unsigned long lastBlinkTime = 0;
const unsigned long blinkInterval = 300;

