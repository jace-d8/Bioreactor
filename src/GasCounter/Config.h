// ============================================================================
// Config.h
//
// PLAIN-ENGLISH SUMMARY:
// This is the "settings dashboard" for the whole gas-counting project --
// same role as Config.h in the companion bioreactor project. Nearly every
// tunable number (pin assignments, defaults, edit limits) lives here in
// one place. See the bioreactor project's Config.h for a fuller
// explanation of what a "namespace" is and why this file is organized
// this way; this file follows the exact same pattern, just with different
// content.

//THIS IS THE OPTICAL-SENSOR (Adafruit 3397) VARIANT OF Config.h.
//
// It is a drop-in replacement for the project's normal Config.h when 
// the SST Sensing Optomax LLC200D3SH liquid level sensors is used:
// https://www.adafruit.com/product/3397
// ============================================================================

#pragma once
#include <Arduino.h>

namespace PinConfigurations
{
  const int PIN_SW = 5;//Joystick switch pin//
  const int PIN_Y = 2;//Joystick y-axis pin//
  const int LCD_PIN_SDA = 6;//LCD SDA pin//
  const int LCD_PIN_SCL = 7;//LCD SCL pin//
  const int SD_CHIP_SELECT = 21;//SD chip select pin//

  // Liquid level sensors (SST Sensing Optomax LLC200D3SH), one per gas
  // channel, connected directly to Arduino digital inputs per
  // https://cdn-shop.adafruit.com/product-files/3397/3397_datasheet_actual.pdf
  //
  // >>> EDIT THESE: replace with the actual GPIO pin each sensor's Output
  // >>> wire is connected to on your board.
  //
  // Wire colors (this sensor only has 3 wires, same colors on both ends --
  // no dual-adapter-cable naming split like the bioreactor's SEN0368):
  //   Red   -> Vs   (sensor power, 4.5-15.4 VDC)
  //   Blue  -> 0V   (ground)
  //   Green -> Output (LEVEL SIGNAL -- wire this one to the Arduino pin below)
  //
  // *** VOLTAGE WARNING ***
  // This sensor's Output goes HIGH to (Vs - 1V). Since Vs must be at least
  // 4.5V, an unmodified Output-HIGH signal can reach ~3.5-14.4V -- well
  // above the ESP32's 3.3V-max input rating, and the datasheet explicitly
  // warns that shorting the output causes irreparable damage, so this is
  // not a "probably fine" situation. You MUST add a voltage divider (two
  // resistors) or a logic-level shifter between each sensor's Output wire
  // and its Arduino pin below, sized so the pin never sees more than 3.3V.
  //
  // Output logic (default part variant, "Output High in air"): HIGH when
  // the tip is in air (no liquid), LOW when the tip is immersed (liquid
  // detected) -- see LevelSensorConfig::ACTIVE_HIGH below, already set to
  // match this default part variant.
  // ── Pin pool and assignment (Arduino Nano ESP32) ───────────────────────
  // Board label -> GPIO number (the numbers actually stored in the array
  // below -- Arduino's digitalRead()/pinMode() on this board accept
  // either the printed "Dn"/"An" label via a macro, or the raw GPIO
  // number directly; this codebase uses raw GPIO numbers everywhere, per
  // the "GPIO numbering" scheme):
  //
  //   D5 = GPIO8    D6 = GPIO9    D7 = GPIO10   D8 = GPIO17   D9 = GPIO18
  //   A0 = GPIO1    A2 = GPIO3    A3 = GPIO4    A6 = GPIO13   A7 = GPIO14
  //   B0 = GPIO46   B1 = GPIO0
  //
  // Of the 12 pins currently free on the board (D5, D6, D7, D8, D9, B0,
  // A0, A2, A3, A6, A7, B1), B0 and B1 are DELIBERATELY EXCLUDED from the
  // 6 chosen below: GPIO46 and GPIO0 are two of the ESP32-S3's four
  // "strapping" pins, sampled at boot/reset to select boot mode (GPIO0
  // held LOW = download mode). Arduino's own documentation for this
  // board recommends using B0/B1 for OUTPUTS only, since an external
  // device holding one of them in the "wrong" state at the moment of a
  // reset can put the board into the wrong boot mode. A level sensor is
  // exactly the kind of external device that could do that (its output
  // depends on liquid level, not on what the board wants at reset), so
  // it must not be wired to B0/B1. That leaves 10 genuinely safe general-
  // purpose candidates: D5-D9 and A0/A2/A3/A6/A7.
  //
  // The 6 pins below are the same 6 this project already used (all of
  // which fall within that safe 10-pin set) -- S1-S3 on the analog
  // header, S4-S6 on the digital header:
  const int LEVEL_SENSOR_PIN[6] = {1, 3, 4, 8, 9, 10}; //S1-S6 -PIN A0, A2, A3, D5, D6, D7//
  // An ARRAY of 6 whole numbers, one Arduino pin per gas channel:
  // LEVEL_SENSOR_PIN[0] is channel 1's sensor pin, LEVEL_SENSOR_PIN[1] is
  // channel 2's, and so on. This lets the rest of the code loop "for each
  // channel c, read pin LEVEL_SENSOR_PIN[c]" instead of writing 6 separate
  // near-identical blocks of code.
  //
  // A6 (GPIO13) and A7 (GPIO14) are left unused -- spares for a 7th/8th
  // channel later, or for the capacitive sensor variant's MODE pins if
  // you choose the per-channel-MODE-pin wiring option instead of the
  // hardwired-MODE option (see capacitive_sensor_variant/ for both).
}

namespace LevelSensorConfig
{
  // Set true if the sensor output reads HIGH when liquid is detected,
  // false if LOW. The default LLC200D3SH variant (no suffix on the output
  // logic order code) is "Output High in air", i.e. LOW when liquid is
  // detected -- hence false here. If you ordered the inverted-logic
  // variant, change this to true.
  const bool ACTIVE_HIGH = false;
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

namespace TimingIntervals
{
  const unsigned long DISPLAY_UPDATE_INTERVAL = 500UL;//*0.5 s between LCD readout refreshes//
  // Unlike the bioreactor project (which paces its LCD updates off "how
  // often the probes are read"), this project has no probes to poll --
  // the level sensors are read every single loop() iteration regardless
  // (see GasChannel::update()). This timer exists purely to limit how
  // often the LCD itself gets rewritten, independent of sensor reading.
  const unsigned long SD_LOG_INTERVAL = 30000UL;//*30 s between SD card snapshot logs//

  const unsigned long BLINK_INTERVAL = 300UL;//*300 ms between blink state changes//
  const unsigned long UI_RENDER_INTERVAL = 80UL;//*80 ms between UI renders//

  // Fixed 1-minute window used for the "too many triggers per minute"
  // check. Only the trigger-COUNT threshold is editable (see
  // GasChannelBounds::MAX_TRIG_PER_MIN below); the window itself is fixed
  // at 1 minute per the spec ("more than 29 times in a minute").
  const unsigned long TRIGGER_RATE_WINDOW_MS = 60UL * 1000UL;

  // How many past trigger timestamps are remembered per channel, purely
  // for the rate check above. Must be at least ONE MORE than the largest
  // configurable per-minute limit (GasChannelBounds::MAX_TRIG_PER_MIN_MAX)
  // -- see GasChannel.cpp's recordTrigger_() for exactly why the "+1" is
  // required (in short: locking out a channel only once it has triggered
  // MORE THAN maxTrigPerMin times in a minute means the rate check has to
  // look back maxTrigPerMin+1 triggers, one further than the limit
  // itself, so the history needs room for that one extra entry too).
  const int MAX_TRIGGER_HISTORY = 100;
}

namespace GasChannelDefaults
// Starting values for the 3 editable settings, changeable later from the
// Settings menu. These apply to all 6 channels at once (see the note in
// GasChannel.h about why this project uses one shared setting rather than
// 6 independent ones).
{
  const int   PULSE_SEC_DEFAULT          = 2;     // valve pulse duration, seconds
  const float CAL_FACTOR_DEFAULT         = 1.00f; // mL of gas per valve trigger
  const int   MAX_TRIG_PER_MIN_DEFAULT   = 29;     // trigger-rate error threshold
}

namespace GasChannelBounds
// The allowed MIN/MAX/STEP when editing each of the 3 settings above from
// the menu. Every menu edit screen reads these three numbers so the
// joystick can never push a setting outside a sane range (see
// Menu::adjustSettingsFromJoystick(), which uses Arduino's constrain()
// with exactly these MIN/MAX pairs).
{
  const int   PULSE_SEC_MIN        = 1,     PULSE_SEC_MAX        = 10,    PULSE_SEC_STEP        = 1;
  const float CAL_FACTOR_MIN       = 0.00f, CAL_FACTOR_MAX       = 5.00f, CAL_FACTOR_STEP       = 0.05f;
  const int   MAX_TRIG_PER_MIN_MIN = 1,     MAX_TRIG_PER_MIN_MAX = 99,    MAX_TRIG_PER_MIN_STEP = 1;
  // "const int A = 1, B = 30, C = 1;" declares three separate constants
  // on one line -- purely a formatting choice to keep each setting's
  // MIN/MAX/STEP grouped together visually (same style used in the
  // bioreactor project's Config.h).
}

namespace ChannelMappings//Mapping of MCP pins to gas-channel valves//
{
  const int CHANNEL_COUNT = 6;
  const int TOTAL_VALVES  = CHANNEL_COUNT; // ids 0-5, one per channel
  // Unlike the bioreactor project (which has several different KINDS of
  // valve -- pH, ORP, feed, waste, vent -- laid out in separate blocks),
  // every valve here does the exact same job, so the mapping is as simple
  // as it gets: valve id == channel index, no offsets needed at all.

  // ── Physical MCP23X17 pin map ──────────────────────────────────────────
  // The Adafruit_MCP23X17 library numbers its 16 GPIO pins 0-15, where
  // pins 0-7 are port A (GPA0-GPA7) and pins 8-15 are port B (GPB0-GPB7).
  // Wire your transistor/valve driver for each channel to the MCP23X17
  // physical pin below (a fixed consequence of the channel numbering
  // above, not something you need to edit):
  //
  //   channel | valve id | mcp pin | MCP23X17 physical pin
  //   --------+----------+---------+------------------------
  //      1    |    0     |    0    | GPA0
  //      2    |    1     |    1    | GPA1
  //      3    |    2     |    2    | GPA2
  //      4    |    3     |    3    | GPA3
  //      5    |    4     |    4    | GPA4
  //      6    |    5     |    5    | GPA5
  //                                  GPA6-GPB7 (pins 6-15) unused / free
}

struct ConfigState//Global configuration state//
// There is exactly ONE ConfigState in the whole program (see
// GasCounter.ino, where a single "config" variable is created), shared by
// pointer/reference with every other class -- Menu, Lcd, SdLogger,
// GasChannel all read and/or write this SAME struct. See the bioreactor
// project's Config.h for the fuller explanation of why this "central
// status board" design is used throughout both projects.
{
  bool valvesDisabled = false;//Global pause: while true, no channel will trigger its valve//
  bool lcdCleared = false;//LCD cleared flag//
  bool blinkState = true;//Blink state flag//
  bool requestResetErrors = false;//Request reset errors flag//

  bool channelErrorLocked[ChannelMappings::CHANNEL_COUNT] = {};//Rate-limit error lock, per channel//
  // One true/false per channel: has this channel been locked out for
  // triggering too fast? "= {}" starts every entry at false (not locked).

  bool  sensorState[ChannelMappings::CHANNEL_COUNT]   = {};//Live level-sensor reading, per channel//
  float gasVolumeML[ChannelMappings::CHANNEL_COUNT]   = {};//Cumulative gas volume since boot, per channel (mL)//
  unsigned long triggerCount[ChannelMappings::CHANNEL_COUNT] = {};//Cumulative trigger count since boot, per channel//
  // gasVolumeML and triggerCount only ever grow (they represent real
  // measured gas production) -- unlike channelErrorLocked, they are
  // deliberately NEVER touched by Settings > Reset Error (see
  // GasChannel::resetErrors()'s comments). They reset to 0 only on a
  // power cycle.

  // Runtime-editable settings (see Menu > Settings), shared by all 6 channels//
  int   pulseSec        = GasChannelDefaults::PULSE_SEC_DEFAULT;
  float calFactor       = GasChannelDefaults::CAL_FACTOR_DEFAULT;
  int   maxTrigPerMin   = GasChannelDefaults::MAX_TRIG_PER_MIN_DEFAULT;
  // These start out equal to the ...Defaults constants above, but can
  // then be changed independently at runtime from the menu -- editing
  // pulseSec here does NOT change GasChannelDefaults::PULSE_SEC_DEFAULT
  // itself, which stays fixed as the value the device boots up with.
};
