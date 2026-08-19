# BioReactor

This is the firmware for an Arduino Nano ESP32 controlling a 3-reactor bioreactor rig. Each reactor has its own pH and ORP probe (Atlas Scientific EZO boards) and its own pair of dosing valves. The firmware reads the probes on a fixed cadence, automatically doses acid or oxidizer when readings fall below configured thresholds, displays live data on a 20x4 LCD, logs everything to an SD card, and exposes a joystick-driven menu for probe calibration and valve control.

## What it does

On a 2-second cadence, the firmware:

1. Reads pH and ORP from each of the 3 reactors over I2C (6 EZO probes total).
2. Compares each reading to its threshold. If a probe reads below threshold, the corresponding dosing valve is enqueued.
3. The MCP23017 I/O expander services the queue. One valve fires at a time, opens for 10 seconds, then closes. The next queued valve waits 12 seconds after the previous fired before it can open.
4. Each valve has a 60-second cooldown after firing before it can be re-enqueued.
5. Each valve has a rolling error window: if a valve fires more than its allowed trigger count inside a 60-minute window, I latch an error lock on that valve so it stops firing until the user resets it from the menu.
6. Sensor readings are written to the SD card every 2 seconds, and individual valve events get logged as message rows.

The LCD shows live readings when idle. The joystick (analog Y axis + push switch) opens the menu, where you can calibrate any probe and toggle valve control.
The timestamp is synced on boot via WiFi. Currently, the WiFi connection is set to be automatically connected to Washington State University Guest WiFi (WSU Guest), an open WiFi that requires adding the Arduino Nano ESP32 MAC address to a whitelist. WiFi SSID and time zone can be changed in config.h.

## Hardware

| Component | Role |
|---|---|
| Arduino Nano ESP32 | Main MCU |
| MCP23017 I/O expander | Drives the 6 dosing valves over I2C |
| 6x Atlas Scientific EZO boards | 3 pH + 3 ORP probes, I2C |
| LCM2004A 20x4 LCD | I2C display (on `Wire1`) |
| SD card module | SPI logging |
| 2-axis analog joystick with switch | Menu input |
| 6x N-channel power MOSFETs | Switch the dosing solenoids |
| 12V DC supply + LM2596 buck converters | Power for the logic side and the valves |

## Pin assignments

All in [`Config.h`](src/BioReactor/Config.h):

| Pin | Purpose |
|---|---|
| `PIN_Y = 2` | Joystick Y axis (analog) |
| `PIN_SW = 5` | Joystick push switch (active-low, `INPUT_PULLUP`) |
| `LCD_PIN_SDA = 6` | LCD I2C SDA (`Wire1`) |
| `LCD_PIN_SCL = 7` | LCD I2C SCL (`Wire1`) |
| `SD_CHIP_SELECT = 21` | SD card SPI chip-select |

MCP23017 outputs:

| MCP pin | Valve |
|---|---|
| 0 | pH1 dosing valve |
| 1 | ORP1 dosing valve |
| 2 | pH2 dosing valve |
| 3 | ORP2 dosing valve |
| 4 | pH3 dosing valve |
| 5 | ORP3 dosing valve |

EZO I2C addresses (declared in [`Config.h`](src/BioReactor/Config.h) under `EzoAddresses`, instantiated in [`BioReactor.ino`](src/BioReactor/BioReactor.ino)):

| Probe | I2C address |
|---|---|
| pH1 / pH2 / pH3 | 99 / 101 / 103 |
| ORP1 / ORP2 / ORP3 | 98 / 100 / 102 |

## Configurables

In [`Config.h`](src/BioReactor/Config.h):

```cpp
namespace Thresholds
{
  const float PH_MINIMUM = 6.3f;   // dose acid if pH drops below
  const int   ORP_MINIMUM = -400;  // dose oxidizer if ORP drops below
}

namespace BusSpeeds
{
  const uint32_t I2C_HZ    = 100000UL;    // both Wire and Wire1
  const uint32_t SD_SPI_HZ = 1000000UL;   // SD card SPI clock
}

namespace JoystickConfig
{
  const int           ANALOG_CENTER       = 1980;    // analogRead center value
  const int           ANALOG_DEADZONE     = 400;     // counts above/below center before scrolling
  const unsigned long SCROLL_DELAY_MS     = 200UL;   // throttle between menu scrolls
  const unsigned long SWITCH_DEBOUNCE_MS  = 300UL;   // joystick push-button debounce
}

namespace EzoAddresses
{
  const int PH[3]  = {99, 101, 103};
  const int ORP[3] = {98, 100, 102};
}

namespace TimingIntervals
{
  const unsigned long PROBE_READ_INTERVAL = 2000UL;             // probe polling
  const unsigned long SD_LOG_INTERVAL     = 2000UL;             // SD write cadence
  const unsigned long VALVE_COOLDOWN      = 60UL * 1000UL;      // per-valve refire lockout
  const unsigned long VALVE_ERROR_WINDOW  = 60UL*60UL*1000UL;   // rolling error window
  const int PH_VALVE_TRIGGER_LIMIT  = 10;                       // max pH fires per window
  const int ORP_VALVE_TRIGGER_LIMIT = 20;                       // max ORP fires per window
  const int MAX_VALVE_ERROR_HISTORY = 20;                       // ring buffer size
  const unsigned long BLINK_INTERVAL      = 300UL;              // menu cursor blink
  const unsigned long UI_RENDER_INTERVAL  = 80UL;               // menu redraw cap (~12.5 FPS)
  const unsigned long EZO_READ_INTERVAL   = 900UL;              // EZO state-machine tick
  const unsigned long QUEUE_TIMER         = 12000UL;            // min gap between queue pops
  const unsigned long VALVE_TIMER         = 10000UL;            // how long a valve stays open
}
```

`VALVE_TIMER` and `QUEUE_TIMER` are read by [`Mcp.cpp`](src/BioReactor/Mcp.cpp) and control the dosing valve on-time and the inter-valve gap. To shorten a dose (e.g. for tighter NaOH or air control), reduce `VALVE_TIMER` and keep `QUEUE_TIMER` at least as large.

## Module map

| File | What it does |
|---|---|
| [`BioReactor.ino`](src/BioReactor/BioReactor.ino) | Top-level setup + main loop. Owns the `ConfigState`, the probe arrays, and the valve trigger history. |
| [`Config.h`](src/BioReactor/Config.h) | All pins, thresholds, timings, and the shared `ConfigState` struct. |
| [`EzoBoard.{h,cpp}`](src/BioReactor/EzoBoard.h) | Wraps the Atlas EZO I2C driver. Runs an async read state machine (`Reading -> Waiting -> Receiving`), holds the last valid value, and exposes `sendCmd()` for calibration. |
| [`Mcp.{h,cpp}`](src/BioReactor/Mcp.h) | Owns the MCP23017 and the dosing-valve queue. `enqueue()` adds a valve, `update()` services the queue (open one valve, wait `VALVE_TIMER`, close it, gate the next pop on `QUEUE_TIMER`). |
| [`Sd.{h,cpp}`](src/BioReactor/Sd.h) | SPI SD logger. Auto-generates a fresh `LOG####.CSV` at boot, writes timestamped sensor rows and message rows for valve events and lockouts. |
| [`Lcd.{h,cpp}`](src/BioReactor/Lcd.h) | I2C 20x4 driver wrapper. Live data view and menu rendering helpers. |
| [`Joystick.{h,cpp}`](src/BioReactor/Joystick.h) | Reads the analog Y axis with a dead zone, debounces the switch, holds the current menu selection and max index. |
| [`Menu.{h,cpp}`](src/BioReactor/Menu.h) | UI state machine (`Idle / Calibrating / Valves / PhCalibration / OrpCalibration / Off`). Probe calibration writes EZO `Cal,low/mid/high/clear` commands; valve menu toggles dosing on/off and resets error locks. |
| [`Timer.{h,cpp}`](src/BioReactor/Timer.h) | Interval timer: `isReady()` returns true once `interval` has elapsed since last `reset()`. |
| [`ActionTimer.h`](src/BioReactor/ActionTimer.h) | One-shot cooldown timer. `start()` arms it, `done()` reports whether the duration has elapsed. |
| [`Wifi.h`](src/BioReactor/Wifi.h) | Connects the system to Wifi and syncs time at boot. Writes the status of time sync to the log file |
## Logging format

Every row in `LOG####.CSV` has the header `timestamp,pH1,ORP1,pH2,ORP2,pH3,ORP3,message`.

- **Sensor rows** (every `SD_LOG_INTERVAL`): the 6 readings populated, message blank.
- **Event rows** (valve fires, lockouts, boot): sensor columns blank, message column populated. Examples:

```
2026-05-13 14:02:11,6.273,-412,6.95,-380,6.84,-405,
2026-05-13 14:02:13,,,,,,,pH1 valve was triggered
2026-05-13 14:02:13,,,,,,,BOOT
2026-05-13 14:03:47,,,,,,,ORP2 valve locked
```

## Boot sequence

`setup()` brings up `Wire` (EZO + MCP bus), `Wire1` (LCD bus), the MCP, the LCD, SPI, the system clock (`configTime`), and the SD logger. If either the MCP or the SD card fails to initialize I show the failure on the LCD instead of starting the main loop. On success I write a `BOOT` row to the log.

## Menu navigation

Push the joystick at any time from the idle screen to open the menu.

| Screen | Actions |
|---|---|
| **Main** | Calibrate Probes / Valve Control / Exit |
| **Calibrate Probes** | Pick pH1/2/3 or ORP1/2/3 to enter probe calibration, or Done to go back |
| **pH calibration** | `Cal,low,4.00`, `Cal,mid,7.00`, `Cal,high,10.00`, `Cal,clear`, Done. A `*` marks buffers already calibrated for the active probe. The live pH reading is shown at the bottom. |
| **ORP calibration** | `Cal,222` (single-point at 222 mV) or Done. The live ORP reading is shown at the bottom. |
| **Valve Control** | Toggle valves enabled/disabled globally, reset any latched valve error locks, or return. |

## Naming Conventions

| Item            | Style           | Example              |
|-----------------|----------------|----------------------|
| Classes         | PascalCase     | `EzoBoard`           |
| Functions       | camelCase      | `sendCmd()`          |
| Variables       | camelCase      | `orpValue`           |
| Private Members | trailing `_`   | `lastTrigger_`       |
| Constants       | ALL_CAPS       | `MAX_BUFFER_SIZE`    |

## Dependencies

Install through the Arduino IDE Library Manager unless noted otherwise:

- **SD** by SparkFun
- **Adafruit MCP23017 Arduino Library** by Adafruit
- **I2C_LCD** by Rob Tillaart
- **LiquidCrystal I2C** by Frank de Barbander
- **Ezo_I2c_lib** by Atlas-Scientific. This one is not in the Library Manager. I downloaded it from the Atlas-Scientific GitHub page and installed it manually into the Arduino IDE.

In the Arduino IDE, under **Tools**, set Pin Numbering to **"By GPIO number (legacy)"**.

## Building

Open [`src/BioReactor/BioReactor.ino`](src/BioReactor/BioReactor.ino) in the Arduino IDE, select the Arduino Nano ESP32 board, set the pin numbering option above, and upload.

---

Jace Dunn  
Kuangs Biotech Group, WSU  
jace.dunn@wsu.edu
