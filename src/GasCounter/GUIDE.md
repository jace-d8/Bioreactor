# Gas Counting System — Plain-English Guide

This guide is a companion to the line-by-line comments in every code
file. Read this first for the big picture, then dip into whichever
file's comments you need for detail.

## 1. What this device does

One Arduino Nano ESP32 monitors **six independent gas-counting channels**
at once. For each channel, it:

- Watches a liquid level sensor for the moment liquid rises past a
  trigger point.
- Opens that channel's 3-way valve for a fixed pulse (2 seconds by
  default) to vent the trapped gas — this pulse also acts as a
  **cooldown**: nothing new can be counted on that channel until the
  pulse finishes and the sensor has genuinely reset.
- Converts each valve pulse into a gas volume using an adjustable
  calibration factor (mL of gas per trigger).
- Watches for a channel triggering too rapidly (a likely sign of a stuck
  sensor or malfunction) and locks that channel out until a human clears
  the error from the menu.
- Displays running totals on a small LCD screen and logs every trigger,
  error, and periodic snapshot to an SD card with a timestamp.
- Lets you adjust the pulse duration, calibration factor, and rate-limit
  threshold using a joystick + button, without reflashing the code.

This project is a sibling of the bioreactor control system — same core
hardware (Arduino Nano ESP32, MCP23X17, LCD, joystick, SD card), same
architecture and coding patterns, adapted for a very different job. If
you've read the bioreactor project's guide, most of what follows will
feel familiar.

## 2. How the files relate to each other

**Layer 1 — talks to hardware directly** (the "drivers"), largely
unchanged from the bioreactor project:
- `Mcp.h/.cpp` — talks to the MCP23X17 chip that controls all 6 valves.
  Much simpler than the bioreactor's version: no queueing, since each
  channel's valve is fully independent.
- `Lcd.h/.cpp` — talks to the physical LCD screen.
- `Sd.h/.cpp` — talks to the SD card. Keeps the bioreactor project's
  write-reliability machinery (see §6) but drops all probe-calibration
  storage, since there are no probes here.
- `Joystick.h/.cpp` — reads the physical joystick + button. Byte-for-byte
  identical to the bioreactor project's version.
- `Timer.h/.cpp` and `ActionTimer.h` — the same reusable "stopwatch"
  helpers, also unchanged.

**Layer 2 — decision-making** (the "brains"):
- `GasChannel.h/.cpp` — decides, for each of the 6 channels, whether it
  should trigger, count, or lock out. This is the new centerpiece of the
  project, playing the same role `FeedWaste` played in the bioreactor.
- `Menu.h/.cpp` — decides what the LCD shows and what a button press
  should do. Much smaller than the bioreactor's Menu, since there's no
  probe calibration to navigate — a 4-item main menu (Settings / Reset
  Error / Valves On-Off / Exit) plus one Settings screen holding just the
  3 numeric settings (PulseSec / CalFactor / MaxTrigPerMin).

**Layer 3 — the conductor**:
- `GasCounter.ino` — creates one of everything, and its `loop()` function
  calls into every other file, over and over, forever.

- `Config.h` sits underneath all three layers — nearly every tunable
  number and pin assignment lives there.

## 3. The one idea that explains almost everything: never wait

Just like the bioreactor project, this code never uses `delay()` for
anything longer than a few milliseconds, because that would freeze the
whole chip — including the sensor reading that has to happen every single
`loop()` pass to catch a rising edge promptly. Instead, `GasChannel`
tracks each channel's own state with plain `millis()` timestamps
(`valveStarted_[]`, `triggerTimes_[]`) and compares against the current
time every pass through `loop()`, rather than ever blocking to wait.

## 4. The core trick: edge detection, not level detection

This is the one idea in `GasChannel.cpp` most worth understanding
deeply, since it's what makes the whole "count discrete events" concept
work:

- Each channel remembers what its sensor read on the *previous* pass
  through `loop()`.
- A "trigger" only happens on the exact moment the reading flips from
  **no liquid** to **liquid detected** — a **rising edge** — not on every
  pass where liquid happens to be detected.
- Without this, a single physical rise-and-vent event (which takes a
  couple of seconds to fully resolve) would get counted hundreds of
  times, since `loop()` runs far faster than that.
- The 2-second valve pulse doubles as a **cooldown**: while a channel's
  valve is open, that channel can't trigger again no matter what the
  sensor reads, and even once the pulse ends, the sensor has to actually
  drop back to "no liquid" and rise again before the next count can
  register.

## 5. How to make common changes

**Change a pin number** → `Config.h`, `namespace PinConfigurations`.

**Swap the liquid level sensor hardware** (e.g. to DFRobot's SEN0368
capacitive sensor instead of the default Optomax sensor) → just swap in
the alternate `Config.h` from `capacitive_sensor_variant/` at the
project root. `GasChannel.cpp` is written to work unchanged with either
sensor -- see its `begin()`, which checks
`PinConfigurations::LEVEL_SENSOR_MODE_PIN` (a sentinel: `-1` means "this
sensor has no MODE pin," a real GPIO number means "drive it HIGH once at
boot") and `LevelSensorConfig::ACTIVE_HIGH` (which polarity means
"liquid detected"). Both live in `Config.h`, so that's the only file
that needs to change.

**Change a default pulse duration, calibration factor, or rate limit** →
`Config.h`, `namespace GasChannelDefaults` (starting values) and
`namespace GasChannelBounds` (the min/max/step allowed when editing from
the menu). All three are also editable live from Settings without
reflashing.

**Make settings per-channel instead of shared by all 6** → currently
`pulseSec`, `calFactor`, and `maxTrigPerMin` in `ConfigState` (`Config.h`)
are single values shared by every channel. To make them independent,
turn each into an array of 6 (following the same pattern as
`gasVolumeML[]`/`triggerCount[]`), and add a per-channel selector to the
Settings screen in `Menu.cpp`.

**Change what a menu screen says or does** → `Menu.cpp`. Find
`handleMainMenu()`/`displayMainMenu()` for the main menu (Settings /
Reset Error / Valves On-Off / Exit), or `handleSettingsMenu()` /
`displaySettingsMenu()` for the Settings screen's 3 numeric values.

**Change how a channel decides to trigger/lock** → `GasChannel.cpp`'s
`update()` function, and its three helpers `recordTrigger_()`,
`countTrigger_()`, `lockChannel_()`.

**Change what gets logged to the SD card** → `Sd.cpp`'s `logMessage()`
and `logData()`, or the `logEvent_()`/`countTrigger_()` calls in
`GasChannel.cpp`.

## 6. Why the SD "recovery" logic is still here

`Sd.cpp` still contains the same two-stage write-recovery system
(`tryRecover_()`) and split flush strategy (`writeRow_()`) the bioreactor
project used. This is **not** related to probes or EZO boards in any
way — it exists because SD cards themselves can occasionally fail to
write (a worn card, a brief glitch, the card's own internal
garbage-collection pauses), and this project still logs continuously for
potentially long, unattended runs. Removing it would trade reliability
for a slightly shorter file, with no real upside.

## 7. Key C++ concepts that show up everywhere

Explained in-line the first time each appears; quick index below:

| Concept | What it means | See it explained in |
|---|---|---|
| Edge detection | Reacting only to a *change* in a reading, not its steady value | `GasChannel.h`'s class comment, `GasChannel.cpp`'s `update()` |
| Pointer (`Type*`, `&var`) | A variable holding "where something else lives in memory" | `GasChannel.h`, `Mcp.cpp` |
| Reference (`Type&`) | An alternate name for an existing variable | `GasChannel.cpp`'s `recordTrigger_()` |
| `enum class` | A named set of fixed options | `Menu.h`'s `SettingsEdit` |
| `namespace` | A labeled group of related constants | `Config.h` |
| Header (`.h`) vs. implementation (`.cpp`) | `.h` = what a class can do; `.cpp` = how | `GasChannel.h` / `GasChannel.cpp` |
| Ternary operator (`a ? b : c`) | A one-line if/else that produces a value | `Mcp.cpp`'s `setValve()` |
| Lambda (`[&](...){...}`) | A small nameless function defined on the spot | `Menu.cpp`'s `displaySettingsMenu()` |
| 2D array (`arr[6][99]`) | A grid — 6 channels x 99 remembered timestamps each | `GasChannel.h`'s `triggerTimes_` |

## 8. The Arduino Nano ESP32 pin-remap workaround

`Mcp.h`/`Mcp.cpp` carry the same `push_macro`/`undef`/`pop_macro` blocks
around `pinMode`/`digitalRead`/`digitalWrite` that the bioreactor project
needed, for the identical reason: the Nano ESP32 board core and the
Adafruit_MCP23X17 library both use those three names, and left
unguarded, one corrupts the other. If a build ever produces errors
mentioning `digitalPinToGPIONumber`, this is almost certainly the cause,
and the fix is already scoped correctly in these two files — no action
needed unless you're adding a *new* file that also touches the MCP23X17
object directly.

## 9. Where to look when something goes wrong

- **"MCP init failed" / "SD init failed" on the LCD at boot** — a wiring
  or hardware problem; `GasCounter.ino`'s `setup()` shows this if
  `mcp.begin()` or `sd.begin()` returns false.
- **"GAS RATE ERROR" on the LCD** — one or more channels triggered too
  many times within a minute and got locked out; check the SD log for
  which channel, then use Settings > Reset Error once the underlying
  cause (usually a stuck or noisy sensor) is fixed.
- **"SD ERR - CHECK CARD" on the LCD** — `Sd.cpp`'s automatic recovery
  already tried and failed; check the card itself.
- **A channel's total looks too high/low** — check `calFactor` in
  Settings; the displayed volume is simply `triggerCount * calFactor`,
  recomputed incrementally in `GasChannel::countTrigger_()`.
- **A channel seems "stuck locked"** — check
  `config.channelErrorLocked[]` in `Config.h`'s `ConfigState`; use the
  main menu's Reset Error item to clear it (this does not erase the
  channel's cumulative gas volume/trigger count, only the error and its
  supporting history).

## 10. Recent fixes (see each file's comments for the full reasoning)

- **LCD gas readout now shows whole mL, no decimal** (`Lcd.cpp`). The SD
  log still records 2 decimal places (`Sd.cpp`'s `logData()`), and
  `ConfigState::gasVolumeML` still accumulates at full float precision
  internally — only the small on-screen readout changed.
- **Menu restructured**: Reset Error and the Valves On/Off toggle moved
  out of Settings onto the main menu, directly parallel to Settings
  (Reset Error listed above Valves) — see `Menu.h`/`Menu.cpp`.
- **Rate-limit off-by-one bug fixed** (`GasChannel.cpp`'s
  `recordTrigger_()`): a channel used to lock out on exactly its
  `maxTrigPerMin`-th trigger within a minute, one trigger earlier than
  the spec ("more than 29 times in a minute") actually calls for. Fixed
  by checking `maxTrigPerMin + 1` triggers back instead of
  `maxTrigPerMin`; `TimingIntervals::MAX_TRIGGER_HISTORY` in `Config.h`
  was bumped from 99 to 100 to keep enough history for that one extra
  look-back at the top of the editable range.
- **Joystick no longer blocks `loop()` while scrolling** (`Joystick.h`/
  `Joystick.cpp`): `move()`/`yAxisStep()` used a blocking
  `delay(SCROLL_DELAY_MS)` to pace menu scrolling, which — contrary to
  this project's own "never wait" rule (§3 above) — paused
  `gasChannel.update()` for up to 200 ms every time the operator nudged
  the joystick, risking a missed rising edge on a fast-cycling channel.
  Replaced with the same non-blocking `Timer` pattern already used for
  button debouncing.
- **Level sensor pin assignment reviewed** (`Config.h`): documented why
  the board's `B0`/`B1` pins (GPIO46/GPIO0, two of the ESP32-S3's boot
  strapping pins) are excluded from the level-sensor pin pool and should
  be reserved for outputs only, and confirmed the existing 6-pin
  assignment already falls within the safe pin set.
