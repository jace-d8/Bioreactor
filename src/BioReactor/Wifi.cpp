#include "Wifi.h"
#include "Config.h"
#include "Sd.h"
#include <time.h>
#include <sys/time.h>

namespace {

// Short, human-readable text for the ESP32's wl_status_t codes. Included in
// the failure log so the specific cause is visible without a serial
// monitor. Some are especially diagnostic:
//   - "SSID not found": the radio never even saw "WSU Guest" broadcasting.
//     Usually means out of range, hidden SSID, or a typo — not a whitelist
//     issue, since MAC whitelisting normally blocks *after* association,
//     not the AP's beacon visibility.
//   - "connect failed" / "disconnected" soon after joining: consistent with
//     a MAC-based network access control system deauthenticating a device
//     whose MAC hasn't been approved yet.
const char* wifiStatusText(wl_status_t s)
{
  switch (s)
  {
    case WL_IDLE_STATUS:     return "idle";
    case WL_NO_SSID_AVAIL:   return "SSID not found";
    case WL_SCAN_COMPLETED:  return "scan done";
    case WL_CONNECTED:       return "connected";
    case WL_CONNECT_FAILED:  return "connect failed";
    case WL_CONNECTION_LOST: return "connection lost";
    case WL_DISCONNECTED:    return "disconnected";
    default:                 return "unknown";
  }
}

}  // namespace

void WifiTime::begin()
{
  Serial.begin(115200);

  // Puts the radio into station mode. This alone makes macAddress() valid
  // and takes on the order of milliseconds — it does NOT wait to associate
  // with a network, so this adds no meaningful delay to boot.
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();   // clear any auto-reconnect state from a previous run

  // Fire the join attempt immediately so the radio gets a head start while
  // the rest of setup() (I2C init, EZO calibration restore, etc.) runs —
  // but deliberately do NOT start the timeout clock here. See timerArmed_.
  WiFi.begin(WifiConfig::SSID);
  state_      = State::Connecting;
  timerArmed_ = false;

  Serial.print("[WiFi] joining \"");
  Serial.print(WifiConfig::SSID);
  Serial.println("\"...");
}

void WifiTime::startConnect_()
{
  WiFi.begin(WifiConfig::SSID);   // open network: no password argument
  state_        = State::Connecting;
  stateStartMs_ = millis();
  timerArmed_   = true;

  Serial.print("[WiFi] retry: joining \"");
  Serial.print(WifiConfig::SSID);
  Serial.println("\"...");
}

void WifiTime::update(SdLogger& sd)
{
  switch (state_)
  {
    case State::Connecting:
    {
      // Arm the timeout clock on the first update() call rather than back
      // in begin(), so time spent blocked earlier in setup() never eats
      // into this budget. See timerArmed_ comment in Wifi.h.
      if (!timerArmed_)
      {
        stateStartMs_ = millis();
        timerArmed_   = true;
      }

      if (WiFi.status() == WL_CONNECTED)
      {
        Serial.print("[WiFi] joined, IP=");
        Serial.println(WiFi.localIP());

        // Remember the current clock so it can be restored if this attempt
        // fails, then force the clock to an obviously-fake placeholder
        // (2000-01-01) before starting SNTP. Without this, the success
        // check below ("is the year plausible?") would always pass
        // instantly — the clock already held a plausible year from the
        // build-time fallback (or a previous sync) before any real NTP
        // reply had arrived, so it looked synced even when no reply ever
        // came back. That's why every reported sync time was identical
        // regardless of the real time: it was just re-reporting the
        // pre-existing fallback value, never real NTP data.
        time(&preSyncEpoch_);
        struct timeval sentinel = { 946684800, 0 };  // 2000-01-01 00:00:00 UTC
        settimeofday(&sentinel, nullptr);

        configTime(WifiConfig::UTC_OFFSET_SEC, 0, WifiConfig::NTP_SERVER);
        state_        = State::WaitingForTime;
        stateStartMs_ = millis();
      }
      else if (millis() - stateStartMs_ >= WifiConfig::CONNECT_TIMEOUT_MS)
      {
        char reason[40];
        snprintf(reason, sizeof(reason), "WiFi join timeout: %s", wifiStatusText(WiFi.status()));
        finishAttempt_(false, sd, reason);
      }
      break;
    }

    case State::WaitingForTime:
    {
      struct tm ti;
      // 10 ms cap: getLocalTime() polls internally, so this only blocks a
      // few ms at a time and only while actively waiting for a reply.
      if (getLocalTime(&ti, 10) && (ti.tm_year + 1900) > 2023)
      {
        finishAttempt_(true, sd);
      }
      else if (millis() - stateStartMs_ >= WifiConfig::TIME_WAIT_MS)
      {
        // No real reply arrived — restore whatever time we had before this
        // attempt (build time, or the last successful sync) rather than
        // leaving the clock stuck at the year-2000 sentinel.
        struct timeval restore = { preSyncEpoch_, 0 };
        settimeofday(&restore, nullptr);

        // Joined the network fine but got no NTP reply — a common cause is
        // a captive portal (guest networks often block all traffic,
        // including NTP, until a browser-based login is completed).
        finishAttempt_(false, sd, "no NTP reply");
      }
      break;
    }

    case State::Cooldown:
    {
      const unsigned long interval = lastSyncOk_
        ? WifiConfig::RESYNC_INTERVAL_MS
        : WifiConfig::RETRY_INTERVAL_MS;

      if (millis() - lastAttemptMs_ >= interval)
        startConnect_();
      break;
    }
  }
}

void WifiTime::finishAttempt_(bool ok, SdLogger& sd, const char* failReason)
{
  // Release the radio between attempts rather than holding an association
  // (or a failed connect state) for hours between resyncs.
  WiFi.disconnect(true);

  lastSyncOk_    = ok;
  lastAttemptMs_ = millis();
  state_         = State::Cooldown;

  if (ok)
  {
    everSynced_    = true;
    lastSyncEpoch_ = time(nullptr);   // system clock now reflects real NTP time
    Serial.println("[WiFi] NTP sync OK");
    sd.logMessage("NTP time sync OK");
  }
  else
  {
    Serial.print("[WiFi] attempt failed: ");
    Serial.println(failReason);

    char msg[48];
    snprintf(msg, sizeof(msg), "NTP time sync failed (%s)", failReason);
    sd.logMessage(msg);
  }
}

void WifiTime::syncStatus(char* buf, size_t bufLen) const
{
  if (!everSynced_)
  {
    snprintf(buf, bufLen, "Not synced yet");
    return;
  }

  struct tm ti;
  localtime_r(&lastSyncEpoch_, &ti);

  int hour12 = ti.tm_hour % 12;
  if (hour12 == 0) hour12 = 12;
  const char ampm    = (ti.tm_hour < 12) ? 'A' : 'P';
  const int  year2   = (ti.tm_year + 1900) % 100;

  snprintf(buf, bufLen, "sync@%d:%02d%c %d/%d/%02d",
           hour12, ti.tm_min, ampm,
           ti.tm_mon + 1, ti.tm_mday, year2);
}
