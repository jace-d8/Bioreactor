#include <Wire.h>
#include <SPI.h>
#include "Config.h"
#include "Mcp.h"
#include "EzoBoard.h"
#include "Lcd.h"
#include "Sd.h"
#include "Timer.h"
#include "ActionTimer.h"
#include "Menu.h"

#define UTC_OFFSET (-7 * 3600)

ConfigState config;

EzoBoard orpSensors[3] = {//set up ORP sensors (3 probes)//
  EzoBoard(EzoAddresses::ORP[0], "ORP"),
  EzoBoard(EzoAddresses::ORP[1], "ORP"),
  EzoBoard(EzoAddresses::ORP[2], "ORP")
};

EzoBoard phSensors[3] = {//set up pH sensors (3 probes)//
  EzoBoard(EzoAddresses::PH[0], "pH"),
  EzoBoard(EzoAddresses::PH[1], "pH"),
  EzoBoard(EzoAddresses::PH[2], "pH")
};

Mcp mcp;//set up MCP controller (controls the valves)//
Lcd lcd(0x27, &Wire1);//set up LCD display//
SdLogger sd;//set up SD card logger//
Menu menu(&config, phSensors, orpSensors, lcd, sd);

Timer probeTimer(TimingIntervals::PROBE_READ_INTERVAL);//*2 s between probe reads//
Timer sdLogTimer(TimingIntervals::SD_LOG_INTERVAL);//*2 s between SD card logs//

bool queued[ValveMappings::TOTAL_VALVES] = { false };//valve already in queue guard//

ActionTimer cooldown[ValveMappings::TOTAL_VALVES] = {
  ActionTimer(TimingIntervals::VALVE_COOLDOWN),
  ActionTimer(TimingIntervals::VALVE_COOLDOWN),
  ActionTimer(TimingIntervals::VALVE_COOLDOWN),
  ActionTimer(TimingIntervals::VALVE_COOLDOWN),
  ActionTimer(TimingIntervals::VALVE_COOLDOWN),
  ActionTimer(TimingIntervals::VALVE_COOLDOWN)
};//One 60 s cooldown timer per valve; started when a valve is queued.//

// ── Dwell timers ──────────────────────────────────────────────────────────────
// Each valve gets its own ActionTimer.  The timer is started (or kept running)
// while the reading is below the threshold, and reset as soon as the reading
// rises above it.  A valve is only queued once the dwell timer has expired,
// i.e. the reading has been continuously below threshold for at least
// TimingIntervals::VALVE_BELOW_THRESHOLD_MS milliseconds.
ActionTimer dwellTimer[ValveMappings::TOTAL_VALVES] = {
  ActionTimer(TimingIntervals::VALVE_BELOW_THRESHOLD_MS),
  ActionTimer(TimingIntervals::VALVE_BELOW_THRESHOLD_MS),
  ActionTimer(TimingIntervals::VALVE_BELOW_THRESHOLD_MS),
  ActionTimer(TimingIntervals::VALVE_BELOW_THRESHOLD_MS),
  ActionTimer(TimingIntervals::VALVE_BELOW_THRESHOLD_MS),
  ActionTimer(TimingIntervals::VALVE_BELOW_THRESHOLD_MS)
};

// Tracks whether each valve's dwell timer is currently running (started but
// not yet done).  Prevents re-starting the timer on every loop iteration.
bool dwellRunning[ValveMappings::TOTAL_VALVES] = { false };

//History of open times for error-locking (too many opens in *1 hour).//
unsigned long valveTriggerTimes[ValveMappings::TOTAL_VALVES][TimingIntervals::MAX_VALVE_ERROR_HISTORY] = {};
int valveTriggerCounts[ValveMappings::TOTAL_VALVES] = {};

// ── tryQueueValve ─────────────────────────────────────────────────────────────
// condition = true  → reading is below threshold right now.
// A valve is only actually queued once the dwell timer says the reading has
// been below threshold for at least VALVE_BELOW_THRESHOLD_MS continuously.
bool tryQueueValve(int valveId, bool condition)
{
  if (!condition)
  {
    // Reading is fine — reset the dwell timer so the clock starts fresh
    // if it dips below threshold again later.
    dwellTimer[valveId].stop();
    dwellRunning[valveId] = false;
    return false;
  }

  // Reading is below threshold.  Kick off the dwell timer on the first
  // low reading.  dwellRunning[] tracks whether we have already called
  // start() so we do not reset a running timer on every loop iteration.
  if (!dwellRunning[valveId])
  {
    dwellTimer[valveId].start();
    dwellRunning[valveId] = true;
  }

  // Not yet held below threshold long enough.
  if (!dwellTimer[valveId].done()) return false;

  // Dwell satisfied — now apply the usual guards.
  if (config.valvesDisabled)               return false;
  if (queued[valveId])                     return false;
  if (config.valveErrorLocked[valveId])    return false;
  if (!cooldown[valveId].done())
  {
    // Cooldown still running — reset dwell so the 5 s must elapse
    // again from scratch once the cooldown is over.
    dwellTimer[valveId].stop();
    dwellRunning[valveId] = false;
    return false;
  }
  if (!mcp.enqueue(valveId))               return false;

  cooldown[valveId].start();
  queued[valveId] = true;

  // Reset dwell so the timer must expire again for the next trigger.
  dwellTimer[valveId].stop();
  dwellRunning[valveId] = false;

  return true;
}

// ── recordValveTrigger ────────────────────────────────────────────────────────
void recordValveTrigger(int valveId)
{
  int& count = valveTriggerCounts[valveId];

  if (count < TimingIntervals::MAX_VALVE_ERROR_HISTORY)
  {
    valveTriggerTimes[valveId][count++] = millis();
  }
  else
  {
    for (int i = 1; i < TimingIntervals::MAX_VALVE_ERROR_HISTORY; ++i)
      valveTriggerTimes[valveId][i - 1] = valveTriggerTimes[valveId][i];
    valveTriggerTimes[valveId][TimingIntervals::MAX_VALVE_ERROR_HISTORY - 1] = millis();
  }

  // Use runtime-editable limits from ConfigState.
  const bool isPh  = (valveId % 2 == 0);
  const int  limit = isPh ? config.phMaxTrig : config.orpMaxTrig;

  // Select the error window for the valve type being checked.
  // pH and ORP now have independent windows defined in Config.h so they
  // can be tuned separately without affecting each other.
  const unsigned long errorWindow = isPh
    ? TimingIntervals::PH_VALVE_ERROR_WINDOW
    : TimingIntervals::ORP_VALVE_ERROR_WINDOW;

  if (count >= limit)
  {
    unsigned long oldest = valveTriggerTimes[valveId][count - limit];
    if (millis() - oldest < errorWindow)
    {
      if (!config.valveErrorLocked[valveId])
      {
        config.valveErrorLocked[valveId] = true;
        sd.logValveLocked(valveId);
      }
    }
  }
}

// ── resetValveErrors ──────────────────────────────────────────────────────────
void resetValveErrors()
{
  for (int i = 0; i < ValveMappings::TOTAL_VALVES; ++i)
  {
    config.valveErrorLocked[i] = false;
    valveTriggerCounts[i]      = 0;
    queued[i]                  = false;
    dwellTimer[i].stop();
    dwellRunning[i]            = false;
    // Stop the cooldown so valves can re-trigger immediately after a reset.
    // ActionTimer::done() returns true when not running, so stop() is all
    // that is needed — no separate "clear" is required.
    cooldown[i].stop();

    for (int j = 0; j < TimingIntervals::MAX_VALVE_ERROR_HISTORY; ++j)
      valveTriggerTimes[i][j] = 0;
  }

  mcp.resetState(queued);
  config.requestResetErrors = false;
}

// ── handleProbeReads ──────────────────────────────────────────────────────────
void handleProbeReads()
{
  for (int i = 0; i < 3; ++i)
  {
    config.phValues[i]  = phSensors[i].read();
    config.orpValues[i] = orpSensors[i].read();
    config.phValid[i]   = phSensors[i].hasValidReading();
    config.orpValid[i]  = orpSensors[i].hasValidReading();
  }

  if (!config.lcdCleared)
  {
    lcd.clear();
    config.lcdCleared = true;
  }

  if (!menu.isActive())
  {
    lcd.printData(config);

    // ROW_3 is not used by printData (which only writes rows 0-2), so it is
    // safe to write here without disrupting the main display.
    // The message is padded to 20 characters to overwrite any stale content
    // from a previous state (e.g. a menu screen that wrote to this row).
    lcd.setCursor(COL_LEFT, ROW_3);
    lcd.print(sd.isHealthy() ? "                    " : "SD ERR - CHECK CARD ");
  }
}

// ── setup ─────────────────────────────────────────────────────────────────────
void setup()
{
  Wire.begin();
  Wire.setClock(BusSpeeds::I2C_HZ);
  // Release the bus automatically if any I2C transaction stalls for more than
  // 50 ms.  Without this, a locked-up EZO board (SDA held low due to bus
  // loading or a defective board) would block Wire.requestFrom() indefinitely
  // and trigger the ESP32 task watchdog — the same failure mode as an SD card
  // garbage-collection stall.  The second argument (true) resets the bus on
  // timeout so subsequent transactions can proceed normally.
  Wire.setTimeout(50);  // 50 ms in microseconds

  Wire1.begin(PinConfigurations::LCD_PIN_SDA, PinConfigurations::LCD_PIN_SCL, BusSpeeds::I2C_HZ);

  const bool mcpReady = mcp.begin();

  lcd.init();

  SPI.begin();

  configTime(UTC_OFFSET, 0, "");

  const bool sdReady = sd.begin(PinConfigurations::SD_CHIP_SELECT, BusSpeeds::SD_SPI_HZ);
  sd.setTimeFromBuild();

  if (!mcpReady || !sdReady)
  {
    lcd.clear();
    lcd.setCursor(COL_LEFT, ROW_TITLE);
    lcd.print(!mcpReady ? "MCP init failed" : "SD init failed");
    lcd.setCursor(COL_LEFT, ROW_1);
    lcd.print(!mcpReady && !sdReady ? "SD init failed" : "Check wiring");
  }
  else
  {
    sd.restoreCalibrations(phSensors, orpSensors, config);
    sd.logMessage("BOOT");
  }
}

// ── loop ──────────────────────────────────────────────────────────────────────
void loop()
{
  if (probeTimer.isReady())
    handleProbeReads();

  if (config.requestResetErrors)
    resetValveErrors();

  // pH valves: ids 0, 2, 4
  tryQueueValve(0, config.phValid[0] && config.phValues[0] < config.phMinimum);
  tryQueueValve(2, config.phValid[1] && config.phValues[1] < config.phMinimum);
  tryQueueValve(4, config.phValid[2] && config.phValues[2] < config.phMinimum);

  // ORP valves: ids 1, 3, 5
  tryQueueValve(1, config.orpValid[0] && config.orpValues[0] < config.orpMinimum);
  tryQueueValve(3, config.orpValid[1] && config.orpValues[1] < config.orpMinimum);
  tryQueueValve(5, config.orpValid[2] && config.orpValues[2] < config.orpMinimum);

  // Pass runtime valve-timer durations so the MCP uses the latest menu values.
  // opened[] receives every valve id opened this cycle (up to 2: one pH, one ORP).
  int opened[2];
  int openedCount = mcp.update(queued, config.phValveTimerSec, config.orpValveTimerSec, opened);
  for (int i = 0; i < openedCount; ++i)
  {
    sd.logValveOn(opened[i]);
    recordValveTrigger(opened[i]);
  }

  if (sdLogTimer.isReady())
    sd.logData(config);

  if (!menu.isActive() && menu.joystick.isPressed())
    menu.enter();

  if (menu.isActive())
  {
    menu.update();
    menu.draw();
  }
}
