#pragma once
#include <Arduino.h>


namespace PinConfigurations
{
  const int PIN_SW = D2;            // 5
  const int PIN_Y = A1;    // 2
  const int PH_VALVE_PIN = 3;
  const int ORP_VALVE_PIN = 4; 
}


namespace Thresholds
{
  const float PH_MINIMUM = 6.3f; 
  const int ORP_MINIMUM = -163; 
}

namespace TimingIntervals // Counted in milliseconds; anything larger than 32,767 MUST be followed by UL
{
  const unsigned long PROBE_READ_INTERVAL = 2000UL; // 2 seconds
  const unsigned long SD_LOG_INTERVAL = 2 * 60 * 1000UL; // 2 * 60 seconds/minute * 1000 milliseconds/second
  const unsigned long HOUR_INTERVAL = 60 * 60 * 1000UL; // 60 minutes * 60 seconds/minute * 1000 milliseconds/second // no longer needed 
  const unsigned long VALVE_COOLDOWN = 1 * 60 * 1000UL;
  const int PH_VALVE_MAX_ACTIVATIONS = 3; 
  const int ORP_VALVE_MAX_ACTIVATIONS = 4; 
  const unsigned long PH_RESET_WINDOW = 4 * 60 * 1000UL; 
  const unsigned long ORP_RESET_WINDOW = 4 * 60 * 1000UL; 
  const unsigned long BLINK_INTERVAL = 300UL;
  const unsigned long EZO_READ_INTERVAL = 900UL;
}

// SD
const int SD_CHIP_SELECT = 10; // put in pin config

struct ConfigState  // this can go completely at some point 
{
  String serialInput = "";
  int phValveActivationCount = 0;
  int orpValveActivationCount = 0;
  bool valvesDisabled = false;
  bool lcdCleared = false;
  bool blinkState = true;
  float phValue = 0.0f;
  float orpValue = 0; 
};
