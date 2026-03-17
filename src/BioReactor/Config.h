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
  const float PH_MINIMUM =  6.3f; // 6.3 chosen at Kuang's advice. pH for rumen is 5.5-7.0 pH for anaerobic digesters is 6.5 to 7.5, sometimes lower.
  const int ORP_MINIMUM = -400; // -400 chosen as default baseline. Rumen varies by region, reactors vary as well. We want +25 mV above anaerobic baseline. 
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
