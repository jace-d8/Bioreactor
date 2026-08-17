// ============================================================================
// Sd.cpp
//
// PLAIN-ENGLISH SUMMARY:
// The implementation behind Sd.h. Compared to the bioreactor project's
// Sd.cpp, everything calibration-related is gone (CalibrationCache,
// restoreCalibrations, save/clearPhBufferCalibration, phBufferName_,
// parseBufferName_, loadCalibrationFile_, writeCalibrationFile_) since
// there are no probes to calibrate here. Everything else -- begin(), the
// two-stage tryRecover_(), the split-flush writeRow_(), logMessage(),
// setTimeFromBuild() -- is unchanged, because that machinery is about SD
// card hardware reliability, not about what data happens to be logged.
// ============================================================================

#include "Sd.h"
#include <time.h>
// Gives us time_t, struct tm, time(), localtime(), mktime(), strptime().
#include <sys/time.h>
// Gives us settimeofday(), used to actually set this board's clock.
#include <string.h>
// Gives us strncpy(), strlen().

// ── diag_ ────────────────────────────────────────────────────────────────────
void SdLogger::diag_(const char* line)
// Writes one line of DEBUGGING text to a separate file (SD_DIAG.TXT),
// completely independent of the normal log file. Does nothing at all
// unless runDiagnostics_ has been manually turned on (defaults to false).
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
  csPin_ = csPin;
  spiHz_ = spiHz;
  sdError_     = false;
  lastFlushMs_ = 0;

  if (!SD.begin(csPin, SPI, spiHz))
  // Try to actually mount the SD card's filesystem. If this fails
  // (missing card, wiring issue, corrupted card, etc.), give up.
  {
    diag_("BEGIN: SD.begin FAIL");
    ready_ = false;
    return false;
  }

  String fname = makeNextFilename_();
  strncpy(currentFilename_, fname.c_str(), sizeof(currentFilename_) - 1);
  currentFilename_[sizeof(currentFilename_) - 1] = '\0';
  // Copy the filename text out of the temporary String into our own
  // fixed-size currentFilename_ array, always safely terminated.

  dataFile_ = SD.open(currentFilename_, FILE_WRITE);
  if (!dataFile_)
  {
    diag_("BEGIN: open log FAIL");
    ready_ = false;
    return false;
  }

  dataFile_.println("timestamp,ch1_mL,ch1_n,ch2_mL,ch2_n,ch3_mL,ch3_n,ch4_mL,ch4_n,ch5_mL,ch5_n,ch6_mL,ch6_n,message");
  // Write the CSV header row (column titles) as the first line of this
  // fresh log file: a timestamp, then a volume+trigger-count pair for
  // each of the 6 channels, then a message column.
  dataFile_.flush();
  ready_ = true;
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
// Finds the first unused "LOGnnnn.CSV" filename, so every fresh power-on
// gets its own new log file rather than overwriting an old one.
{
  char base[16];
  for (int i = 1; i <= 9999; i++)
  {
    snprintf(base, sizeof(base), "LOG%04d.CSV", i);
    // "%04d" formats a number as at least 4 digits, padding with leading
    // zeros, e.g. i=7 becomes "0007".
    String path = String("/") + base;
    if (!SD.exists(path)) return path;
    // As soon as we find a filename that ISN'T already on the card, use it.
  }
  return String("/LOG9999.CSV");
  // Fallback if somehow all 9999 numbers are already taken.
}

// ── tryRecover_ ───────────────────────────────────────────────────────────────
// Called whenever a write failure is detected. Attempts two escalating
// recovery stages so a transient SPI glitch or SD card stall doesn't
// permanently silence the logger for the rest of the run.
//
// Stage 1 — reopen the existing log file in append mode.
//   Handles: a transient error that invalidated the file handle while the
//   underlying SD card and filesystem are still intact.
//
// Stage 2 — full SD reinit (up to 3 attempts), then reopen.
//   Handles: an SPI bus lock-up or a card that entered a partially-
//   committed state after an interrupted write. The 700 ms first-attempt
//   delay covers the worst-case internal erase cycle on consumer SD
//   cards; subsequent attempts use 300 ms each. This totals up to ~1.3 s
//   of blocking -- well below the ESP32's 5 s task watchdog ceiling.
//
// If both stages fail, ready_ and sdError_ are set accordingly so the LCD
// alert stays on and no further log calls are attempted.
bool SdLogger::tryRecover_()
{
  if (dataFile_) dataFile_.close();
  // Always close the potentially corrupt handle first.

  // ── Stage 1: reopen ──────────────────────────────────────────────────────
  dataFile_ = SD.open(currentFilename_, FILE_APPEND);
  if (dataFile_)
  {
    sdError_ = false;
    diag_("RECOVER: stage 1 OK (file reopened)");
    return true;
    // Just reopening the same file worked -- the SD card itself was fine,
    // only our file handle had gone stale. Cheapest possible recovery.
  }

  // ── Stage 2: full SD reinit with retry ───────────────────────────────────
  diag_("RECOVER: stage 1 FAIL, attempting SD reinit");
  SD.end();
  // Fully shut down the SD library's connection to the card, so the next
  // SD.begin() call starts completely fresh.

  bool sdOk = false;
  for (int attempt = 0; attempt < 3 && !sdOk; ++attempt)
  // Try up to 3 times, stopping early as soon as one attempt succeeds.
  {
    delay(attempt == 0 ? 700 : 300);
    sdOk = SD.begin(csPin_, SPI, spiHz_);
  }

  if (!sdOk)
  {
    diag_("RECOVER: SD.begin FAIL after 3 attempts — logging disabled");
    ready_   = false;
    sdError_ = true;
    return false;
    // Give up entirely -- isHealthy() will now report false, and the LCD
    // will show the "SD ERR" warning until the user power-cycles the
    // device (or the card is fixed/replaced).
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
// All CSV writes funnel through here so error detection, recovery, and
// retry are handled in exactly one place.
//
// Flush strategy -- flushing forces the data to actually be committed to
// the physical card (rather than sitting in a temporary memory buffer).
// Consumer SD cards can stall for up to several seconds during internal
// garbage-collection cycles, and flushing on every single write risks one
// of those stalls exceeding the ESP32's 5-second task watchdog threshold,
// crashing the system with no log entry to show for it. So:
//   flushAfterWrite = true  (message rows: gas triggers, rate errors,
//     BOOT) -> flush immediately. These are low-frequency, real-time
//     events whose exact timestamp matters and that must survive a power
//     loss.
//   flushAfterWrite = false (data rows: routine channel snapshots every
//     30 s) -> flush only when DATA_FLUSH_INTERVAL_MS has elapsed since
//     the last flush. Rows between flushes are held in the write cache;
//     up to that interval's worth of readings could be lost on a hard
//     power cut, an acceptable trade-off given that the actual trigger
//     events (the important data) are always flushed immediately.
//
// Recovery marker: if the file handle was invalid on entry and
// tryRecover_() succeeds, a timestamped marker row is inserted before the
// intended row so any gap in the data is visible in the CSV.
//
// Retry: one retry is attempted after a failed write before giving up.
static const unsigned long DATA_FLUSH_INTERVAL_MS = 5UL * 60UL * 1000UL; // 5 min
// "static" here (a variable outside any class, at file scope) means this
// constant is only visible within THIS .cpp file.

bool SdLogger::writeRow_(const char* row, bool flushAfterWrite)
{
  if (!ready_) return false;
  // If the SD card never initialized successfully, don't even try.

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
        "%04d-%02d-%02d %02d:%02d:%02d,,,,,,,,,,,,,[SD logging recovered]\n",
        ti->tm_year + 1900, ti->tm_mon + 1, ti->tm_mday,
        ti->tm_hour, ti->tm_min, ti->tm_sec);
      // 12 empty commas before the message text, matching the 12 data
      // columns (6 channels x volume+count) in the CSV header written in
      // begin() -- keeps this marker row lined up under the same columns
      // as every other row.
      dataFile_.print(marker);
    }
    dataFile_.flush();
    lastFlushMs_ = millis();
  }

  // ── Write attempt ─────────────────────────────────────────────────────────
  const size_t expected = strlen(row);
  size_t written = dataFile_.print(row);
  // dataFile_.print() returns how many characters it actually managed to
  // write -- comparing that against how many we EXPECTED (strlen(row)) is
  // how a partial/failed write is detected.

  if (written == expected)
  {
    sdError_ = false;
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
    // The retry, after recovery, succeeded.
  }

  sdError_ = true;
  dataFile_.close();
  return false;
  // Give up on this particular row.
}

// ── logData ───────────────────────────────────────────────────────────────────
void SdLogger::logData(const ConfigState& config)
// Writes one CSV row of all 6 channels' current cumulative gas volume +
// trigger count.
{
  if (!ready_) return;

  time_t now = time(nullptr);
  struct tm* ti = localtime(&now);
  if (!ti) return;
  // If for some reason the clock isn't working, skip this row entirely
  // rather than log a nonsense timestamp.

  char row[160];
  snprintf(row, sizeof(row),
    "%04d-%02d-%02d %02d:%02d:%02d,"
    "%.2f,%lu,%.2f,%lu,%.2f,%lu,%.2f,%lu,%.2f,%lu,%.2f,%lu,\n",
    ti->tm_year + 1900, ti->tm_mon + 1, ti->tm_mday,
    ti->tm_hour, ti->tm_min, ti->tm_sec,
    config.gasVolumeML[0], (unsigned long)config.triggerCount[0],
    config.gasVolumeML[1], (unsigned long)config.triggerCount[1],
    config.gasVolumeML[2], (unsigned long)config.triggerCount[2],
    config.gasVolumeML[3], (unsigned long)config.triggerCount[3],
    config.gasVolumeML[4], (unsigned long)config.triggerCount[4],
    config.gasVolumeML[5], (unsigned long)config.triggerCount[5]);
  // "%.2f" prints a decimal number with 2 digits after the decimal point
  // (used for mL volumes); "%lu" prints an unsigned long whole number
  // (used for trigger counts). This builds one complete CSV row matching
  // the header written in begin(): timestamp, then 6 pairs of
  // (volume, count), then a trailing comma before an (empty, for data
  // rows) message column.

  writeRow_(row, false);  // defer flush; data rows commit on the 5-minute timer
}

// ── setTimeFromBuild ──────────────────────────────────────────────────────────
void SdLogger::setTimeFromBuild()
// Sets the board's clock using the date/time the program was COMPILED --
// this board has no other way to know "now" without an internet
// connection or a battery-backed real-time clock chip.
{
  struct tm tm{};
  // "{}" makes sure every field starts at 0 rather than random leftover
  // memory.
  if (strptime(__DATE__ " " __TIME__, "%b %d %Y %H:%M:%S", &tm))
  // "__DATE__" and "__TIME__" are special built-in macros the COMPILER
  // fills in automatically with the date/time it compiled this file.
  // strptime() parses that text into the "tm" structure, returning
  // non-null (true-ish) if it succeeded.
  {
    time_t t = mktime(&tm);
    struct timeval now = { .tv_sec = t, .tv_usec = 0 };
    settimeofday(&now, nullptr);
    // Actually set the board's system clock to this value.
  }
}

// ── logMessage ────────────────────────────────────────────────────────────────
void SdLogger::logMessage(const char* message)
{
  if (!ready_) return;

  time_t now = time(nullptr);
  struct tm* ti = localtime(&now);
  if (!ti) return;

  char row[128];
  snprintf(row, sizeof(row),
    "%04d-%02d-%02d %02d:%02d:%02d,,,,,,,,,,,,,%s\n",
    ti->tm_year + 1900, ti->tm_mon + 1, ti->tm_mday,
    ti->tm_hour, ti->tm_min, ti->tm_sec,
    message);
  // Same timestamp format as logData(), but all 12 channel columns left
  // blank (since this is an EVENT message, not a sensor reading) and the
  // message text placed in the final column instead. There are exactly
  // 12 empty commas here, matching the 12 data columns in the header.

  writeRow_(row, true);   // flush immediately: message rows are real-time events
}
