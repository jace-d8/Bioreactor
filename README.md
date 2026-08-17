# Gas counting system

This is the firmware for an Arduino Nano ESP32 controlling a gas counting (respirometry) system with 6 parallel gas counters. The hardware for the gas counting system is based on that of the bioreactor system, except that all the Atlas EZO pH and ORP probes are replaced with liquid level sensors. There are two types of liquid level sensors that can be used: an optical liquid level sensor that is sensitive but needs to physically contact the liquid and a capacitive liquid level sensor that is less sensitive to the liquid used here to prevent evaporation (silicone oil) but does not need to contact the liquid at all.

## What it does

1. A U-tube holds a non-evaporative liquid (silicone oil), and gas production pushes the liquid to the other side of the U-tube, causing the liquid level to rise.
2. When the liquid touches the liquid sensor, it signals the Arduino board to open the corresponding 3-way solenoid valve to reset the liquid level by opening the gas-pushing side to atmospheric pressure.
3. The valve opening event is logged as 1 count, which is a fixed volume of gas that is determined through calibration.
4. The valve stays open for 2 seconds, during which the gas counting is paused. This is called cooldown.
5. The 3-way solenoid valve resets after the 2-second cooldown, allowing gas produced from the bioreactor to push the liquid level again.
6. The gas count is logged every 30 seconds on an SD card
7. However, if the gas counter gets triggered more than 29 times in 1 minute, the liquid must be stuck at the high level on the sensor side. The system reports an error.

The LCD shows live readings when idle. The joystick (analog Y axis + push switch) opens the menu, where you can calibrate the gas counter, change settings like cooldown time, and toggle valve control.

## Hardware

| Component | Role |
|---|---|
| Arduino Nano ESP32 | Main MCU |
| MCP23017 I/O expander | Drives the 6 dosing valves over I2C |
| 6x Atlas liquid level sensor: either the DFRobot Capacitive Liquid Level Sensor (SEN0368) or Adafruit Optomax Digital Liquid Level Sensor (3397)|
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
| 0 | gas valve 1 |
| 1 | gas valve 2 |
| 2 | gas valve 3 |
| 3 | gas valve 4 |
| 4 | gas valve 5 |
| 5 | gas valve 6 |

## Module map

| File | What it does |
|---|---|
| [`BioReactor.ino`](src/BioReactor/BioReactor.ino) | Top-level setup + main loop. Owns the `ConfigState`, the probe arrays, and the valve trigger history. |
| [`Config.h`](src/BioReactor/Config.h) | All pins, thresholds, timings, and the shared `ConfigState` struct. |
| [`GasChannel.{h,cpp}`](src/BioReactor/EzoBoard.h) | Reads the level sensor. Notices if it just transitioned from "no liquid" to "liquid". On that transition, opens the valve for a fixed pulse, and either counts it as real gas production or -- if this channel has been triggering too fast lately -- locks the channel out instead. |
| [`Mcp.{h,cpp}`](src/BioReactor/Mcp.h) | Owns the MCP23017 and the dosing-valve control. |
| [`Sd.{h,cpp}`](src/BioReactor/Sd.h) | SPI SD logger. Auto-generates a fresh `.CSV` at boot, writes timestamped sensor rows and message rows for valve events and lockouts. |
| [`Lcd.{h,cpp}`](src/BioReactor/Lcd.h) | I2C 20x4 driver wrapper. Live data view and menu rendering helpers. |
| [`Joystick.{h,cpp}`](src/BioReactor/Joystick.h) | Reads the analog Y axis with a dead zone, debounces the switch, holds the current menu selection and max index. |
| [`Menu.{h,cpp}`](src/BioReactor/Menu.h) | UI state machine (`Idle / Calibrating / Valves / PhCalibration / OrpCalibration / Off`). Probe calibration writes EZO `Cal,low/mid/high/clear` commands; valve menu toggles dosing on/off and resets error locks. |
| [`Timer.{h,cpp}`](src/BioReactor/Timer.h) | Interval timer: `isReady()` returns true once `interval` has elapsed since last `reset()`. |
| [`ActionTimer.h`](src/BioReactor/ActionTimer.h) | One-shot cooldown timer. `start()` arms it, `done()` reports whether the duration has elapsed. |

## Logging format

Every row in `LOG####.CSV` has the header `timestamp,gas1,gas2,gas3,gas4,gas5,gas6,message`.

- **Sensor rows** (every `SD_LOG_INTERVAL`): the 6 readings populated, message blank.
- **Event rows** (valve fires, lockouts, boot): sensor columns blank, message column populated. Examples:


## Boot sequence

`setup()` brings up `Wire` (MCP bus), `Wire1` (LCD bus), the MCP, the LCD, SPI, the system clock (`configTime`), and the SD logger. If either the MCP or the SD card fails to initialize, I show the failure on the LCD instead of starting the main loop. On success, I write a `BOOT` row to the log.

## Menu navigation

Push the joystick at any time from the idle screen to open the menu.


## Dependencies

Install through the Arduino IDE Library Manager unless noted otherwise:

- **SD** by SparkFun
- **Adafruit MCP23017 Arduino Library** by Adafruit
- **I2C_LCD** by Rob Tillaart
- **LiquidCrystal I2C** by Frank de Barbander

In the Arduino IDE, under **Tools**, set Pin Numbering to **"By GPIO number (legacy)"**.

## Building

Open [`src/BioReactor/BioReactor.ino`](src/BioReactor/BioReactor.ino) in the Arduino IDE, select the Arduino Nano ESP32 board, set the pin numbering option above, and upload.

---

Kuang Zhu and Jace Dunn
Kuangs Biotech Group, WSU  
jace.dunn@wsu.edu
