#include <Wire.h>
#include <SPI.h>
#include "Config.h"
#include "Valve.h"
#include "EzoBoard.h"
#include "Lcd.h"
#include "Sd.h"
#include "Timer.h"
#include "Menu.h"

// TO-DO: Time valve cooldown check irl and make sure its 60 seconds (not 50), 

// ----- Time config -----
#define UTC_OFFSET (-7 * 3600)   // adjust if needed

// ----- Probes -----
EzoBoard orpSensor(98, "ORP");
EzoBoard phSensor(99, "PH");

// ----- Valves -----
Valve phValve(PinConfigurations::PH_VALVE_PIN, 10000);   // Open for 10 seconds
Valve orpValve(PinConfigurations::ORP_VALVE_PIN, 4000);  // Open for 4 seconds

// ----- UI -----
Lcd lcd;

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
Menu menu(&config, &phSensor, &orpSensor, lcd);

// ----- Forward decls -----
void handleProbeReads(ConfigState& config);
void handlePhValve();
void handleOrpValve();

void setup()
{
  // Serial optional:
  // Serial.begin(115200);

  // I2C
  Wire.begin();
  Wire.setClock(400000);

  // IO
  pinMode(PinConfigurations::PH_VALVE_PIN, OUTPUT);
  pinMode(PinConfigurations::ORP_VALVE_PIN, OUTPUT);
  digitalWrite(PinConfigurations::PH_VALVE_PIN, LOW); // Ensure they are off initially 
  digitalWrite(PinConfigurations::ORP_VALVE_PIN, LOW);

  // LCD
  lcd.init();

  // SPI must come BEFORE SD.begin()
  SPI.begin();

  // Set system time base (no NTP host)
  configTime(UTC_OFFSET, 0, "");

  // --- SD init: explicit CS pin ---
  // use a conservative SPI freq for stability; can raise later.
  sd.begin(/*csPin=*/10, /*spiHz=*/1000000);

  // Build-time RTC seed
  sd.setTimeFromBuild();

  // First log line
  sd.log(config, "BOOT");
}

void loop() // cooldown is 50 seconds rather than 1 minute // if ph is started in 4 (below threshold, no trigger), 1 min timer on boot an issue? 
{
  phValve.update();
  orpValve.update();
  if (probeTimer.isReady())
    handleProbeReads(config);

  if (sdLogTimer.isReady())
    sd.log(config);  

  if ((config.phValue < Thresholds::PH_MINIMUM) && valveCooldownTimerPh.isReady())
    handlePhValve();

  if ((config.orpValue < Thresholds::ORP_MINIMUM) && valveCooldownTimerOrp.isReady())
    handleOrpValve();

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
  config.phValue  = phSensor.read();
  config.orpValue = orpSensor.read();

  if (!config.lcdCleared) 
  {
    lcd.clear();
    config.lcdCleared = true;
  }
  if (!menu.isActive())
    lcd.printData(config);
}

void handlePhValve() 
{
  phValve.open();
  valveCooldownTimerPh.reset();  

  if (phCountResetTimer.isReady())
  {
    config.phValveActivationCount = 1;
    phCountResetTimer.reset();  
  }
  else
    config.phValveActivationCount++;

  if (config.phValveActivationCount >= TimingIntervals::PH_VALVE_MAX_ACTIVATIONS) 
  {
    // menu.displayError("pH buffer not reacting");
    config.phValveActivationCount = 0;
  }
}

void handleOrpValve()
{
  orpValve.open();
  valveCooldownTimerOrp.reset();  

  if (orpCountResetTimer.isReady())
  {
    config.orpValveActivationCount = 1;
    orpCountResetTimer.reset();
  }
  else
    config.orpValveActivationCount++;

  if (config.orpValveActivationCount >= TimingIntervals::ORP_VALVE_MAX_ACTIVATIONS) 
  {
    // menu.displayError("ORP buffer not reacting");
    config.orpValveActivationCount = 0;
  }
}
