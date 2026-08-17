// ============================================================================
// GasChannel.h
//
// PLAIN-ENGLISH SUMMARY:
// This defines the GasChannel class -- the "brain" of the whole project,
// playing the same role the bioreactor project's FeedWaste class played
// there. One GasChannel object manages all 6 gas-counting channels at
// once (there's no per-channel object -- everything is arrays of 6
// instead, indexed 0-5).
//
// For each channel, every single pass through loop() this class:
//   1) Reads the level sensor.
//   2) Notices if it just transitioned from "no liquid" to "liquid" (a
//      RISING EDGE -- see below).
//   3) On that transition, opens the valve for a fixed pulse, and either
//      counts it as real gas production or -- if this channel has been
//      triggering too fast lately -- locks the channel out instead.
//
// WHY "RISING EDGE" AND NOT "WHENEVER LIQUID IS DETECTED"?
// If we counted every loop() pass where the sensor reads "liquid," a
// single physical rise-and-vent event (which takes a couple of seconds
// from start to finish) would get counted hundreds of times, since
// loop() runs far faster than that. Instead, this class remembers what
// the sensor read on the PREVIOUS pass, and only counts the exact moment
// it flips from false to true -- i.e., the instant the level first rises
// past the trigger point. This is a standard technique called "edge
// detection," and it's what makes "count one discrete event, no matter
// how long the sensor stays high afterward" actually work.
// ============================================================================

#pragma once
#include "Config.h"
#include "Mcp.h"

class SdLogger;
// "Forward declaration" -- tells the compiler "a class called SdLogger
// exists somewhere; I don't need its full details here, just enough to
// know I can hold a pointer to one." See Sd.h itself for the real
// definition. This avoids needing to #include the whole of Sd.h just for
// that.

// Drives all 6 gas-counting channels. Each channel independently:
//
//   - Watches its liquid level sensor for a RISING EDGE (transition from
//     "no liquid" to "liquid detected") -- this is what "the liquid level
//     rising to a certain point" means here. Counting on the edge (rather
//     than "every loop while liquid is detected") ensures one physical
//     rise-and-vent event is counted exactly once, even though the vent
//     pulse and drainage take a couple of seconds during which the sensor
//     may still read "liquid detected."
//   - On that edge, opens its valve for config->pulseSec seconds (no
//     queueing -- see Mcp.h) and adds config->calFactor mL to that
//     channel's running gas-volume total.
//   - While the valve is open (the pulseSec-long "cooldown"), that channel
//     is fully locked out: no new trigger can fire and no new gas count
//     can register, no matter what the sensor reads during that window.
//     A channel only becomes re-armable once BOTH (a) the full pulseSec
//     cooldown has elapsed, AND (b) the sensor has actually returned to
//     "no liquid" and then risen again -- so a single sustained "liquid
//     detected" reading can never be double-counted.
//   - Tracks how many times each channel has triggered in the trailing 1
//     minute; if that would exceed config->maxTrigPerMin, the OFFENDING
//     trigger itself is locked out too -- its valve never opens and it is
//     not added to the gas volume total, since a trigger that's flagged
//     as a rate-limit fault isn't legitimate data. The channel then stays
//     locked (no triggering, no counting) until cleared via Settings >
//     Reset Error.
//
// All 6 channels share the pulseSec/calFactor/maxTrigPerMin settings in
// ConfigState (rather than each having its own) -- this mirrors how the
// bioreactor project applied one pH/ORP threshold across all 3 reactors.
// If your channels need independently different settings later, the
// natural change is to make these arrays of 6 in ConfigState and add a
// per-channel edit screen, following the same pattern already used here.
class GasChannel
{
public:
  GasChannel(ConfigState* config, Mcp* mcp, SdLogger* sd);
  // Constructor: takes POINTERS (not copies!) to the one shared
  // ConfigState, the one shared Mcp (valve controller), and the one
  // shared SdLogger. Using pointers means this class always reads/writes
  // the SAME real objects the rest of the program uses -- e.g. when it
  // updates config->gasVolumeML[2], that change is immediately visible to
  // Lcd.cpp and Menu.cpp too, since they're all looking at the exact same
  // ConfigState in memory.

  // Configures the level-sensor pins as inputs. Call once from setup().
  void begin();

  // Reads all 6 sensors and advances every channel's logic. Call once per
  // loop() iteration, unconditionally.
  void update();

  // Clears every channel's rate-limit error lock and trigger history
  // (NOT the cumulative gas volume/trigger counts -- those are real
  // measured production data and are only ever reset by a power cycle).
  // Wired into the main .ino's resetErrors(), itself triggered by
  // Settings > Reset Error.
  void resetErrors();

private:
  ConfigState* config_;
  Mcp*         mcp_;
  SdLogger*    sd_;
  // The three pointers handed in through the constructor, remembered for
  // later use throughout this class's other functions.

  bool          prevSensorState_[ChannelMappings::CHANNEL_COUNT] = {};
  // The sensor reading from the PREVIOUS update() call, per channel --
  // comparing this against the current reading is how a rising edge is
  // detected. An ARRAY of 6 bools: prevSensorState_[0] is channel 1's
  // previous reading, prevSensorState_[1] is channel 2's, and so on.

  bool          valveOpen_[ChannelMappings::CHANNEL_COUNT]  = {};
  unsigned long valveStarted_[ChannelMappings::CHANNEL_COUNT] = {};
  // Whether each channel's valve is currently open, and when (in millis())
  // it opened -- used to auto-close it after config->pulseSec seconds.
  // Tracked here with plain millis() math (rather than as a fixed-duration
  // ActionTimer object) specifically because pulseSec is editable at
  // runtime -- an ActionTimer's duration is fixed once created, so it
  // can't follow a setting that might change while the program is running.

  unsigned long triggerTimes_[ChannelMappings::CHANNEL_COUNT][TimingIntervals::MAX_TRIGGER_HISTORY] = {};
  int           triggerHistoryCount_[ChannelMappings::CHANNEL_COUNT] = {};
  // A short rolling history of recent trigger timestamps per channel,
  // used only to answer "how many times has this channel triggered in
  // the last 60 seconds" -- the same technique the bioreactor project
  // used for its pH/ORP "too many opens per hour" check, just with a
  // 1-minute window instead of a 1-hour one. triggerTimes_ is a 2D array:
  // 6 channels x up to 99 remembered timestamps each.

  bool readSensor_(int pin) const;
  // Reads one level sensor pin and returns true/false for "is this sensor
  // currently detecting liquid," accounting for the ACTIVE_HIGH setting
  // in Config.h.

  bool recordTrigger_(int channelId);
  // Records a trigger's timestamp in the rate-limit history and returns
  // whether that pushes this channel over its per-minute limit. Does NOT
  // touch the gas volume total or open the valve -- see GasChannel.cpp's
  // update() for exactly why that decision has to wait until after this
  // check runs.

  void countTrigger_(int channelId);
  // Actually counts a trigger as real gas production (updates the
  // running total and trigger count) and logs it. Only ever called for a
  // trigger that recordTrigger_() did NOT flag as a rate violation.

  void lockChannel_(int channelId);
  // Locks a channel following a trigger that DID fail the rate check.

  void logEvent_(const char* fmt, int channelId);
  // A small helper for writing a timestamped SD-card log line about a
  // specific channel (e.g. "Ch3 gas trigger #12, 12.00 mL total").
};
