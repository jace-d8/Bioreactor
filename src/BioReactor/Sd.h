#pragma once
#include <SPI.h>
#include <SD.h>
#include "Config.h"

class SdLogger {
public:
  SdLogger() = default;

  bool begin(uint8_t csPin, uint32_t spiHz = 1000000);
  void end();
  void setTimeFromBuild();

  void logData(const ConfigState& config);
  void logMessage(const String& message);
  void logValveOn(int valveId);
  void logValveLocked(int valveId);
  void logReactorEvent(int reactorId, const char* eventType, unsigned long durationMs);

private:
  File dataFile_;
  String makeNextFilename_();
  const char* valveName_(int valveId) const;

  bool runDiagnostics_{false};
  void diag_(const char* line);
  bool ready_ = false;
};