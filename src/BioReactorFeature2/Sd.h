#pragma once
#include <SPI.h>
#include <SD.h>
#include "Config.h"

#define UTC_OFFSET (-7 * 3600)

class SdLogger 
{
public:
    SdLogger();
    void setTimeFromBuild();
    void log(ConfigState& config, const String& message = "");

private:
    File dataFile_;
};
