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
// ============================================================================

#pragma once
#include <Arduino.h>

// ============================================================================
// THIS IS THE CAPACITIVE-SENSOR (DFRobot SEN0368) VARIANT OF Config.h.
//
// It is a drop-in replacement for the project's normal Config.h, for use
// if you swap the SST Sensing Optomax LLC200D3SH liquid level sensors for
// DFRobot's Non-contact Capacitive Liquid Level Sensor (SEN0368):
// https://www.dfrobot.com/product-2109.html
// Example/reference code: https://wiki.dfrobot.com/sen0368/docs/19134
//
// This sensor needs no other file changed: GasChannel.cpp's begin() was
// written to check PinConfigurations::LEVEL_SENSOR_MODE_PIN and only
// touch it if it's >= 0 -- the standard Config.h sets it to -1 (meaning
// "not wired," since the Optomax sensor has no such pin), and this file
// sets it to a real GPIO number instead. That one shared, sensor-
// agnostic GasChannel.cpp -- from the main project, unmodified -- is
// all you need alongside this Config.h. Every other file (Menu.h/.cpp,
// Lcd.h/.cpp, GasCounter.ino, etc.) is unchanged too, since they only
// ever go through GasChannel::readSensor_(), which still just reads a
// pin and applies LevelSensorConfig::ACTIVE_HIGH exactly as before.
//
// MODE is shared across all 6 sensors from a SINGLE pin (see
// PinConfigurations::LEVEL_SENSOR_MODE_PIN below), not one pin per
// channel -- see that constant's comment for why this is safe.
// ============================================================================

namespace PinConfigurations
{
  const int PIN_SW = 5;//Joystick switch pin//
  const int PIN_Y = 2;//Joystick y-axis pin//
  const int LCD_PIN_SDA = 6;//LCD SDA pin//
  const int LCD_PIN_SCL = 7;//LCD SCL pin//
  const int SD_CHIP_SELECT = 21;//SD chip select pin//

  // Liquid level sensors (DFRobot SEN0368, non-contact capacitive), one
  // per gas channel, connected directly to Arduino digital pins. This
  // sensor mounts to the OUTSIDE of a non-metallic container/tube (up to
  // 20 mm wall thickness, OD >= 11 mm) and senses liquid through the
  // wall via capacitance -- unlike the Optomax sensor, it never touches
  // the liquid at all, so there's no wetted part to fail or foul.
  //
  // Wiring (4-pin adapter cable included with the sensor):
  //   Red    -> VCC     (sensor power, 5-12 VDC)
  //   Black  -> GND     (ground)
  //   Yellow -> OUT     (digital LEVEL SIGNAL -- an INPUT to the Arduino;
  //                       matches "inPin" in DFRobot's own example code)
  //   Blue   -> MODE    (selects output polarity -- an OUTPUT FROM the
  //                       Arduino; matches "modePin" in DFRobot's own
  //                       example code, which drives it with
  //                       digitalWrite(modePin, running))
  //
  // *** MODE is shared: one Arduino pin drives all 6 sensors' MODE ***
  // MODE is a plain digital control input on the sensor -- once set, it
  // is never toggled again during normal operation (GasChannel::begin()
  // drives it HIGH exactly once, at boot, and nothing in update() ever
  // touches it afterward). Because it's a single fixed logic level, not
  // a signal that changes per channel or per loop iteration, one ESP32
  // GPIO can drive all 6 sensors' MODE inputs in parallel with a single
  // digitalWrite(): a digital input like this draws only microamps of
  // leakage current, so 6 of them in parallel are a negligible load for
  // a GPIO rated for tens of milliamps. Wire the ONE MODE pin below to
  // all 6 sensors' blue wires (a simple wire splice, or a small terminal
  // block/bus -- no additional components needed). This is standard
  // practice for "set once, shared among many inputs" control lines, and
  // it means this variant only needs 7 pins total (6 OUT + 1 shared
  // MODE) instead of 12 -- so, unlike an earlier version of this file,
  // it does NOT need every pin in the 12-pin pool.
  //
  // *** VOLTAGE WARNING -- same caution as the Optomax sensor ***
  // SEN0368's operating voltage is 5-12 VDC, and its OUT signal ("high
  // level digital output") swings to roughly VCC. Even at the sensor's
  // minimum 5V supply, an Output-HIGH signal (~5V) still exceeds the
  // ESP32-S3's 3.3V-max GPIO input rating. You MUST add a voltage
  // divider (two resistors) or a logic-level shifter between each
  // sensor's OUT wire and its Arduino INPUT pin below, sized so the pin
  // never sees more than 3.3V. This only applies to OUT (an input to the
  // board, driven by the sensor) -- MODE is an output FROM the board
  // driven at the board's own 3.3V logic level, so it needs no divider.
  //
  // Output logic: per DFRobot's example code, driving MODE HIGH sets
  // "running = 1", which makes OUT read HIGH exactly when liquid is
  // detected. We drive MODE HIGH once in GasChannel::begin() (see
  // GasChannel.cpp -- the same, unmodified one from the main project)
  // and leave it there, so
  // in normal operation OUT is a plain active-high liquid signal --
  // hence ACTIVE_HIGH = true below (the OPPOSITE polarity from the
  // Optomax sensor's default wiring).
  //
  // ── Pin pool and assignment (Arduino Nano ESP32) ───────────────────────
  // Board label -> GPIO number (see the standard Config.h for the full
  // table and the reasoning behind it): D5=8, D6=9, D7=10, D8=17, D9=18,
  // A0=1, A2=3, A3=4, A6=13, A7=14, B0=46, B1=0.
  //
  // OUT (INPUT) pins use the same 6 general-purpose candidates already
  // used for the Optomax sensor -- these are pins the sensor itself
  // drives, so (per the standard Config.h's reasoning) B0/B1 are
  // excluded here too, since an external device driving one of them
  // could interfere with the board's boot mode:
  const int LEVEL_SENSOR_PIN[6] = {8, 9, 10, 17, 18, 1}; //S1-S6 OUT -PIN D5, D6, D7, D8, D9, A0//
  // An ARRAY of 6 whole numbers, one Arduino INPUT pin per gas channel:
  // LEVEL_SENSOR_PIN[0] is channel 1's OUT (signal) pin, and so on --
  // read every loop via GasChannel::readSensor_(), exactly like the
  // Optomax sensor's single pin was.

  // MODE (OUTPUT) -- a SINGLE pin, wired to all 6 sensors' MODE inputs
  // in parallel (see the note above). Set once as OUTPUT and driven HIGH
  // in GasChannel::begin(), then never touched again. Using B0 here --
  // rather than one of the 4 still-unused general-purpose pins (A2, A3,
  // A6, A7) -- is deliberate: this pin is purely an output the board
  // drives, and Arduino's own guidance for this board is to use B0/B1
  // for outputs only, so B0 is the ideal (and safest) home for it, and
  // it leaves every general-purpose pin free for future use.
  const int LEVEL_SENSOR_MODE_PIN = 46; //MODE (shared, all 6 sensors) -PIN B0//
  // A6 (GPIO13), A7 (GPIO14), A2 (GPIO3), A3 (GPIO4), and B1 (GPIO0) are
  // all left completely free -- spares for a 7th/8th channel later, or
  // for anything else the project might need.
}

namespace LevelSensorConfig
{
  // Set true if the sensor output reads HIGH when liquid is detected,
  // false if LOW.
  //
  // THIS IS THE ONE LINE THAT ACTUALLY DIFFERS FROM THE STANDARD
  // Config.h: with SEN0368's MODE pin driven HIGH by GasChannel::begin()
  // (see the wiring note above -- "running = 1" in DFRobot's own
  // example), OUT reads HIGH when liquid is detected, so this is true
  // here -- versus false for the Optomax sensor's default wiring.
  const bool ACTIVE_HIGH = true;
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
