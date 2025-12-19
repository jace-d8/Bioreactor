#pragma once
#include <Arduino.h>


namespace PinConfigurations
{
  const int PIN_SW = 5;  // D2 or 5
  const int PIN_Y = 2;  // A1 or 2
  const int PH_VALVE_PIN = 10; // D7 or 10
  const int ORP_VALVE_PIN = 17; // D8 or 17
  const int LCD_PIN_SDA = 6; // D3 or 6
  const int LCD_PIN_SCL = 7; // D4 or 7
  const int SD_CHIP_SELECT = 21; // D10 or 21
}


namespace Thresholds
{
  const float PH_MINIMUM =  6.3f; // changed ph min 
  const int ORP_MINIMUM = -163; 
}

namespace TimingIntervals // Counted in milliseconds; anything larger than 32,767 MUST be followed by UL
{
  const unsigned long PROBE_READ_INTERVAL = 2000UL; // 2 seconds
  const unsigned long SD_LOG_INTERVAL = 2 * 60 * 1000UL; // 2 * 60 seconds/minute * 1000 milliseconds/second
  const unsigned long VALVE_COOLDOWN = 1 * 70 * 1000UL; // TEMP FIX, added 10 seconds to cooldown to account for activation time
  const int PH_VALVE_MAX_ACTIVATIONS = 3; 
  const int ORP_VALVE_MAX_ACTIVATIONS = 4; 
  const unsigned long PH_RESET_WINDOW = 4 * 60 * 1000UL; 
  const unsigned long ORP_RESET_WINDOW = 4 * 60 * 1000UL; 
  const unsigned long BLINK_INTERVAL = 300UL;
  const unsigned long EZO_READ_INTERVAL = 900UL;
  const unsigned long QUEUE_TIMER = 12000UL;
  const unsigned long VALVE_TIMER = 10000UL;
}



struct ConfigState  // this can go completely at some point 
{
  // String serialInput = "";
  int phValveActivationCount = 0;
  int orpValveActivationCount = 0;
  bool valvesDisabled = false;
  bool lcdCleared = false;
  bool blinkState = true;
  float phValues[3] = {0.0f, 0.0f, 0.0f};
  float orpValues[3] = {0, 0, 0};
  float phValue = 0.0f;
  float orpValue = 0; 
};
