#pragma once
#include <Arduino.h>


namespace PinConfigurations
{
  const int PIN_SW = 5;            // D2
  const int PIN_JOYSTICK_Y = 2;    // A1
  const int PH_VALVE_PIN = 3;
  const int ORP_VALVE_PIN = 4; 
}


namespace Thresholds
{
  const int PH_MINIMUM = 6.3;
  const int ORP_MINIMUM = -163;
}

namespace TimingIntervals // Counted in milliseconds; anything larger than 32,767 MUST be followed by UL
{
  const unsigned long PROBE_READ_INTERVAL = 2000UL;
  const unsigned long SD_LOG_INTERVAL = 120000UL;
  const unsigned long HOUR_INTERVAL = 3600000UL;
  const unsigned long VALVE_COOLDOWN = 60000UL;
  const int PH_VALVE_MAX_ACTIVATIONS = 5; 
  const unsigned long PH_RESET_WINDOW = 15 * 60 * 1000UL;
  const unsigned long BLINK_INTERVAL = 300UL;
}

// SD
const int SD_CHIP_SELECT = 10;

struct ConfigState 
{
  bool valvesDisabled = false;
  String serialInput = "";
  int phValveActivationCount = 0;
  bool lcdCleared = false;
  bool blinkState = true;
  unsigned long lastBlinkTime = 0;
  float phValue = 0.0f;
  int orpValue = 0;
};
