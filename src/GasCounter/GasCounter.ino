// ============================================================================
// GasCounter.ino
//
// PLAIN-ENGLISH SUMMARY:
// This is the "main" file of the whole project -- the one that creates
// every object and ties every other file together. Just like the
// bioreactor project's BioReactor.ino, every Arduino program requires
// exactly two functions:
//   - setup()  runs ONCE, right when the board powers on. Everything gets
//     initialized here (screen, valves, SD card).
//   - loop()   runs OVER AND OVER, forever, as fast as the chip can
//     manage. This is where the actual "keep watching the sensors, keep
//     the menu responsive" work happens.
// Nothing in loop() ever waits for very long (no multi-second delay()
// calls) -- see GasChannel.cpp's edge-detection logic and Timer.h for how
// that's achieved without ever blocking the whole chip.
// ============================================================================

#include <Wire.h>
// Arduino's built-in I2C communication library.
#include <SPI.h>
// Arduino's built-in SPI communication library (used by the SD card).
#include "Config.h"
#include "Mcp.h"
#include "Lcd.h"
#include "Sd.h"
#include "Timer.h"
#include "Menu.h"
#include "GasChannel.h"

#define UTC_OFFSET (-7 * 3600)
// Pacific time's offset from UTC in seconds -- adjust if used elsewhere.
// This board has no internet connection to sync its clock automatically,
// so setTimeFromBuild() (see Sd.cpp) uses the program's own compile time
// as a starting approximation of "now," and this offset makes that show
// up as local time rather than UTC in the log's timestamps.

// ── Global objects: one of each, shared by the whole program ──────────────
ConfigState config;
// THE single ConfigState (see Config.h) that every other class gets a
// pointer/reference to.

Mcp mcp;//set up MCP controller (controls the 6 gas-channel valves)//
Lcd lcd(0x27, &Wire1);//set up LCD display//
SdLogger sd;//set up SD card logger//
Menu menu(&config, lcd, sd);
// Constructs the Menu, handing it a POINTER to config (&config means "the
// memory address of config") and REFERENCES to lcd and sd.
GasChannel gasChannel(&config, &mcp, &sd);//set up the 6-channel gas counting controller//
// GasChannel gets POINTERS to config, mcp, and sd (matching its
// constructor's expected parameter types -- see GasChannel.h).

Timer displayTimer(TimingIntervals::DISPLAY_UPDATE_INTERVAL);
Timer sdLogTimer(TimingIntervals::SD_LOG_INTERVAL);
// Two Timer objects (see Timer.h) used to space out how often the LCD
// redraws its readout and how often a data snapshot gets logged to the
// SD card. Note neither of these gates the actual sensor reading/edge
// detection -- gasChannel.update() runs every single loop() pass,
// unconditionally, regardless of these timers (see loop() below).

// ── anyChannelError ──────────────────────────────────────────────────────────
bool anyChannelError()
// A quick check used by the LCD status row: is ANY channel currently
// locked with a rate-limit error?
{
  for (int c = 0; c < ChannelMappings::CHANNEL_COUNT; ++c)
    if (config.channelErrorLocked[c])
      return true;
      // As soon as we find ONE locked channel, we already have our
      // answer -- no need to keep checking the rest.
  return false;
  // Checked every channel and found no lock.
}

// ── resetErrors ───────────────────────────────────────────────────────────────
void resetErrors()
// Called from loop() whenever config.requestResetErrors is true (set by
// Menu.cpp's Settings > Reset Error option).
{
  gasChannel.resetErrors();
  config.requestResetErrors = false;
  // Clear the request flag last, now that the reset has actually happened.
}

// ── handleDisplayUpdate ─────────────────────────────────────────────────────
void handleDisplayUpdate()
// Called from loop() every DISPLAY_UPDATE_INTERVAL. Redraws the LCD's
// main readout, unless the menu is currently covering the screen.
{
  if (!config.lcdCleared)
  {
    lcd.clear();
    config.lcdCleared = true;
    // Clear the screen exactly once, the very first time this function
    // runs after boot -- afterward, printData() only overwrites specific
    // characters rather than wiping and redrawing the whole screen each
    // time, which avoids visible flicker.
  }

  if (!menu.isActive())
  // Only update the main gas readout screen if the menu ISN'T currently
  // showing something else -- otherwise we'd be fighting the menu for
  // control of the same physical screen.
  {
    lcd.printData(config);

    lcd.setCursor(COL_LEFT, ROW_3);
    if (anyChannelError())
      lcd.print("GAS RATE ERROR      ");
    else
      lcd.print(sd.isHealthy() ? "                    " : "SD ERR - CHECK CARD ");
      // "condition ? valueIfTrue : valueIfFalse" -- if the SD card is
      // healthy, print 20 blank spaces (clearing any old message);
      // otherwise print the warning text (also padded to 20 characters
      // so it fully overwrites whatever was there before). Rate errors
      // take priority over the SD status since they mean a channel's
      // counting has stopped and needs attention.
  }
}

// ── setup ─────────────────────────────────────────────────────────────────────
void setup()
// Runs exactly once, right after the board powers on.
{
  Wire.begin();
  // Start the main I2C bus, used by the MCP23X17 chip. (No probes in
  // this project, but the chip still talks over this same bus, same as
  // the bioreactor project.)
  Wire.setClock(BusSpeeds::I2C_HZ);
  Wire.setTimeout(50);
  // Release the bus automatically if any I2C transaction stalls for more
  // than 50 ms, rather than blocking forever.

  Wire1.begin(PinConfigurations::LCD_PIN_SDA, PinConfigurations::LCD_PIN_SCL, BusSpeeds::I2C_HZ);
  // Start the SECOND I2C bus (Wire1), dedicated to the LCD screen, on its
  // own pair of pins.

  const bool mcpReady = mcp.begin();
  gasChannel.begin();

  lcd.init();

  SPI.begin();
  // Start the SPI bus (used by the SD card).

  configTime(UTC_OFFSET, 0, "");
  // Tell the ESP32's built-in clock what time zone offset to use (the
  // third argument would normally be an NTP time server address for
  // syncing over the internet, left blank since this board has no
  // network connection -- see sd.setTimeFromBuild() below for how it
  // gets an actual starting date/time instead).

  const bool sdReady = sd.begin(PinConfigurations::SD_CHIP_SELECT, BusSpeeds::SD_SPI_HZ);
  sd.setTimeFromBuild();

  if (!mcpReady || !sdReady)
  // If EITHER the valve controller or the SD card failed to initialize,
  // show an error on the screen rather than silently continuing broken.
  {
    lcd.clear();
    lcd.setCursor(COL_LEFT, ROW_TITLE);
    lcd.print(!mcpReady ? "MCP init failed" : "SD init failed");
    lcd.setCursor(COL_LEFT, ROW_1);
    lcd.print(!mcpReady && !sdReady ? "SD init failed" : "Check wiring");
    // A small piece of logic to show the right message whether one or
    // both subsystems failed: if MCP failed, say so on the title row; if
    // SD ALSO failed, mention that on the row below too; otherwise (only
    // one thing failed) just prompt to check the wiring.
  }
  else
  {
    sd.logMessage("BOOT");
    // Everything initialized fine -- log a "BOOT" event so the SD card's
    // log has a clear marker for every time the device was powered on.
  }
}

// ── loop ──────────────────────────────────────────────────────────────────────
void loop()
// Runs over and over, forever, for as long as the board has power.
{
  gasChannel.update();
  // Runs every single loop iteration, unconditionally (not gated by a
  // timer) -- a rising edge on any of the 6 level sensors must be caught
  // promptly, and the valve pulse's auto-close timing must stay accurate.

  if (config.requestResetErrors)
    resetErrors();
    // If the menu set this flag (Settings > Reset Error), handle it now.

  if (displayTimer.isReady())
    handleDisplayUpdate();
    // Only actually redraw the LCD roughly every 0.5 seconds -- no need
    // to do this literally every single pass through loop(), which would
    // happen thousands of times per second.

  if (sdLogTimer.isReady())
    sd.logData(config);
    // Only write a full data snapshot row every 30 seconds.

  if (!menu.isActive() && menu.joystick.isPressed())
    menu.enter();
    // If the menu isn't currently showing, and the joystick button was
    // just freshly pressed, open the menu.

  if (menu.isActive())
  {
    menu.update();
    menu.draw();
    // While the menu is showing, let it react to joystick input and
    // redraw itself as needed, every single pass through loop().
  }
}
