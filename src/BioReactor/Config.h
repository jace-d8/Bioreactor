#pragma once
#include <Arduino.h>

namespace PinConfigurations
{
  const int PIN_SW = 5;//Joystick switch pin//
  const int PIN_Y = 2;//Joystick y-axis pin//
  const int PH_VALVE_PIN = 10;//pH valve pin, no longer used since all valves are handled in MCP//
  const int ORP_VALVE_PIN = 17;//ORP valve pin, no longer used since all valves are handled in MCP// 
  const int LCD_PIN_SDA = 6;//LCD SDA pin//
  const int LCD_PIN_SCL = 7;//LCD SCL pin//
  const int SD_CHIP_SELECT = 21;//SD chip select pin//
}

namespace ThresholdDefaults
{
  const float PH_MINIMUM = 4.1f; //normally 6.3, set to 4.1 for testing//
  const float ORP_MINIMUM = -400.0f; // use -400 for microaerobic trials, use -1000 for anaerobic trials //
}

namespace ThresholdLimits
{
  const float PH_MIN = 4.0f;
  const float PH_MAX = 9.0f;
  const float PH_STEP = 0.1f;
  const float ORP_MIN = -500.0f;
  const float ORP_MAX = 500.0f;
  const float ORP_STEP = 5.0f;
}

namespace BusSpeeds//Bus speeds for I2C and SPI//
{
  const uint32_t I2C_HZ = 100000UL;//I2C bus speed//
  const uint32_t SD_SPI_HZ = 1000000UL;//SPI bus speed for SD card//
}

namespace JoystickConfig//Joystick configuration//
{
  const int ANALOG_CENTER = 1980;//Analog center value//
  const int ANALOG_DEADZONE = 800;//Analog deadzone value, within which the joystick is considered to be in the center position//
  const unsigned long SCROLL_DELAY_MS = 200UL;//Scroll delay time, to prevent rapid scrolling//
  const unsigned long SWITCH_DEBOUNCE_MS = 300UL;//Switch debounce time, to prevent rapid button presses//
}

namespace EzoAddresses//EZO addresses for the 3 probes//
{
  const int PH[3]  = {99, 101, 103};//pH probe addresses//
  const int ORP[3] = {98, 100, 102};//ORP probe addresses//
}

namespace TimingIntervals
{
  const unsigned long PROBE_READ_INTERVAL = 2000UL;//*2 seconds between probe reads//
  const unsigned long SD_LOG_INTERVAL = 30000UL;//*30 seconds between SD card logs//

  const unsigned long VALVE_COOLDOWN = 60UL * 1000UL;//*60 s cooldown between valve triggers//

  // Separate error-lock windows for pH and ORP valves.
  // Each defines how far back the trigger history is scanned when deciding
  // whether a valve has exceeded its trigger limit.  Having two independent
  // constants means the pH and ORP windows can be tuned separately in
  // Config.h without affecting each other.
  const unsigned long PH_VALVE_ERROR_WINDOW  = 60UL * 60UL * 1000UL;//*1 hour window for pH error-locking//
  const unsigned long ORP_VALVE_ERROR_WINDOW = 60UL * 60UL * 1000UL;//*1 hour window for ORP error-locking//

  const int MAX_VALVE_ERROR_HISTORY = 99;

  const unsigned long BLINK_INTERVAL = 300UL;//*300 ms between blink state changes//
  const unsigned long UI_RENDER_INTERVAL = 80UL;//*80 ms between UI renders//

  const unsigned long EZO_READ_INTERVAL = 900UL;//*900 ms between probe reads//
  const unsigned long EZO_EXPORT_IMPORT_DELAY = 1800UL; //*1800 ms delay per calibration data import chunk//
  const unsigned long EZO_CAL_SETTLE_MS = 1000UL; //*1000 ms gap after a Cal command before export begins; the EZO has no command queue so Export must not be sent until Cal has fully executed (~900 ms)//

  // Default valve open durations (seconds); editable at runtime via Thresholds menu//
  const int PH_VALVE_TIMER_SEC  = 10;//*10 s default pH valve open duration//
  const int ORP_VALVE_TIMER_SEC = 10;//*10 s default ORP valve open duration//

  // Queue timers are always valve-timer + 2 s; recomputed at runtime in BioReactor.ino//
  // These compile-time constants give the startup values before any menu edits.//
  const unsigned long PH_VALVE_TIMER  = (unsigned long)PH_VALVE_TIMER_SEC  * 1000UL;
  const unsigned long ORP_VALVE_TIMER = (unsigned long)ORP_VALVE_TIMER_SEC * 1000UL;
  const unsigned long PH_QUEUE_TIMER  = PH_VALVE_TIMER  + 2000UL;
  const unsigned long ORP_QUEUE_TIMER = ORP_VALVE_TIMER + 2000UL;

  // How long a reading must stay below threshold before the valve is queued//
  const unsigned long VALVE_BELOW_THRESHOLD_MS = 10000UL;//*10 s dwell before queueing//
}

namespace TriggerLimitDefaults
{
  const int PH_VALVE_TRIGGER_LIMIT  = 5;//max 5 pH valve triggers per hour (default)//
  const int ORP_VALVE_TRIGGER_LIMIT = 73;//max 73 ORP valve triggers per hour (default)//
}

namespace TriggerLimitBounds
{
  const int MIN  = 1;
  const int MAX  = 99;
  const int STEP = 1;
}

namespace ValveTimerBounds
{
  const int MIN_SEC  = 1;
  const int MAX_SEC  = 18;  //cannot exceed 18 seconds or it will conflict with the 1min cooldown.//
  const int STEP_SEC = 1;
}

namespace ValveMappings//Mapping of MCP pins to valves//
{
  const int TOTAL_VALVES = 6;
}

namespace CalibrationStorage
{
  const char FILE_PATH[] = "/CALIB.CSV";
  const size_t EXPORT_PAYLOAD_MAX = 160;
}

namespace WifiConfig//WiFi + NTP time sync configuration//
{
  const char* const SSID = "WSU Guest";//Open network, no password required//
  const long UTC_OFFSET_SEC = -7 * 3600;//Pacific time offset applied to NTP time//
  const char* const NTP_SERVER = "pool.ntp.org";

  const unsigned long CONNECT_TIMEOUT_MS = 15000UL;//*give up joining WiFi after 15 s//
  const unsigned long TIME_WAIT_MS       = 5000UL;//*give up waiting for an NTP reply after 5 s//
  const unsigned long RESYNC_INTERVAL_MS = 12UL * 60UL * 60UL * 1000UL;//*resync every 12 h once synced, to correct RTC drift//
  const unsigned long RETRY_INTERVAL_MS  = 30UL * 60UL * 1000UL;//*retry every 30 min after a failed attempt//
}

struct ConfigState//Global configuration state//
{
  bool valvesDisabled = false;//Valves disabled flag//
  bool lcdCleared = false;//LCD cleared flag//
  bool blinkState = true;//Blink state flag//
  bool requestResetErrors = false;//Request reset errors flag//

  // Cached "AA:BB:CC:DD:EE:FF" string, filled once at boot from the radio's
  // MAC address. Reading it doesn't require joining a network, so this adds
  // no meaningful delay to startup. Shown on demand via the Settings > Show
  // MAC menu item, rather than printed on every boot.
  char macAddress[18] = "";

  bool valveErrorLocked[ValveMappings::TOTAL_VALVES] = {};//Valve error locked flags//

  float phValues[3] = {0.0f, 0.0f, 0.0f};//pH values from the 3 probes// 
  float orpValues[3] = {};//ORP values from the 3 probes//

  bool phValid[3] = {};//pH valid flags//
  bool orpValid[3] = {};//ORP valid flags//

  float phMinimum = ThresholdDefaults::PH_MINIMUM;
  float orpMinimum = ThresholdDefaults::ORP_MINIMUM;

  bool phBufferCalibrated[3][3] = {};
  bool orpCalibrated[3] = {};

  // Runtime-editable valve trigger limits (triggers per hour before error-lock)//
  int phMaxTrig  = TriggerLimitDefaults::PH_VALVE_TRIGGER_LIMIT;
  int orpMaxTrig = TriggerLimitDefaults::ORP_VALVE_TRIGGER_LIMIT;

  // Runtime-editable valve open durations in seconds//
  int phValveTimerSec  = TimingIntervals::PH_VALVE_TIMER_SEC;
  int orpValveTimerSec = TimingIntervals::ORP_VALVE_TIMER_SEC;
};