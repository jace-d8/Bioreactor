//
// Created by Jace Dunn on 7/28/25.
//

#pragma once



#include <SPI.h>
#include <SD.h>
#define UTC_OFFSET (-7 * 3600)  // For Pacific Time (PST). Use 0 for UTC, adjust as needed

void setTimeFromBuild();

void logToSD(String message = "");