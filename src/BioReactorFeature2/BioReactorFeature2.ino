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

Valve phValve(PinConfigurations::PH_VALVE_PIN, 10000); // Open for 10 seconds
Valve orpValve(PinConfigurations::ORP_VALVE_PIN, 4000); // Open for 4 seconds 

Lcd lcd; 
SdLogger sd(lcd);

Timer probeTimer(TimingIntervals::PROBE_READ_INTERVAL);
Timer sdLogTimer(TimingIntervals::SD_LOG_INTERVAL);
Timer orpFlushTimer(TimingIntervals::HOUR_INTERVAL);
Timer phCountResetTimer(TimingIntervals::PH_RESET_WINDOW);
Timer orpCountResetTimer(TimingIntervals::ORP_RESET_WINDOW);
Timer valveCooldownTimer(TimingIntervals::VALVE_COOLDOWN);

ConfigState config;

Menu menu(&config, &phSensor, &orpSensor, lcd); // Pass LCD reference to menu

void setup()
{
    Serial.begin(9600);
    Wire.begin();
    Wire.setClock(400000);

    pinMode(PinConfigurations::PH_VALVE_PIN, OUTPUT);
    pinMode(PinConfigurations::ORP_VALVE_PIN, OUTPUT);

    lcd.init();                  // Initialize LCD via class
    configTime(UTC_OFFSET, 0, ""); 
    sd.setTimeFromBuild(); 
}

void loop()
{ 
    if (probeTimer.isReady()) handleProbeReads(config);
    if (sdLogTimer.isReady()) sd.log(config);
    if ((config.phValue < Thresholds::PH_MINIMUM) && valveCooldownTimer.isReady()) handlePhValve(); 
    if ((config.orpValue < Thresholds::ORP_MINIMUM) && valveCooldownTimer.isReady()) handleOrpValve();
    if (orpFlushTimer.isReady()) orpValve.open();


    // think more on this later
    static bool lastPressed = false;                 // remembers prior state
    bool pressed = !digitalRead(PinConfigurations::PIN_SW); // active-low

    if (!menu.isActive()) 
    {
        if (pressed && !lastPressed)
        {
            menu.enter();
        }
    }
    if (menu.isActive()) 
    {
        menu.update();
        menu.draw();
    }
    lastPressed = pressed;
}

void handleProbeReads(ConfigState& config)
{
    config.phValue = phSensor.read();
    config.orpValue = orpSensor.read();

    if (!config.lcdCleared)
    {
        lcd.clear();
        config.lcdCleared = true;
    }
    if (!menu.isActive()) lcd.printData(config); 
}

void handlePhValve()
{
    phValve.open();

    if (phCountResetTimer.isReady())
        config.phValveActivationCount = 1;
    else
        config.phValveActivationCount++;

    if (config.phValveActivationCount >= TimingIntervals::PH_VALVE_MAX_ACTIVATIONS)
    {
        menu.displayError("pH buffer not reacting");
        config.phValveActivationCount = 0;
    }
}

void handleOrpValve()
{
    orpValve.open();

    if (orpCountResetTimer.isReady())
        config.orpValveActivationCount = 1;
    else
        config.orpValveActivationCount++;

    if (config.orpValveActivationCount >= TimingIntervals::ORP_VALVE_MAX_ACTIVATIONS)
    {
        menu.displayError("ORP buffer not reacting");
        config.orpValveActivationCount = 0;
    }
}
