#pragma once
#include <SPI.h>
#include <SD.h>
#include "Config.h"


class SdLogger {
public:
  SdLogger() = default;

  bool begin(uint8_t csPin, uint32_t spiHz = 1000000);

  void end();

  void log(struct ConfigState& config, const String& message = "");

  void setTimeFromBuild();

private:
  File dataFile_;
  String makeNextFilename_();
  bool runDiagnostics_{false};
  void diag_(const char* line);

  bool ready_ = false;
};
