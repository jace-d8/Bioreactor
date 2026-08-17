# Capacitive sensor (DFRobot SEN0368) variant

This folder contains the one file needed to use DFRobot's Non-contact
Capacitive Liquid Level Sensor (SEN0368,
https://www.dfrobot.com/product-2109.html) instead of the SST Sensing
Optomax LLC200D3SH the main project uses by default: **`Config.h`**.

## Only `Config.h` needs to change

The main project's `GasChannel.cpp` (unmodified -- use the one from the
project root, not a separate copy) already handles both sensors:

```cpp
void GasChannel::begin()
{
  if (PinConfigurations::LEVEL_SENSOR_MODE_PIN >= 0)
  {
    pinMode(PinConfigurations::LEVEL_SENSOR_MODE_PIN, OUTPUT);
    digitalWrite(PinConfigurations::LEVEL_SENSOR_MODE_PIN, HIGH);
  }
  for (int c = 0; c < ChannelMappings::CHANNEL_COUNT; ++c)
    pinMode(PinConfigurations::LEVEL_SENSOR_PIN[c], INPUT);
}
```

`LEVEL_SENSOR_MODE_PIN` is a sentinel: the standard `Config.h` sets it
to `-1` ("not wired" -- the Optomax sensor has no such pin), and this
folder's `Config.h` sets it to a real GPIO instead. `begin()` checks
that value and only configures/drives the pin when it's actually wired
to something. That's the only piece of sensor-specific logic in
`GasChannel.cpp`, and it now lives behind a Config.h-driven switch
instead of needing its own copy of the file -- so swapping sensors really
is just swapping `Config.h`, matching how the rest of this project
already worked (`LevelSensorConfig::ACTIVE_HIGH` did the same job for
polarity).

An earlier version of this variant shipped its own modified
`GasChannel.cpp` alongside `Config.h`. That's no longer needed (and has
been removed from this folder) now that the sentinel-value check lives
in the main project's `GasChannel.cpp` itself -- if you have that older
copy lying around, delete it and use the project root's `GasChannel.cpp`
instead, since mixing the two `Config.h` files with the wrong
`GasChannel.cpp` either fails to compile (capacitive `GasChannel.cpp` +
standard `Config.h`, which has no `LEVEL_SENSOR_MODE_PIN` to reference)
or silently leaves MODE floating (standard `GasChannel.cpp` without the
sentinel check + capacitive `Config.h`).

## Wiring: 7 pins total, not 12

DFRobot's own example code drives each sensor's MODE pin from a
dedicated microcontroller output pin:

```cpp
int inPin = 8;
boolean running = 0;
int modePin = 9;

void setup()
{
  pinMode(inPin, INPUT);
  pinMode(modePin, OUTPUT);
  digitalWrite(modePin, running);
}
```

That's a real GPIO, not a wire tied to a fixed rail -- so MODE can't be
hardwired to VCC. But it also only ever gets set ONCE, at boot, and is
never toggled again during normal operation. Since it's a single fixed
logic level rather than a per-channel or per-loop signal, **one shared
Arduino pin can drive all 6 sensors' MODE inputs in parallel** -- a
digital control input like this draws only microamps of leakage current,
a negligible load to fan out from one GPIO. So this variant uses:

- 6 pins for `OUT` (one per channel, read every loop -- this genuinely
  needs to be per-channel, since each sensor's liquid reading is
  independent)
- 1 pin for `MODE` (shared, wired to all 6 sensors' blue wires in
  parallel -- a simple wire splice or small terminal block, no extra
  components needed)

7 pins total, leaving 5 of the original 12-pin pool completely free.

## Pin assignment

| Purpose | Pins used | Board label -> GPIO |
|---|---|---|
| OUT, channel 1-6 (input, from sensor) | 6 | D5=8, D6=9, D7=10, D8=17, D9=18, A0=1 |
| MODE, all 6 sensors (output, from board) | 1 | B0=46 |
| Free / spare | 5 | A2=3, A3=4, A6=13, A7=14, B1=0 |

OUT pins (driven by the sensor) avoid `B0`/`B1`, for the same reason the
standard `Config.h` avoids them for the Optomax sensor: they're two of
the ESP32-S3's boot-strapping pins, and Arduino's own guidance is to use
them for outputs only. MODE (driven by the board, never the sensor) is
exactly that -- an output -- so `B0` is a good, safe home for it, and it
leaves every general-purpose pin (A2/A3/A6/A7 and B1) free.

## Wiring and setup

1. Wire each SEN0368 sensor's `OUT` (yellow) wire through its own
   voltage divider or logic-level shifter to its own INPUT pin above --
   SEN0368 runs at 5-12V and its output swings close to that supply
   voltage, which exceeds the ESP32-S3's 3.3V-max GPIO rating.
2. Wire all 6 sensors' `MODE` (blue) wires together, and connect that
   single joined wire to the one MODE pin above (no divider needed --
   this is an output FROM the board, already at the board's own 3.3V
   logic level).
3. Replace the project's `Config.h` with this folder's version. Leave
   every other file, including `GasChannel.cpp`, as-is.

## Polarity

With MODE driven HIGH by `GasChannel::begin()`, `OUT` reads HIGH exactly
when liquid is detected (DFRobot's "running = 1" case), so
`LevelSensorConfig::ACTIVE_HIGH` is `true` in this variant's `Config.h`
-- the opposite polarity from the Optomax sensor's default wiring
(`false`).
