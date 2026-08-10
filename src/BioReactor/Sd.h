#pragma once
#include <SPI.h>
#include <SD.h>
#include "Config.h"

class EzoBoard;

class SdLogger {
public:
  SdLogger() = default;

  bool begin(uint8_t csPin, uint32_t spiHz = 1000000);
  void end();
  void setTimeFromBuild();

  void logData(const ConfigState& config);

  // Primary overload — takes a plain C string to avoid heap allocation.
  // All internal code uses this path.
  void logMessage(const char* message);

  // Convenience overload for any caller that already holds a String.
  // Delegates to the const char* overload immediately so no heap String
  // object lives longer than this one-liner.
  void logMessage(const String& message) { logMessage(message.c_str()); }

  void logValveOn(int valveId);
  void logValveLocked(int valveId);

  // Returns false if the SD card has encountered a write error that has
  // not yet been recovered. BioReactor.ino uses this to drive the LCD
  // error indicator on ROW_3.
  bool isHealthy() const { return ready_ && !sdError_; }

  void restoreCalibrations(EzoBoard* phSensors, EzoBoard* orpSensors, ConfigState& config);
  bool savePhBufferCalibration(int probeIdx, int bufferIdx, const char* exportPayload);
  bool clearPhBufferCalibration(int probeIdx, int bufferIdx);
  bool clearPhProbeCalibration(int probeIdx);
  bool saveOrpCalibration(int probeIdx, const char* exportPayload);
  bool clearOrpCalibration(int probeIdx);

private:
  struct CalibrationCache
  {
    bool phActive[3][3] = {};
    char phPayload[3][3][CalibrationStorage::EXPORT_PAYLOAD_MAX] = {};
    bool orpActive[3] = {};
    char orpPayload[3][CalibrationStorage::EXPORT_PAYLOAD_MAX] = {};
    bool loaded = false;
  };

  File     dataFile_;
  CalibrationCache calCache_;

  // Stored at begin() so tryRecover_() can reinitialise without parameters.
  uint8_t  csPin_  = 0;
  uint32_t spiHz_  = 1000000;

  // Stack buffer for the active log filename ("/LOG9999.CSV" = 13 chars + NUL).
  // Stored as a char array to avoid any heap allocation after begin().
  char currentFilename_[16] = {};

  // True after any write failure; cleared on successful recovery.
  // Exposed via isHealthy() so BioReactor.ino can show an LCD alert.
  bool sdError_ = false;

  String makeNextFilename_();
  const char* valveName_(int valveId) const;
  static const char* phBufferName_(int bufferIdx);
  static bool parseBufferName_(const char* name, int& bufferIdx);

  void loadCalibrationFile_();

  // No longer const: needs to flush dataFile_ before FAT operations (Issue 4).
  bool writeCalibrationFile_();

  // Tracks the last time dataFile_.flush() was called so that routine data
  // rows are flushed at most once every DATA_FLUSH_INTERVAL_MS rather than
  // on every write.  Message rows (valve triggers, boot, calibration events)
  // always flush immediately via the flushAfterWrite flag.
  unsigned long lastFlushMs_ = 0;

  // Low-level write helper used by logData and logMessage.
  // flushAfterWrite = true  → flush immediately after writing (message rows).
  // flushAfterWrite = false → flush only if DATA_FLUSH_INTERVAL_MS has elapsed
  //                           since the last flush (routine data rows).
  bool writeRow_(const char* row, bool flushAfterWrite);

  // Attempts to restore a valid dataFile_ handle after a write failure.
  // Stage 1: close and reopen the existing log file (FILE_APPEND).
  // Stage 2: full SD.end() + SD.begin() reinit, then reopen.
  // Sets ready_ = false and sdError_ = true if both stages fail.
  bool tryRecover_();

  bool runDiagnostics_{false};
  void diag_(const char* line);
  bool ready_ = false;
};