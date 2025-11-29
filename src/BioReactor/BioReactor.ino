#include <Wire.h>
#include <SPI.h>
#include "Config.h"
#include "Mcp.h"
#include "EzoBoard.h"
#include "Lcd.h"
#include "Sd.h"
#include "Timer.h"
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

// ----- Config state & Menu -----
ConfigState config;
Menu menu(&config, phSensors, orpSensors, lcd);

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

  mcp.begin();

  // LCD
  lcd.init();

  // SPI must come BEFORE SD.begin()
  SPI.begin();

  // Set system time base (no NTP host)
  configTime(UTC_OFFSET, 0, "");

  // --- SD init: explicit CS pin ---
  // used a conservative SPI freq for stability; can raise later.
  sd.begin(PinConfigurations::SD_CHIP_SELECT ,1000000);

  // Build-time RTC seed
  sd.setTimeFromBuild();

  // First log line
  sd.log(config, "BOOT");
}

void loop() // cooldown is 50 seconds rather than 1 minute // if ph is started in 4 (below threshold, no trigger), 1 min timer on boot an issue? 
{
  // this will all need to be updated for 6 probes
  if (probeTimer.isReady())
    handleProbeReads(config);


  // PH
  if (config.phValues[0] < Thresholds::PH_MINIMUM) mcp.enqueue(1);
  if (config.phValues[1] < Thresholds::PH_MINIMUM) mcp.enqueue(3);
  if (config.phValues[2] < Thresholds::PH_MINIMUM) mcp.enqueue(5);

  // ORP
  if (config.orpValues[0] < Thresholds::ORP_MINIMUM) mcp.enqueue(2);
  if (config.orpValues[1] < Thresholds::ORP_MINIMUM) mcp.enqueue(4);
  if (config.orpValues[2] < Thresholds::ORP_MINIMUM) mcp.enqueue(6);
  
  mcp.update();


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
