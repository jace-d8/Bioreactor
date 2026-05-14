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
#include "ReactorController.h"

#define UTC_OFFSET (-7 * 3600)

ConfigState config;

EzoBoard orpSensors[3] = {
  EzoBoard(98,  "ORP"),
  EzoBoard(100, "ORP"),
  EzoBoard(102, "ORP")
};

EzoBoard phSensors[3] = {
  EzoBoard(99,  "pH"),
  EzoBoard(101, "pH"),
  EzoBoard(103, "pH")
};

Mcp mcp;
Lcd lcd(0x27, &Wire1);
SdLogger sd;
Menu menu(&config, phSensors, orpSensors, lcd);
ReactorController reactorCtrl(mcp, sd);

Timer probeTimer(TimingIntervals::PROBE_READ_INTERVAL);
Timer sdLogTimer(TimingIntervals::SD_LOG_INTERVAL);

bool queued[ValveMappings::TOTAL_VALVES] = { false };

ActionTimer cooldown[ValveMappings::TOTAL_VALVES] = {
  ActionTimer(TimingIntervals::VALVE_COOLDOWN),
  ActionTimer(TimingIntervals::VALVE_COOLDOWN),
  ActionTimer(TimingIntervals::VALVE_COOLDOWN),
  ActionTimer(TimingIntervals::VALVE_COOLDOWN),
  ActionTimer(TimingIntervals::VALVE_COOLDOWN),
  ActionTimer(TimingIntervals::VALVE_COOLDOWN)
};

unsigned long valveTriggerTimes[ValveMappings::TOTAL_VALVES][TimingIntervals::MAX_VALVE_ERROR_HISTORY] = {};
int valveTriggerCounts[ValveMappings::TOTAL_VALVES] = {};

bool tryQueueValve(int valveId, bool condition)
{
  if (!condition) return false;
  if (config.valvesDisabled) return false;
  if (queued[valveId]) return false;
  if (config.valveErrorLocked[valveId]) return false;
  if (!cooldown[valveId].done()) return false;
  if (!mcp.enqueue(valveId)) return false;

  cooldown[valveId].start();
  queued[valveId] = true;
  return true;
}

void recordValveTrigger(int valveId)
{
  int &count = valveTriggerCounts[valveId];

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

  const bool isPhValve = (valveId % 2 == 0);
  const int limit = isPhValve
    ? TimingIntervals::PH_VALVE_TRIGGER_LIMIT
    : TimingIntervals::ORP_VALVE_TRIGGER_LIMIT;

  if (count >= limit)
  {
    unsigned long oldest = valveTriggerTimes[valveId][count - limit];
    if (millis() - oldest < TimingIntervals::VALVE_ERROR_WINDOW)
    {
      if (!config.valveErrorLocked[valveId])
      {
        config.valveErrorLocked[valveId] = true;
        sd.logValveLocked(valveId);
      }
    }
  }
}

void resetValveErrors()
{
  for (int i = 0; i < ValveMappings::TOTAL_VALVES; ++i)
  {
    config.valveErrorLocked[i] = false;
    valveTriggerCounts[i] = 0;
    queued[i] = false;

    for (int j = 0; j < TimingIntervals::MAX_VALVE_ERROR_HISTORY; ++j)
      valveTriggerTimes[i][j] = 0;
  }

  mcp.resetState(queued);

  config.requestResetErrors = false;
}

void handleProbeReads()
{
  for (int i = 0; i < 3; ++i)
  {
    config.phValues[i] = phSensors[i].read();
    config.orpValues[i] = orpSensors[i].read();

    config.phValid[i] = phSensors[i].hasValidReading();
    config.orpValid[i] = orpSensors[i].hasValidReading();
  }

  if (!config.lcdCleared)
  {
    lcd.clear();
    config.lcdCleared = true;
  }

  if (!menu.isActive())
    lcd.printData(config);
}

void setup()
{
  Wire.begin();
  Wire.setClock(100000);

  Wire1.begin(PinConfigurations::LCD_PIN_SDA, PinConfigurations::LCD_PIN_SCL, 100000);

  const bool mcpReady = mcp.begin();

  lcd.init();

  SPI.begin();

  configTime(UTC_OFFSET, 0, "");

  const bool sdReady = sd.begin(PinConfigurations::SD_CHIP_SELECT, 1000000);
  sd.setTimeFromBuild();

  if (!mcpReady || !sdReady)
  {
    lcd.clear();
    lcd.setCursor(COL_LEFT, ROW_TITLE);
    lcd.print(!mcpReady ? "MCP init failed" : "SD init failed");
    lcd.setCursor(COL_LEFT, ROW_1);
    lcd.print(!mcpReady && !sdReady ? "SD init failed" : "Check wiring");
  }

  sd.logMessage("BOOT");

  menu.setReactorController(&reactorCtrl);
  reactorCtrl.begin();
}

void loop()
{
  if (probeTimer.isReady())
    handleProbeReads();

  if (config.requestResetErrors)
    resetValveErrors();

  tryQueueValve(0, config.phValid[0] && config.phValues[0] < Thresholds::PH_MINIMUM);
  tryQueueValve(2, config.phValid[1] && config.phValues[1] < Thresholds::PH_MINIMUM);
  tryQueueValve(4, config.phValid[2] && config.phValues[2] < Thresholds::PH_MINIMUM);

  tryQueueValve(1, config.orpValid[0] && config.orpValues[0] < Thresholds::ORP_MINIMUM);
  tryQueueValve(3, config.orpValid[1] && config.orpValues[1] < Thresholds::ORP_MINIMUM);
  tryQueueValve(5, config.orpValid[2] && config.orpValues[2] < Thresholds::ORP_MINIMUM);

  int openedValve = mcp.update(queued);
  if (openedValve >= 0)
  {
    sd.logValveOn(openedValve);
    recordValveTrigger(openedValve);
  }

  reactorCtrl.update(config);

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