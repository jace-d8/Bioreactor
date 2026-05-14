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

  const uint8_t LS_A_PINS[3] = {8, 13, A1};
  const uint8_t LS_B_PINS[3] = {9, A0, A2};
}

namespace McpPins
{
  const int FV[3]   = {6, 7, 8};
  const int WV0     = 9;
  const int WV[3]   = {10, 11, 12};
  const int FIRST_REACTOR_PIN = 6;
  const int LAST_REACTOR_PIN  = 12;
}

namespace Thresholds
{
  const float PH_MINIMUM = 6.3f;
  const int ORP_MINIMUM = -400;
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

  const unsigned long EZO_READ_INTERVAL = 900UL;
  const unsigned long QUEUE_TIMER = 12000UL;
  const unsigned long VALVE_TIMER = 10000UL;
}

namespace ReactorTimings
{
  constexpr unsigned long LS_A_CONFIRM_MS  = 500UL;
  constexpr unsigned long LS_B_CONFIRM_MS  = 1500UL;
  constexpr unsigned long RECIRC_HOLD_MS   = 60UL * 60UL * 1000UL;
  constexpr unsigned long FILL_TIMEOUT_MS  = 5000UL;
  constexpr unsigned long DRAIN_TIMEOUT_MS = 10000UL;
}

namespace ValveMappings
{
  const int TOTAL_VALVES = 6;
}

namespace ReactorMappings
{
  const int NUM_REACTORS = 3;
}

enum class ReactorState : uint8_t
{
  Recirculating,
  WaitingForFill,
  Filling,
  Holding,
  WaitingForDrain,
  Draining,
  ErrorLsA,
  ErrorLsB
};

struct ConfigState
{
  bool valvesDisabled = false;
  bool lcdCleared = false;
  bool blinkState = true;
  bool requestResetErrors = false;
  bool requestResetLsErrors = false;
  bool reactorAutoEnabled = true;

  bool valveErrorLocked[ValveMappings::TOTAL_VALVES] = {};

  float phValues[3] = {0.0f, 0.0f, 0.0f};
  float orpValues[3] = {};

  bool phValid[3] = {};
  bool orpValid[3] = {};

  bool lsA[ReactorMappings::NUM_REACTORS] = {};
  bool lsB[ReactorMappings::NUM_REACTORS] = {};
  bool lsAError[ReactorMappings::NUM_REACTORS] = {};
  bool lsBError[ReactorMappings::NUM_REACTORS] = {};
  ReactorState reactorState[ReactorMappings::NUM_REACTORS] = {
    ReactorState::Recirculating,
    ReactorState::Recirculating,
    ReactorState::Recirculating
  };
  bool fvOpen[ReactorMappings::NUM_REACTORS] = {};
  bool wvOpen[ReactorMappings::NUM_REACTORS] = {};
  bool wv0Open = true;
};
