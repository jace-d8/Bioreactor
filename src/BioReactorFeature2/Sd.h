#pragma once
#include <SPI.h>
#include <SD.h>
#include "Config.h"
#include "Lcd.h"

#define UTC_OFFSET (-7 * 3600)

class SdLogger 
{
public:
  SdLogger(Lcd& lcdRef, int csPin = SD_CHIP_SELECT);
  ~SdLogger();
  void setTimeFromBuild();
  void log(ConfigState& config, const String& message = "");
  void beginSession();
  Lcd& lcd;

private:
  File dataFile_;
  int cs_ = SD_CHIP_SELECT;
  String makeNextFilename_();
};
