// ============================================================================
// GasChannel.cpp
//
// PLAIN-ENGLISH SUMMARY:
// The step-by-step implementation behind GasChannel.h. The function to
// study closely is update() near the bottom -- everything else exists to
// support it. Pay particular attention to the ORDER of operations inside
// update()'s trigger-handling block: the rate limit is checked BEFORE
// anything is committed (valve opened, gas counted), which is a
// deliberate fix over an earlier version of this logic that checked the
// rate AFTER already opening the valve and counting the gas -- see the
// comment right at that point below for the full story.
//
// begin() is also written to work unchanged with either the SST Sensing
// Optomax LLC200D3SH sensor this project ships with by default, or
// DFRobot's SEN0368 capacitive sensor (see capacitive_sensor_variant/ in
// the project root) -- see begin()'s comment on
// PinConfigurations::LEVEL_SENSOR_MODE_PIN for how.
// ============================================================================

#include "GasChannel.h"
#include "Sd.h"
#include <stdio.h>
// Gives us snprintf(), used in logEvent_()/countTrigger_() to build log
// message text.

GasChannel::GasChannel(ConfigState* config, Mcp* mcp, SdLogger* sd)
  : config_(config), mcp_(mcp), sd_(sd) {}
// Constructor: just remember the three pointers we were given. Nothing
// else needs setting up here -- prevSensorState_, valveOpen_, etc. are
// all zero-initialized automatically by their "= {}" in GasChannel.h.

void GasChannel::begin()
{
  // MODE pin setup -- only relevant for sensors that have one (e.g. the
  // DFRobot SEN0368 capacitive variant). PinConfigurations::
  // LEVEL_SENSOR_MODE_PIN is -1 (a sentinel meaning "not wired") for the
  // Optomax sensor this file ships with by default, so this block is
  // skipped entirely in that case -- see Config.h's comment on that
  // constant. This is what lets THIS SAME GasChannel.cpp work correctly
  // for either sensor without editing it: swapping sensors only ever
  // means swapping Config.h.
  if (PinConfigurations::LEVEL_SENSOR_MODE_PIN >= 0)
  {
    pinMode(PinConfigurations::LEVEL_SENSOR_MODE_PIN, OUTPUT);
    digitalWrite(PinConfigurations::LEVEL_SENSOR_MODE_PIN, HIGH);
    // Driving MODE HIGH once here (and never touching it again) sets a
    // capacitive sensor's "running = 1" state, per DFRobot's own example
    // code -- see capacitive_sensor_variant/Config.h for the full
    // explanation. A single shared pin drives all 6 sensors' MODE inputs
    // in parallel, so this only runs once, not once per channel.
  }

  for (int c = 0; c < ChannelMappings::CHANNEL_COUNT; ++c)
    pinMode(PinConfigurations::LEVEL_SENSOR_PIN[c], INPUT);
    // Tell the Arduino hardware "these 6 pins will be used to READ a
    // signal, not send one." Must happen before digitalRead() will give
    // sensible results on those pins.
}

bool GasChannel::readSensor_(int pin) const
{
  const bool raw = (digitalRead(pin) == HIGH);
  // Read the pin's raw electrical state. "raw" is true if the pin
  // currently reads HIGH.
  return LevelSensorConfig::ACTIVE_HIGH ? raw : !raw;
  // If ACTIVE_HIGH is true (HIGH means liquid detected), just return the
  // raw reading as-is. Otherwise ("!raw" -- NOT raw), flip it, since for
  // this sensor LOW actually means "liquid detected" (the LLC200D3SH's
  // default wiring, per Config.h's comment). This one line is what lets
  // Config.h support both possible sensor output polarities with a
  // single flag, without needing different code for each.
}

// fmt must contain exactly one %d (the 1-based channel number). SdLogger's
// logMessage() timestamps every row itself (see Sd.cpp), so every call
// here is automatically recorded with a timestamp on the SD card.
void GasChannel::logEvent_(const char* fmt, int channelId)
{
  if (!sd_) return;
  // Defensive check: if somehow no SdLogger was provided, just skip
  // logging rather than crash.
  char msg[56];
  snprintf(msg, sizeof(msg), fmt, channelId + 1);
  // Build the actual message text. "fmt" is something like "Ch%d rate
  // error..." -- snprintf replaces the "%d" with a real number.
  // "channelId + 1" converts our internal 0-based channel numbering (0-5)
  // into the 1-based numbering a human expects to read ("Ch1", not "Ch0").
  sd_->logMessage(msg);
}

// Called for EVERY rising edge this channel sees (whether or not it ends
// up counting as legitimate gas production) -- records the timestamp in
// the rate-limit history and reports back whether doing so has now
// pushed this channel over its per-minute limit. Deliberately does NOT
// touch gasVolumeML/triggerCount or open the valve -- see update() for
// why that decision has to wait until AFTER this check.
bool GasChannel::recordTrigger_(int channelId)
{
  int& count = triggerHistoryCount_[channelId];
  // "int&" creates a REFERENCE to triggerHistoryCount_[channelId] --
  // count is now just another name for that exact same number, not a
  // copy, so "count++" below directly changes the real array entry.
  if (count < TimingIntervals::MAX_TRIGGER_HISTORY)
  {
    triggerTimes_[channelId][count++] = millis();
    // Still room left in the history -- store this trigger's timestamp
    // at the next free slot, then increase count by one ("count++" means
    // "use count's current value, then add 1 afterward").
  }
  else
  {
    for (int i = 1; i < TimingIntervals::MAX_TRIGGER_HISTORY; ++i)
      triggerTimes_[channelId][i - 1] = triggerTimes_[channelId][i];
    triggerTimes_[channelId][TimingIntervals::MAX_TRIGGER_HISTORY - 1] = millis();
    // History is full -- "shift" every entry one slot toward the start
    // (discarding the very oldest), then put this new trigger's timestamp
    // in the now-empty last slot. This keeps the history at a fixed size
    // while always remembering the most recent triggers.
  }
  // Same rolling-history technique as the bioreactor project's
  // recordValveTrigger() in BioReactor.ino, just per-channel here instead
  // of per-valve.

  // BUG FIX: the channel should lock out once it has triggered MORE THAN
  // config_->maxTrigPerMin times in a minute (see Config.h's comment on
  // TRIGGER_RATE_WINDOW_MS, which quotes the spec directly: "more than
  // 29 times in a minute"; GasChannel.h's class comment uses the same
  // word, "exceed"). An earlier version of this function used
  // "limit = config_->maxTrigPerMin" directly, which actually locked the
  // channel on its maxTrigPerMin-TH trigger (e.g. the 29th, when the
  // default is 29) rather than the (maxTrigPerMin+1)-th (the 30th) --
  // off by one against the stated spec, since triggering exactly 29
  // times in a minute is not "more than 29." Using "maxTrigPerMin + 1"
  // here fixes that: the channel is now only locked once a trigger
  // arrives that pushes the count of triggers within the trailing minute
  // to maxTrigPerMin + 1, i.e. strictly more than maxTrigPerMin.
  const int limit = config_->maxTrigPerMin + 1;
  if (count < limit) return false;
  // Haven't even recorded as many total triggers as the limit allows yet
  // -- can't possibly have exceeded the per-minute rate.

  const unsigned long oldest = triggerTimes_[channelId][count - limit];
  // Look back exactly "limit" triggers ago (e.g. if maxTrigPerMin is 29,
  // limit is 30, so this looks at the 30th-most-recent trigger, counting
  // the one just recorded above).
  return (millis() - oldest < TimingIntervals::TRIGGER_RATE_WINDOW_MS);
  // If that trigger happened less than 1 minute ago, the last "limit"
  // (maxTrigPerMin + 1) triggers -- including this one -- all happened
  // within the last minute, i.e. more than maxTrigPerMin triggers in a
  // minute. Too fast.
}

void GasChannel::countTrigger_(int channelId)
// Called only for a trigger that passed the rate check -- actually counts
// it as real gas production (updates the running total) and logs it.
{
  config_->triggerCount[channelId]++;
  config_->gasVolumeML[channelId] += config_->calFactor;
  // "Displayed gas production = trigger count * calibrated factor" --
  // rather than recomputing this multiplication every time it's
  // displayed, we accumulate it incrementally here (adding one factor's
  // worth per trigger), which is equivalent and slightly cheaper.

  char msg[56];
  snprintf(msg, sizeof(msg), "Ch%%d gas trigger #%lu, %.2f mL total",
           (unsigned long)config_->triggerCount[channelId],
           config_->gasVolumeML[channelId]);
  // "%%d" is how you write a LITERAL "%d" inside a format string -- so
  // this line builds the text "Ch%d gas trigger #12, 12.00 mL total"
  // (with a real "%d" still embedded in it), leaving that placeholder for
  // logEvent_() to fill in with the actual channel number afterward. This
  // two-stage substitution lets this function fill in the count/total
  // immediately while still deferring the channel-number formatting to
  // the single shared logEvent_() helper.
  logEvent_(msg, channelId);
}

void GasChannel::lockChannel_(int channelId)
// Called for a trigger that FAILED the rate check -- locks the channel
// without counting this trigger as gas production and without ever
// opening its valve for it. The trigger's timestamp was already recorded
// by recordTrigger_() above (so the history stays accurate for
// diagnostics/future checks), but the event itself is treated as a fault
// signal, not real data.
{
  if (!config_->channelErrorLocked[channelId])
  // Only log the error message the FIRST time this happens for this
  // channel -- once locked, the outer guard in update() (see below) stops
  // this function from being reached again until a human resets it, but
  // this check is a small extra safety net against double-logging.
  {
    config_->channelErrorLocked[channelId] = true;
    logEvent_("Ch%d rate error: too many triggers/min", channelId);
  }
}

void GasChannel::update()
{
  const unsigned long now = millis();

  for (int c = 0; c < ChannelMappings::CHANNEL_COUNT; ++c)
  {
    const bool cur = readSensor_(PinConfigurations::LEVEL_SENSOR_PIN[c]);
    config_->sensorState[c] = cur;
    // Publish the live reading to the shared ConfigState for display/
    // diagnostics, regardless of lock/valve state below.

    // Auto-close the valve once its pulse duration has elapsed. Runs
    // every cycle, even while locked, so a valve can never be left open
    // indefinitely.
    if (valveOpen_[c] && (now - valveStarted_[c] >= (unsigned long)config_->pulseSec * 1000UL))
    {
      mcp_->setValve(c, false);
      valveOpen_[c] = false;
    }

    const bool risingEdge = cur && !prevSensorState_[c];
    // "cur is true AND the previous reading was false" -- exactly the
    // "level rising to a certain point" transition described in the spec.

    // GUARANTEE: no new trigger (and therefore no new gas count) can ever
    // register while valveOpen_[c] is true. Since valveOpen_[c] only goes
    // false again once the full pulseSec duration has elapsed (see the
    // auto-close block above), this makes the entire pulse window a hard
    // "cooldown" -- both the valve and the counting logic are locked out
    // together for exactly config->pulseSec seconds after every trigger,
    // with no gap where a second count could sneak in. On top of that,
    // because triggering also requires a genuine risingEdge (the sensor
    // must have gone back to "no liquid" and then risen again), a channel
    // can't immediately re-fire the instant the cooldown ends either --
    // it has to see one real new rise first.
    if (risingEdge && !config_->channelErrorLocked[c] && !config_->valvesDisabled && !valveOpen_[c])
    // Only actually consider this if: this is a genuine rising edge, this
    // channel isn't rate-limited-locked, the global valvesDisabled pause
    // isn't active, and this channel isn't currently in its cooldown
    // (valve open).
    {
      const bool rateExceeded = recordTrigger_(c);
      // Check the rate BEFORE opening the valve or counting any gas, so
      // the offending trigger itself never gets treated as both an error
      // and legitimate data at the same time. An EARLIER version of this
      // logic opened the valve and counted the gas FIRST, then checked
      // the rate afterward and force-closed the valve if it turned out to
      // be one-too-many -- which meant the very trigger that caused the
      // lockout still got added to gasVolumeML/triggerCount as if it were
      // real production, and its valve would blip open for a fraction of
      // a millisecond before immediately snapping shut again. Checking
      // first, as done here, avoids both problems: the valve for an
      // offending trigger never opens at all, and nothing gets counted
      // that's simultaneously being flagged as a fault.

      if (rateExceeded)
      {
        lockChannel_(c);
        // Valve stays closed; this trigger is not counted as gas
        // production -- it's the trigger that revealed the malfunction
        // (or sensor noise), not real data.
      }
      else
      {
        mcp_->setValve(c, true);
        valveOpen_[c]    = true;
        valveStarted_[c] = now;
        countTrigger_(c);
      }
    }

    prevSensorState_[c] = cur;
    // Always update the previous-reading memory, every cycle, regardless
    // of whether this cycle triggered anything -- this is what makes the
    // NEXT cycle's edge detection correct.
  }
}

void GasChannel::resetErrors()
{
  for (int c = 0; c < ChannelMappings::CHANNEL_COUNT; ++c)
  {
    config_->channelErrorLocked[c] = false;
    triggerHistoryCount_[c] = 0;
    for (int i = 0; i < TimingIntervals::MAX_TRIGGER_HISTORY; ++i)
      triggerTimes_[c][i] = 0;
    // Clear the rate-limit lock and its supporting history -- but
    // deliberately leave config_->gasVolumeML[c] and
    // config_->triggerCount[c] untouched, since those represent real
    // measured gas production, not error-tracking state. A channel that
    // gets reset simply goes back to Monitoring with a clean rate-limit
    // history, ready to be re-armed by the next genuine rising edge --
    // it never needed its valve force-closed here, since (see update()
    // above) a channel can only ever become locked while its valve is
    // already closed in the first place.
  }
}
