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

// TO-DO: Time valve cooldown check irl and make sure its 60 seconds (not 50), 

// ----- Time config -----
#define UTC_OFFSET (-7 * 3600)   // adjust if needed

EzoBoard orpSensors[3] = { // consider changing hardcoded values later 
  EzoBoard(98,  "ORP"),
  EzoBoard(100, "ORP"),
  EzoBoard(102, "ORP")
};

EzoBoard phSensors[3] = {
  EzoBoard(99,  "PH"),
  EzoBoard(101, "PH"),
  EzoBoard(103, "PH")
};

// ----- Valves -----
Mcp mcp;

// ----- UI -----
Lcd lcd(0x27, &Wire1);

// ----- SD Logger -----
SdLogger sd;   

// ----- Timers -----
Timer probeTimer(TimingIntervals::PROBE_READ_INTERVAL);
Timer sdLogTimer(TimingIntervals::SD_LOG_INTERVAL);
Timer phCountResetTimer(TimingIntervals::PH_RESET_WINDOW);
Timer orpCountResetTimer(TimingIntervals::ORP_RESET_WINDOW);
Timer valveCooldownTimerPh(TimingIntervals::VALVE_COOLDOWN);
Timer valveCooldownTimerOrp(TimingIntervals::VALVE_COOLDOWN);

ActionTimer cooldown[6] = { // test 
  ActionTimer(TimingIntervals::VALVE_COOLDOWN),   // 1  PH0
  ActionTimer(TimingIntervals::VALVE_COOLDOWN),   // 2  ORP0
  ActionTimer(TimingIntervals::VALVE_COOLDOWN),   // 3  PH1
  ActionTimer(TimingIntervals::VALVE_COOLDOWN),   // 4  ORP1
  ActionTimer(TimingIntervals::VALVE_COOLDOWN),   // 5  PH2
  ActionTimer(TimingIntervals::VALVE_COOLDOWN)    // 6  ORP2
};




// ----- Config state & Menu -----
ConfigState config;
Menu menu(&config, phSensors, orpSensors, lcd);

// ----- Membership Tracker -----
bool queued[6] = {false};

// ----- Forward decls -----
void handleProbeReads(ConfigState& config);
void handlePhValve();
void handleOrpValve();

void setup()
{
  // Serial optional:
  // Serial.begin(9600);

  // I2C
  Wire.begin(); // SDA = GPIO4, A3, SCL = GPIO11, A4
  Wire.setClock(100000);

  Wire1.begin(PinConfigurations::LCD_PIN_SDA, PinConfigurations::LCD_PIN_SCL, 100000);   // 100 kHz is fine

  const bool mcpReady = mcp.begin();

  // LCD
  lcd.init();

  // SPI must come BEFORE SD.begin()
  SPI.begin();

  // Set system time base (no NTP host)
  configTime(UTC_OFFSET, 0, "");

  // --- SD init: explicit CS pin ---
  // used a conservative SPI freq for stability; can raise later.
  const bool sdReady = sd.begin(PinConfigurations::SD_CHIP_SELECT ,1000000);

  // Build-time RTC seed
  sd.setTimeFromBuild();

  if (!mcpReady || !sdReady)
  {
    lcd.clear();
    lcd.setCursor(COL_LEFT, ROW_TITLE);
    lcd.print(!mcpReady ? "MCP init failed" : "SD init failed");
    lcd.setCursor(COL_LEFT, ROW_1);
    lcd.print(!mcpReady && !sdReady ? "SD init failed" : "Check wiring");
  }

  // First log line
  sd.log(config, "BOOT");
}

void loop() // cooldown is 50 seconds rather than 1 minute // if ph is started in 4 (below threshold, no trigger), 1 min timer on boot an issue? 
{
  if (probeTimer.isReady())
    handleProbeReads(config);


  if (!config.valvesDisabled)
  {
    // PH
    if (config.phValues[0] < Thresholds::PH_MINIMUM && !queued[0] && cooldown[0].done()) 
    {
      cooldown[0].start();
      mcp.enqueue(0);
      queued[0] = true;
    }
    if (config.phValues[1] < Thresholds::PH_MINIMUM && !queued[2] && cooldown[2].done()) 
    {
      cooldown[2].start();
      mcp.enqueue(2);
      queued[2] = true;
    }
    if (config.phValues[2] < Thresholds::PH_MINIMUM && !queued[4] && cooldown[4].done()) 
    {
      cooldown[4].start();
      mcp.enqueue(4);
      queued[4] = true;
    }

    // ORP
    if (config.orpValues[0] < Thresholds::ORP_MINIMUM && !queued[1] && cooldown[1].done()) 
    {
      cooldown[1].start();
      mcp.enqueue(1);
      queued[1] = true;
    }
    if (config.orpValues[1] < Thresholds::ORP_MINIMUM && !queued[3] && cooldown[3].done()) 
    {
      cooldown[3].start();
      mcp.enqueue(3);
      queued[3] = true;
    }
    if (config.orpValues[2] < Thresholds::ORP_MINIMUM && !queued[5] && cooldown[5].done()) 
    {
      cooldown[5].start();
      mcp.enqueue(5);
      queued[5] = true;
    }
  }
  mcp.update(queued);


  if (sdLogTimer.isReady())
    sd.log(config);  

  if (!menu.isActive() && menu.joystick.isPressed())
    menu.enter();

  if (menu.isActive()) 
  {
    menu.update();
    menu.draw();
  }
}

void handleProbeReads(ConfigState& config)
{
  for (int i = 0; i < 3; ++i) 
  {
    config.phValues[i]  = phSensors[i].read();
    config.orpValues[i] = orpSensors[i].read();
  }

  if (!config.lcdCleared) // find cleaner way to do this
  {
    lcd.clear();
    config.lcdCleared = true;
  }
  if (!menu.isActive())
    lcd.printData(config);
}
