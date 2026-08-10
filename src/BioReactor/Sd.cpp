#include "Sd.h"
#include "EzoBoard.h"
#include <time.h>
#include <sys/time.h>
#include <string.h>

// ── diag_ ────────────────────────────────────────────────────────────────────
void SdLogger::diag_(const char* line)
{
  if (runDiagnostics_)
  {
    File d = SD.open("/SD_DIAG.TXT", FILE_APPEND);
    if (d) { d.println(line); d.close(); }
  }
}

// ── begin ─────────────────────────────────────────────────────────────────────
bool SdLogger::begin(uint8_t csPin, uint32_t spiHz)
{
  // Store for use by tryRecover_() so a full SD reinit can be attempted
  // without requiring parameters to be threaded through every call site.
  csPin_ = csPin;
  spiHz_ = spiHz;
  sdError_     = false;
  lastFlushMs_ = 0;

  if (!SD.begin(csPin, SPI, spiHz))
  {
    diag_("BEGIN: SD.begin FAIL");
    ready_ = false;
    return false;
  }

  // Build the next available filename and store it as a plain char array
  // so no heap String object persists after begin() returns.
  String fname = makeNextFilename_();
  strncpy(currentFilename_, fname.c_str(), sizeof(currentFilename_) - 1);
  currentFilename_[sizeof(currentFilename_) - 1] = '\0';

  dataFile_ = SD.open(currentFilename_, FILE_WRITE);

  if (!dataFile_)
  {
    diag_("BEGIN: open log FAIL");
    ready_ = false;
    return false;
  }

  dataFile_.println("timestamp,pH1,ORP1,pH2,ORP2,pH3,ORP3,message");
  dataFile_.flush();
  ready_ = true;

  loadCalibrationFile_();
  return true;
}

// ── end ───────────────────────────────────────────────────────────────────────
void SdLogger::end()
{
  if (dataFile_)
  {
    dataFile_.flush();
    dataFile_.close();
  }
  ready_ = false;
}

// ── makeNextFilename_ ─────────────────────────────────────────────────────────
String SdLogger::makeNextFilename_()
{
  char base[16];
  for (int i = 1; i <= 9999; i++)
  {
    snprintf(base, sizeof(base), "LOG%04d.CSV", i);
    String path = String("/") + base;
    if (!SD.exists(path)) return path;
  }
  return String("/LOG9999.CSV");
}

// ── tryRecover_ ───────────────────────────────────────────────────────────────
// Called whenever a write failure is detected.  Attempts two escalating
// recovery stages so that a transient SPI glitch or SD card stall does not
// permanently silence the logger for the rest of the run.
//
// Stage 1 — reopen the existing log file in append mode.
//   Handles: SPI timeout or transient error that invalidated the file handle
//   while the underlying SD card and filesystem are still intact.
//
// Stage 2 — full SD.end() + SD.begin() reinit with up to 3 attempts, then
//   reopen.  Handles: SPI bus lock-up or SD card that entered a partially-
//   committed state after an interrupted write and needs a deselect/re-select
//   sequence to reset its internal state machine.
//   The 700 ms first-attempt delay covers the worst-case internal erase cycle
//   on consumer SD cards.  Subsequent attempts use 300 ms each.  This totals
//   up to ~1.3 s of blocking — well below the 5 s ESP32 task watchdog ceiling.
//
// If both stages fail after all retries, ready_ and sdError_ are set
// accordingly so the LCD alert stays on and no further log calls are attempted.
bool SdLogger::tryRecover_()
{
  // Always close the potentially corrupt handle first.
  if (dataFile_) dataFile_.close();

  // ── Stage 1: reopen ──────────────────────────────────────────────────────
  dataFile_ = SD.open(currentFilename_, FILE_APPEND);
  if (dataFile_)
  {
    sdError_ = false;
    diag_("RECOVER: stage 1 OK (file reopened)");
    return true;
  }

  // ── Stage 2: full SD reinit with retry ───────────────────────────────────
  diag_("RECOVER: stage 1 FAIL, attempting SD reinit");
  SD.end();

  bool sdOk = false;
  for (int attempt = 0; attempt < 3 && !sdOk; ++attempt)
  {
    // First attempt: 700 ms to cover worst-case SD garbage-collection stall.
    // Subsequent attempts: 300 ms each (card has already had time to settle).
    delay(attempt == 0 ? 700 : 300);
    sdOk = SD.begin(csPin_, SPI, spiHz_);
  }

  if (!sdOk)
  {
    diag_("RECOVER: SD.begin FAIL after 3 attempts — logging disabled");
    ready_   = false;
    sdError_ = true;
    return false;
  }

  dataFile_ = SD.open(currentFilename_, FILE_APPEND);
  if (!dataFile_)
  {
    diag_("RECOVER: file reopen after reinit FAIL — logging disabled");
    ready_   = false;
    sdError_ = true;
    return false;
  }

  sdError_ = false;
  diag_("RECOVER: stage 2 OK (SD reinit + file reopened)");
  return true;
}

// ── writeRow_ ─────────────────────────────────────────────────────────────────
// All CSV writes funnel through here so error detection, recovery, and retry
// are handled in exactly one place.
//
// Flush strategy — the root cause of the 6-hour logging failure was calling
// dataFile_.flush() on every data write (every 2 s with the original Config.h,
// every 30 s with the updated one).  flush() forces two FAT sector writes plus
// a directory entry update on the physical SD card.  Consumer SD cards stall
// for up to several seconds during internal garbage-collection cycles, and at
// high flush frequencies one of those stalls will eventually exceed the ESP32's
// 5-second task watchdog threshold, crashing the system without any log entry.
//
// The fix splits flush behaviour by row type:
//   flushAfterWrite = true  (message rows: valve triggers, boot, calibration)
//     → flush immediately.  These are low-frequency, real-time events whose
//       exact timestamp matters and that must survive a power loss.
//   flushAfterWrite = false (data rows: routine probe readings every 30 s)
//     → flush only when DATA_FLUSH_INTERVAL_MS has elapsed since the last
//       flush.  Data rows between flushes are held in the FatFS write cache;
//       up to DATA_FLUSH_INTERVAL_MS of readings could be lost on a hard
//       power cut, which is an acceptable trade-off given that the valve event
//       messages (the safety-critical data) are always flushed immediately.
//
// Recovery marker: if the file handle was invalid on entry and tryRecover_()
// succeeds, a timestamped marker row is inserted before the intended row so
// any gap in the data is visible in the CSV.  The marker is always flushed
// immediately regardless of flushAfterWrite so it is committed even if the
// next write also fails.
//
// Retry: one retry is attempted after a failed write before giving up.
static const unsigned long DATA_FLUSH_INTERVAL_MS = 5UL * 60UL * 1000UL; // 5 min

bool SdLogger::writeRow_(const char* row, bool flushAfterWrite)
{
  if (!ready_) return false;

  // ── Recovery if handle is invalid ────────────────────────────────────────
  bool justRecovered = false;
  if (!dataFile_)
  {
    if (!tryRecover_()) return false;
    justRecovered = true;
  }

  // ── Recovery marker ───────────────────────────────────────────────────────
  if (justRecovered)
  {
    time_t ts = time(nullptr);
    struct tm* ti = localtime(&ts);
    if (ti)
    {
      char marker[80];
      snprintf(marker, sizeof(marker),
        "%04d-%02d-%02d %02d:%02d:%02d,,,,,,,[SD logging recovered]\n",
        ti->tm_year + 1900, ti->tm_mon + 1, ti->tm_mday,
        ti->tm_hour, ti->tm_min, ti->tm_sec);
      dataFile_.print(marker);
    }
    // Always flush the recovery marker immediately so it is committed even if
    // the next write also fails.
    dataFile_.flush();
    lastFlushMs_ = millis();
  }

  // ── Write attempt ─────────────────────────────────────────────────────────
  const size_t expected = strlen(row);
  size_t written = dataFile_.print(row);

  if (written == expected)
  {
    sdError_ = false;

    // Decide whether to flush now or defer.
    const unsigned long now = millis();
    if (flushAfterWrite || (now - lastFlushMs_ >= DATA_FLUSH_INTERVAL_MS))
    {
      dataFile_.flush();
      lastFlushMs_ = now;
    }
    return true;
  }

  // ── Write failed — one retry after recovery ───────────────────────────────
  sdError_ = true;
  dataFile_.close();

  if (!tryRecover_()) return false;

  written = dataFile_.print(row);
  if (written == expected)
  {
    dataFile_.flush();
    lastFlushMs_ = millis();
    sdError_ = false;
    return true;
  }

  // Retry also failed.
  sdError_ = true;
  dataFile_.close();
  return false;
}

// ── logData ───────────────────────────────────────────────────────────────────
void SdLogger::logData(const ConfigState& config)
{
  if (!ready_) return;

  time_t now = time(nullptr);
  struct tm* ti = localtime(&now);
  if (!ti) return;

  // Build the row on the stack — no heap allocation.
  char row[80];
  snprintf(row, sizeof(row),
    "%04d-%02d-%02d %02d:%02d:%02d,%.3f,%.0f,%.3f,%.0f,%.3f,%.0f,\n",
    ti->tm_year + 1900, ti->tm_mon + 1, ti->tm_mday,
    ti->tm_hour, ti->tm_min, ti->tm_sec,
    config.phValues[0], config.orpValues[0],
    config.phValues[1], config.orpValues[1],
    config.phValues[2], config.orpValues[2]);

  writeRow_(row, false);  // defer flush; data rows commit on the 5-minute timer
}

// ── valveName_ ────────────────────────────────────────────────────────────────
const char* SdLogger::valveName_(int valveId) const
{
  switch (valveId)
  {
    case 0: return "pH1";
    case 1: return "ORP1";
    case 2: return "pH2";
    case 3: return "ORP2";
    case 4: return "pH3";
    case 5: return "ORP3";
    default: return "UNKNOWN";
  }
}

// ── setTimeFromBuild ──────────────────────────────────────────────────────────
void SdLogger::setTimeFromBuild()
{
  struct tm tm{};
  if (strptime(__DATE__ " " __TIME__, "%b %d %Y %H:%M:%S", &tm))
  {
    time_t t = mktime(&tm);
    struct timeval now = { .tv_sec = t, .tv_usec = 0 };
    settimeofday(&now, nullptr);
  }
}

// ── logMessage ────────────────────────────────────────────────────────────────
// Accepts a plain C string to avoid any heap allocation in the hot path.
// The String overload in Sd.h delegates here via .c_str().
void SdLogger::logMessage(const char* message)
{
  if (!ready_) return;

  time_t now = time(nullptr);
  struct tm* ti = localtime(&now);
  if (!ti) return;

  // Build the message row on the stack.
  char row[100];
  snprintf(row, sizeof(row),
    "%04d-%02d-%02d %02d:%02d:%02d,,,,,,,%s\n",
    ti->tm_year + 1900, ti->tm_mon + 1, ti->tm_mday,
    ti->tm_hour, ti->tm_min, ti->tm_sec,
    message);

  writeRow_(row, true);   // flush immediately: message rows are real-time events
}

// ── logValveOn / logValveLocked ───────────────────────────────────────────────
// Use stack char arrays instead of String concatenation so that repeated valve
// events cannot fragment the ESP32 heap allocator over a long run.
void SdLogger::logValveOn(int valveId)
{
  char msg[32];
  snprintf(msg, sizeof(msg), "%s valve triggered", valveName_(valveId));
  logMessage(msg);
}

void SdLogger::logValveLocked(int valveId)
{
  char msg[32];
  snprintf(msg, sizeof(msg), "%s valve locked", valveName_(valveId));
  logMessage(msg);
}

// ── phBufferName_ / parseBufferName_ ─────────────────────────────────────────
const char* SdLogger::phBufferName_(int bufferIdx)
{
  switch (bufferIdx)
  {
    case 0: return "low";
    case 1: return "mid";
    case 2: return "high";
    default: return "";
  }
}

bool SdLogger::parseBufferName_(const char* name, int& bufferIdx)
{
  if (!name) return false;
  if (strcmp(name, "low")  == 0) { bufferIdx = 0; return true; }
  if (strcmp(name, "mid")  == 0) { bufferIdx = 1; return true; }
  if (strcmp(name, "high") == 0) { bufferIdx = 2; return true; }
  return false;
}

// ── loadCalibrationFile_ ──────────────────────────────────────────────────────
void SdLogger::loadCalibrationFile_()
{
  memset(&calCache_, 0, sizeof(calCache_));

  if (!ready_ || !SD.exists(CalibrationStorage::FILE_PATH))
  {
    calCache_.loaded = true;
    return;
  }

  File calFile = SD.open(CalibrationStorage::FILE_PATH, FILE_READ);
  if (!calFile)
  {
    calCache_.loaded = true;
    return;
  }

  bool headerSkipped = false;
  while (calFile.available())
  {
    String line = calFile.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;
    if (!headerSkipped)
    {
      headerSkipped = true;
      if (line.startsWith("type"))
        continue;
    }

    int probeIdx  = -1;
    int bufferIdx = -1;
    char type[8]    = {};
    char point[8]   = {};
    char active[4]  = {};
    char payload[CalibrationStorage::EXPORT_PAYLOAD_MAX] = {};

    const int parsed = sscanf(
      line.c_str(), "%7[^;];%d;%7[^;];%3[^;];%159[^\n]",
      type, &probeIdx, point, active, payload);

    if (parsed < 4 || probeIdx < 0 || probeIdx > 2)
      continue;

    const bool isActive = (active[0] == '1');

    if (strcmp(type, "PH") == 0)
    {
      if (!parseBufferName_(point, bufferIdx))
        continue;

      calCache_.phActive[probeIdx][bufferIdx] = isActive;
      if (isActive && parsed >= 5)
        strncpy(calCache_.phPayload[probeIdx][bufferIdx], payload,
                CalibrationStorage::EXPORT_PAYLOAD_MAX - 1);
    }
    else if (strcmp(type, "ORP") == 0)
    {
      calCache_.orpActive[probeIdx] = isActive;
      if (isActive && parsed >= 5)
        strncpy(calCache_.orpPayload[probeIdx], payload,
                CalibrationStorage::EXPORT_PAYLOAD_MAX - 1);
    }
  }

  calFile.close();
  calCache_.loaded = true;
}

// ── writeCalibrationFile_ ─────────────────────────────────────────────────────
// No longer const: needs to flush dataFile_ before modifying the FAT.
//
// The flush before SD.remove() ensures the data log's FAT chain is fully
// committed before calibration file operations begin.  Without this, a power
// loss during the SD.remove() → open → write → close sequence could corrupt
// the FAT in a state where both CALIB.CSV and the data log are partially
// written, potentially invalidating the open dataFile_ handle (Issue 4).
bool SdLogger::writeCalibrationFile_()
{
  if (!ready_) return false;

  // Flush the open data log so its FAT entry is fully committed before we
  // start modifying directory entries for CALIB.CSV.
  if (dataFile_) dataFile_.flush();

  SD.remove(CalibrationStorage::FILE_PATH);
  File calFile = SD.open(CalibrationStorage::FILE_PATH, FILE_WRITE);
  if (!calFile) return false;

  calFile.println("type;probe;point;active;export_data");

  for (int probe = 0; probe < 3; ++probe)
  {
    for (int buffer = 0; buffer < 3; ++buffer)
    {
      if (!calCache_.phActive[probe][buffer]) continue;
      calFile.printf("PH;%d;%s;1;%s\n", probe, phBufferName_(buffer),
                     calCache_.phPayload[probe][buffer]);
    }

    if (calCache_.orpActive[probe])
      // "-" placeholder keeps the sscanf field count consistent in loadCalibrationFile_
      calFile.printf("ORP;%d;-;1;%s\n", probe, calCache_.orpPayload[probe]);
  }

  calFile.close();
  return true;
}

// ── restoreCalibrations ───────────────────────────────────────────────────────
void SdLogger::restoreCalibrations(EzoBoard* phSensors, EzoBoard* orpSensors, ConfigState& config)
{
  if (!ready_ || !calCache_.loaded)
    return;

  for (int probe = 0; probe < 3; ++probe)
  {
    config.phBufferCalibrated[probe][0] = calCache_.phActive[probe][0];
    config.phBufferCalibrated[probe][1] = calCache_.phActive[probe][1];
    config.phBufferCalibrated[probe][2] = calCache_.phActive[probe][2];
    config.orpCalibrated[probe] = calCache_.orpActive[probe];

    // Import only the highest-index active buffer: each export snapshot captures
    // the full probe calibration state at that moment (e.g. a mid export already
    // contains the low point), so the highest-index snapshot is the most complete.
    int lastActiveBuffer = -1;
    for (int buffer = 0; buffer < 3; ++buffer)
      if (calCache_.phActive[probe][buffer]) lastActiveBuffer = buffer;

    if (lastActiveBuffer >= 0)
      phSensors[probe].importCalibration(calCache_.phPayload[probe][lastActiveBuffer]);

    if (calCache_.orpActive[probe])
      orpSensors[probe].importCalibration(calCache_.orpPayload[probe]);
  }

  logMessage("Calibration restore complete");
}

// ── savePhBufferCalibration ───────────────────────────────────────────────────
bool SdLogger::savePhBufferCalibration(int probeIdx, int bufferIdx, const char* exportPayload)
{
  if (!ready_ || probeIdx < 0 || probeIdx > 2 || bufferIdx < 0 || bufferIdx > 2)
    return false;
  if (!exportPayload || exportPayload[0] == '\0')
    return false;

  if (!calCache_.loaded)
    loadCalibrationFile_();

  calCache_.phActive[probeIdx][bufferIdx] = true;
  strncpy(calCache_.phPayload[probeIdx][bufferIdx], exportPayload,
          CalibrationStorage::EXPORT_PAYLOAD_MAX - 1);
  calCache_.phPayload[probeIdx][bufferIdx][CalibrationStorage::EXPORT_PAYLOAD_MAX - 1] = '\0';

  return writeCalibrationFile_();
}

// ── clearPhBufferCalibration ──────────────────────────────────────────────────
bool SdLogger::clearPhBufferCalibration(int probeIdx, int bufferIdx)
{
  if (!ready_ || probeIdx < 0 || probeIdx > 2 || bufferIdx < 0 || bufferIdx > 2)
    return false;

  if (!calCache_.loaded)
    loadCalibrationFile_();

  calCache_.phActive[probeIdx][bufferIdx] = false;
  calCache_.phPayload[probeIdx][bufferIdx][0] = '\0';
  return writeCalibrationFile_();
}

// ── clearPhProbeCalibration ───────────────────────────────────────────────────
bool SdLogger::clearPhProbeCalibration(int probeIdx)
{
  if (!ready_ || probeIdx < 0 || probeIdx > 2)
    return false;

  if (!calCache_.loaded)
    loadCalibrationFile_();

  for (int buffer = 0; buffer < 3; ++buffer)
  {
    calCache_.phActive[probeIdx][buffer] = false;
    calCache_.phPayload[probeIdx][buffer][0] = '\0';
  }

  return writeCalibrationFile_();
}

// ── saveOrpCalibration ────────────────────────────────────────────────────────
bool SdLogger::saveOrpCalibration(int probeIdx, const char* exportPayload)
{
  if (!ready_ || probeIdx < 0 || probeIdx > 2)
    return false;
  if (!exportPayload || exportPayload[0] == '\0')
    return false;

  if (!calCache_.loaded)
    loadCalibrationFile_();

  calCache_.orpActive[probeIdx] = true;
  strncpy(calCache_.orpPayload[probeIdx], exportPayload,
          CalibrationStorage::EXPORT_PAYLOAD_MAX - 1);
  calCache_.orpPayload[probeIdx][CalibrationStorage::EXPORT_PAYLOAD_MAX - 1] = '\0';

  return writeCalibrationFile_();
}

// ── clearOrpCalibration ───────────────────────────────────────────────────────
bool SdLogger::clearOrpCalibration(int probeIdx)
{
  if (!ready_ || probeIdx < 0 || probeIdx > 2)
    return false;

  if (!calCache_.loaded)
    loadCalibrationFile_();

  calCache_.orpActive[probeIdx] = false;
  calCache_.orpPayload[probeIdx][0] = '\0';
  return writeCalibrationFile_();
}
