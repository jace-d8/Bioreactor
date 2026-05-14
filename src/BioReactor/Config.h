#pragma once
#include <Arduino.h>

namespace PinConfigurations
{
  const int PIN_SW = 5;
  const int PIN_Y = 2;
  const int PH_VALVE_PIN = 10;
  const int ORP_VALVE_PIN = 17;
  const int LCD_PIN_SDA = 6;
  const int LCD_PIN_SCL = 7;
  const int SD_CHIP_SELECT = 21;
}

namespace Thresholds
{
  const float PH_MINIMUM = 6.3f;
  const int ORP_MINIMUM = -400;
}

namespace BusSpeeds
{
  const uint32_t I2C_HZ = 100000UL;
  const uint32_t SD_SPI_HZ = 1000000UL;
}

namespace JoystickConfig
{
  const int ANALOG_CENTER = 1980;
  const int ANALOG_DEADZONE = 400;
  const unsigned long SCROLL_DELAY_MS = 200UL;
  const unsigned long SWITCH_DEBOUNCE_MS = 300UL;
}

namespace EzoAddresses
{
  const int PH[3]  = {99, 101, 103};
  const int ORP[3] = {98, 100, 102};
}

namespace TimingIntervals
{
  const unsigned long PROBE_READ_INTERVAL = 2000UL;
  const unsigned long SD_LOG_INTERVAL = 2000UL;

  const unsigned long VALVE_COOLDOWN = 60UL * 1000UL;
  const unsigned long VALVE_ERROR_WINDOW = 60UL * 60UL * 1000UL;

  const int PH_VALVE_TRIGGER_LIMIT = 10;
  const int ORP_VALVE_TRIGGER_LIMIT = 20;
  const int MAX_VALVE_ERROR_HISTORY = 20;

  const unsigned long BLINK_INTERVAL = 300UL;
  const unsigned long UI_RENDER_INTERVAL = 80UL;

  const unsigned long EZO_READ_INTERVAL = 900UL;
  const unsigned long QUEUE_TIMER = 12000UL;
  const unsigned long VALVE_TIMER = 10000UL;
}

namespace ValveMappings
{
  const int TOTAL_VALVES = 6;
}

struct ConfigState
{
  bool valvesDisabled = false;
  bool lcdCleared = false;
  bool blinkState = true;
  bool requestResetErrors = false;

  bool valveErrorLocked[ValveMappings::TOTAL_VALVES] = {};

  float phValues[3] = {0.0f, 0.0f, 0.0f};
  float orpValues[3] = {};

  bool phValid[3] = {};
  bool orpValid[3] = {};
};