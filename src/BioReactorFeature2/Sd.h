#pragma once
#include <SPI.h>
#include <SD.h>
#include "Config.h"
#include "Lcd.h"

#define UTC_OFFSET (-7 * 3600)

class SdLogger 
{
public:
    // Take the LCD reference explicitly; construct after Lcd in the sketch.
    SdLogger(Lcd& lcdRef);
    void setTimeFromBuild();
    void log(ConfigState& config, const String& message = "");
    Lcd& lcd;  // reference to Lcd instance

private:
    File dataFile_;
};
