#pragma once
#include <Arduino.h>

namespace PinConfigurations
{
  const int PIN_SW = 5;  //Joystick switch pin//
  const int PIN_Y = 2;  //Joystick y-axis pin//
  const int PH_VALVE_PIN = 10;  //pH valve pin//
  const int ORP_VALVE_PIN = 17;  //ORP valve pin// 
  const int LCD_PIN_SDA = 6;  //LCD SDA pin//
  const int LCD_PIN_SCL = 7;  //LCD SCL pin//
  const int SD_CHIP_SELECT = 21;  //SD chip select pin//
}

namespace Thresholds
{
  const float PH_MINIMUM = 6.3f;  //pH minimum threshold/, below which the pH valve will open//
  const int ORP_MINIMUM = -400;  //ORP minimum threshold/, below which the ORP valve will open//
}

namespace BusSpeeds  //Bus speeds for I2C and SPI//
{
  const uint32_t I2C_HZ = 100000UL;  //I2C bus speed//
  const uint32_t SD_SPI_HZ = 1000000UL;  //SPI bus speed for SD card//
}

namespace JoystickConfig  //Joystick configuration//
{
  const int ANALOG_CENTER = 1980;  //Analog center value//
  const int ANALOG_DEADZONE = 400;  //Analog deadzone value, within which the joystick is considered to be in the center position//
  const unsigned long SCROLL_DELAY_MS = 200UL;  //Scroll delay time, to prevent rapid scrolling//
  const unsigned long SWITCH_DEBOUNCE_MS = 300UL;  //Switch debounce time, to prevent rapid button presses//
}

namespace EzoAddresses  //EZO addresses for the 3 probes//
{
  const int PH[3]  = {99, 101, 103};  //pH probe addresses, this needs to be set up for new Atlas boards//
  const int ORP[3] = {98, 100, 102};  //ORP probe addresses, this needs to be set up for new Atlas boards//
}

namespace TimingIntervals
{
  const unsigned long PROBE_READ_INTERVAL = 2000UL;  //*2 seconds between probe reads//
  const unsigned long SD_LOG_INTERVAL = 2000UL;  //*2 seconds between SD card logs//

  const unsigned long VALVE_COOLDOWN = 60UL * 1000UL;  //*60 s cooldown between valve triggers//
  const unsigned long VALVE_ERROR_WINDOW = 60UL * 60UL * 1000UL;  //*1 hour window for error-locking//

  const int PH_VALVE_TRIGGER_LIMIT = 10;  //max 10 valve triggers per pH valve in the last *1 hour//
  const int ORP_VALVE_TRIGGER_LIMIT = 20;  //max 20 valve triggers per ORP valve in the last *1 hour//
  const int MAX_VALVE_ERROR_HISTORY = 20;

  const unsigned long BLINK_INTERVAL = 300UL;  //*300 ms between blink state changes//
  const unsigned long UI_RENDER_INTERVAL = 80UL;  //*80 ms between UI renders//

  const unsigned long EZO_READ_INTERVAL = 900UL;  //*900 ms between probe reads//
  const unsigned long QUEUE_TIMER = 12000UL;  //up to *12 s between dequeuing events (QUEUE_TIMER)//
  const unsigned long VALVE_TIMER = 10000UL;  //valve is opened for *10 seconds (VALVE_TIMER)//
}

namespace ValveMappings  //Mapping of MCP pins to valves//
{
  const int TOTAL_VALVES = 6;
}

struct ConfigState  //Global configuration state//
{
  bool valvesDisabled = false;  //Valves disabled flag//
  bool lcdCleared = false;  //LCD cleared flag//
  bool blinkState = true;  //Blink state flag//
  bool requestResetErrors = false;  //Request reset errors flag//

  bool valveErrorLocked[ValveMappings::TOTAL_VALVES] = {};  //Valve error locked flags//

  float phValues[3] = {0.0f, 0.0f, 0.0f};  //pH values from the 3 probes// 
  float orpValues[3] = {};  //ORP values from the 3 probes//

  bool phValid[3] = {};  //pH valid flags//
  bool orpValid[3] = {};  //ORP valid flags//
};
