#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <time.h>

class SdLogger;

// Keeps the system clock correct using NTP over the open university guest
// WiFi, without ever blocking the main loop for more than a few ms.
//
// Why this exists: the ESP32 has no battery-backed RTC, so on every power
// cycle it starts from whatever setTimeFromBuild() sets it to (i.e. the
// firmware compile time). WifiTime tries in the background to join WiFi and
// pull the real time from NTP; if it succeeds, the system clock is corrected
// and stays correct even across brief WiFi outages. It keeps re-syncing
// periodically to catch RTC drift on long runs, and retries sooner after a
// failed attempt (e.g. WiFi briefly out of range).
//
// The whole thing is driven by begin() (call once from setup()) and update()
// (call every loop() iteration) — a small internal state machine advances by
// one step per call, so nothing here can stall valve control or SD logging.
class WifiTime
{
public:
  // Initializes the WiFi radio in station mode (so the MAC address is
  // available immediately, with no association required) and starts the
  // first connect+sync attempt in the background. Non-blocking.
  void begin();

  // Advances the connect/sync state machine by one step. Call every loop()
  // iteration. Logs a single message via sd (if the SD card is ready) each
  // time an attempt finishes, success or failure, so sync history shows up
  // in the data log.
  void update(SdLogger& sd);

  // Safe to call at any time after begin() — does not require an active
  // connection, since the radio's MAC is fixed hardware, not something
  // assigned by the network.
  String macAddress() const { return WiFi.macAddress(); }

  // True while currently associated with the WiFi network. Reflects live
  // radio status, not whether a sync has ever completed.
  bool isConnected() const { return WiFi.status() == WL_CONNECTED; }

  // True once at least one NTP sync has succeeded since boot.
  bool timeIsSynced() const { return everSynced_; }

  // Manually starts a connect+sync attempt right now, regardless of the
  // normal resync/retry schedule — cancels any attempt already in
  // progress. Used by the "Sync Now" menu action, since the radio is
  // deliberately disconnected between scheduled syncs (see finishAttempt_),
  // so WiFi status will read "Disconnected" most of the time by design.
  void syncNow() { startConnect_(); }

  // Writes a short human-readable sync status into buf, e.g.
  // "synced@9:39P8/19/26" after a successful sync, or "Not synced yet"
  // if none has completed. Sized to fit a 20-column LCD row.
  void syncStatus(char* buf, size_t bufLen) const;

private:
  enum class State { Connecting, WaitingForTime, Cooldown };
  State state_ = State::Connecting;

  unsigned long stateStartMs_  = 0;
  unsigned long lastAttemptMs_ = 0;
  bool lastSyncOk_ = false;
  bool everSynced_ = false;
  time_t lastSyncEpoch_ = 0;   // set when a sync last succeeded

  // The very first connect attempt is kicked off from begin(), at the top of
  // setup() — but setup() can then block for tens of seconds afterward (e.g.
  // EZO calibration restore does delay(1800) per chunk, across up to 6
  // probes) before loop() ever runs and update() gets called for the first
  // time. If the timeout clock started in begin(), that blocked time would
  // silently eat the whole connect timeout budget, so the first status check
  // in loop() would almost always find it already "timed out" — regardless
  // of whether the radio actually joined the network in the background.
  // timerArmed_ defers starting the clock until the first real update() call.
  bool timerArmed_ = false;

  time_t preSyncEpoch_ = 0;   // clock value saved just before forcing the sentinel

  void startConnect_();
  void finishAttempt_(bool ok, SdLogger& sd, const char* failReason = "");
};
