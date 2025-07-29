#pragma once
#include <SPI.h>
#include <SD.h>
#include "Config.h"

#define UTC_OFFSET (-7 * 3600)

void setTimeFromBuild();
void logToSD(ConfigState& config, String message = "");
