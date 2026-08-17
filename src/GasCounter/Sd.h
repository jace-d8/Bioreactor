// ============================================================================
// Sd.h
//
// PLAIN-ENGLISH SUMMARY:
// This defines the SdLogger class: everything to do with writing data to
// the SD card. It has two jobs here (one fewer than the bioreactor
// project's version, which also stored probe calibration data -- there
// are no probes in this project, so that whole piece has been removed):
//   1) Every so often, write a row of all 6 channels' cumulative gas
//      volume + trigger count ("data logging").
//   2) Write one-off timestamped event messages, like "Ch3 gas trigger
//      #12, 12.00 mL total" or "Ch3 rate error: too many triggers/min"
//      ("message logging").
//
// IMPORTANT: the recovery/flush machinery below (tryRecover_, the
// split-flush strategy in writeRow_) is NOT related to probes or EZO
// boards in any way -- it exists purely because SD cards themselves can
// occasionally fail to write (a worn card, a brief electrical glitch, the
// card's own internal garbage-collection pauses). Since this project
// still logs continuously to SD for potentially long, unattended runs,
// this reliability logic is kept exactly as it was in the bioreactor
// project, just with the probe-calibration-specific code removed.
// ============================================================================

#pragma once
#include <SPI.h>
// The library for "SPI," the wiring protocol the SD card physically uses
// to talk to the microcontroller (different from the I2C protocol the
// LCD and MCP23X17 use).
#include <SD.h>
// Arduino's built-in library for reading/writing files on an SD card.
#include "Config.h"

class SdLogger {
public:
  SdLogger() = default;
  // "= default" means "just use the compiler's automatic, do-nothing
  // constructor" -- all the real setup happens in begin() below, called
  // separately once the SD card hardware is ready.

  bool begin(uint8_t csPin, uint32_t spiHz = 1000000);
  // Call once at startup to actually initialize the SD card.
  //   csPin - which pin is wired to the card's "chip select" line
  //   spiHz - how fast to talk to the card, in Hz; defaults to 1 MHz if
  //           not specified.
  // Returns true if the card was found and is ready to use.

  void end();
  // Cleanly closes the currently-open log file and shuts down SD access.

  void setTimeFromBuild();
  // Sets this board's internal clock using the date/time the program was
  // compiled -- a simple approximation of "now," since this hardware has
  // no other way to know the real date/time without an internet
  // connection.

  // Writes one CSV row of every channel's cumulative gas volume + trigger
  // count. Call periodically (see TimingIntervals::SD_LOG_INTERVAL).
  void logData(const ConfigState& config);

  // Primary overload — takes a plain C string to avoid heap allocation.
  // All internal code uses this path.
  void logMessage(const char* message);
  // Writes one timestamped text message to the log file (used for events
  // like gas triggers and rate errors).

  // Convenience overload for any caller that already holds a String.
  void logMessage(const String& message) { logMessage(message.c_str()); }
  // A second version, for the rare case a caller has an Arduino String
  // instead of plain text -- converts it and hands off to the version
  // above. Defined right here (rather than in Sd.cpp) since it's just one
  // line -- this is called an "inline" function.

  // Returns false if the SD card has encountered a write error that has
  // not yet been recovered. The main sketch uses this to drive the LCD
  // error indicator on ROW_3.
  bool isHealthy() const { return ready_ && !sdError_; }
  // True only if the card initialized successfully (ready_) AND hasn't
  // hit an unrecovered write failure (sdError_ is false).

private:
  // ── Everything below is internal bookkeeping only SdLogger itself uses ──

  File     dataFile_;
  // The currently-open log file handle, which rows of gas data and event
  // messages get written into.

  uint8_t  csPin_  = 0;
  uint32_t spiHz_  = 1000000;
  // Remember the pin/speed settings begin() was originally called with,
  // so if the SD connection needs to be fully reset later (see
  // tryRecover_ below), we don't need those values passed in again.

  // Stack buffer for the active log filename ("/LOG9999.CSV" = 13 chars + NUL).
  char currentFilename_[16] = {};
  // The name of whichever log file is currently being written to (a new
  // one is created each time the device starts up, numbered so old logs
  // are never overwritten).

  // True after any write failure; cleared on successful recovery.
  bool sdError_ = false;

  String makeNextFilename_();
  // Works out the next unused log filename (LOG0001.CSV, LOG0002.CSV,
  // etc.) so each power-on gets its own fresh file.

  // Low-level write helper used by logData and logMessage.
  // flushAfterWrite = true  → flush immediately after writing (message rows).
  // flushAfterWrite = false → flush only if enough time has elapsed since
  //                           the last flush (routine data rows).
  bool writeRow_(const char* row, bool flushAfterWrite);
  // The shared "actually write one line of text to the log file" helper
  // that both logData() and logMessage() ultimately call.

  // Attempts to restore a valid dataFile_ handle after a write failure.
  // Stage 1: close and reopen the existing log file.
  // Stage 2: full SD reinit, then reopen.
  bool tryRecover_();
  // Automatic self-healing: if a write ever fails, this is called to try
  // to get logging working again without needing a human to intervene.

  unsigned long lastFlushMs_ = 0;
  // Tracks the last time the file was actually flushed to the physical
  // card, so routine data rows only flush occasionally (see writeRow_'s
  // comments in Sd.cpp for why).

  bool runDiagnostics_{false};
  void diag_(const char* line);
  // An optional diagnostic-logging helper (writes extra debug lines to a
  // separate file when runDiagnostics_ is turned on) -- useful when
  // troubleshooting SD card problems, otherwise stays quiet.
  bool ready_ = false;
  // Whether begin() has successfully completed and the card is usable.
};
