#include <Wire.h>
#include "Config.h"
#include "Valve.h"
#include "EzoBoard.h"
#include "Lcd.h"
#include "Sd.h"
#include "Timer.h"
#include "Menu.h"

// Probes
EzoBoard orpSensor(98, "ORP");
EzoBoard phSensor(99, "PH");

// Valves
Valve phValve(PinConfigurations::PH_VALVE_PIN, 10000); // Open for 10 seconds
Valve orpValve(PinConfigurations::ORP_VALVE_PIN, 4000); // Open for 4 seconds 
// Recall the open time is counted as cooldown waiting period

// Timers
Timer probeTimer(TimingIntervals::PROBE_READ_INTERVAL);
Timer sdLogTimer(TimingIntervals::SD_LOG_INTERVAL);
Timer orpFlushTimer(TimingIntervals::HOUR_INTERVAL);
Timer phCountResetTimer(TimingIntervals::PH_RESET_WINDOW);
Timer orpCountResetTimer(TimingIntervals::ORP_RESET_WINDOW);
Timer valveCooldownTimer(TimingIntervals::VALVE_COOLDOWN);


// Single runtime config
ConfigState config;
// Consider only passing needed vars to functions instead of full struct 

// Features: time thing, debounce function, non blocking messages for menu switches, throw lcd error class

void setup()
{
  Serial.begin(9600);              // Init serial comms at 9600 baud
  Wire.begin();                    // Init I2C
  Wire.setClock(400000);           // Set I2C speed to 400 kHz (Fast Mode)

  pinMode(PinConfigurations::PH_VALVE_PIN, OUTPUT);
  pinMode(PinConfigurations::ORP_VALVE_PIN, OUTPUT);
  pinMode(PinConfigurations::PIN_SW, INPUT_PULLUP);

  initLcd();
  // lcd.setCursor(COL_LEFT, ROW_1); forgot what this does 
  initSD();
  // lcd.setCursor(COL_LEFT, ROW_2); Idk why this was here

  configTime(UTC_OFFSET, 0, ""); // This should be either computer time or last counted time, whichever is "higher"
  setTimeFromBuild(); // This too 
}

void loop() // minimize delay() and cut down on non modularized logic
{ 
  // Periodic sensor reads
  if (probeTimer.isReady()) handleProbeReads(config);
  // Periodic SD logging
  if (sdLogTimer.isReady()) logToSD(config);
  // PH Valve Control
  if ((config.phValue < Thresholds::PH_MINIMUM) && valveCooldownTimer.isReady()) handlePhValve(); 
  // ORP Valve Control
  if ((config.orpValue < Thresholds::ORP_MINIMUM) && valveCooldownTimer.isReady()) orpValve.open();
  // Hourly ORP flush
  if (orpFlushTimer.isReady()) orpValve.open();

  phValve.update();
  orpValve.update();
  toggleMenu(config, phSensor, orpSensor); // consider some demodularization so you dont have to hold sw
}



// tmp... make task class? 
void handleProbeReads(ConfigState& config)
{
  config.phValue = phSensor.read();
  config.orpValue = orpSensor.read();

  // Look into fixes for this lcd.cleared check... 
  if (!config.lcdCleared)
  {
    lcd.clear();
    config.lcdCleared = true;
  }
  printData(config);
}

void handlePhValve()
{
  phValve.open();

  if (phCountResetTimer.isReady())
  {
    config.phValveActivationCount = 1;
  }
  else
  {
    config.phValveActivationCount++;
  }

  if (config.phValveActivationCount >= TimingIntervals::PH_VALVE_MAX_ACTIVATIONS)
  {
    displayWarning(config);
    config.phValveActivationCount = 0;
  }
}

void handleOrpValve()
{
  orpValve.open();
  if (orpCountResetTimer.isReady())
  {
    config.orpValveActivationCount = 1;
  }
  else
  {
    config.orpValveActivationCount++;
  }

  if (config.orpValveActivationCount >= TimingIntervals::ORP_VALVE_MAX_ACTIVATIONS)
  {
    displayWarning(config);
    config.orpValveActivationCount = 0;
  }
}
// tmp 




