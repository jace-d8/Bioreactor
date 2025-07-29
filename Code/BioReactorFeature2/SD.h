#pragma once
#include <SPI.h>
#include <SD.h>

#define UTC_OFFSET (-7 * 3600)

void setTimeFromBuild();
void logToSD(String message = "");
