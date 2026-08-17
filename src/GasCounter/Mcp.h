// ============================================================================
// Mcp.h
//
// PLAIN-ENGLISH SUMMARY:
// This defines the Mcp class, which controls all 6 gas-channel valves.
// Like the bioreactor project's Mcp, every valve is wired to a separate
// MCP23X17 chip (16 extra digital output pins, controlled over I2C)
// rather than directly to the Arduino; this class is the only place in
// the project that talks to that chip.
//
// This version is MUCH simpler than the bioreactor's Mcp, though: there's
// no queueing system at all. The bioreactor needed queues because several
// valves shared one physical supply line (only one pH valve could safely
// be open at a time, etc). Here, each of the 6 channels has its own fully
// independent valve with no shared resource, and the spec explicitly asks
// for immediate triggering with no exclusivity between channels -- so
// this class is just a thin, direct "turn this pin on/off" wrapper, and
// GasChannel.cpp is fully responsible for all timing decisions.
// ============================================================================

#pragma once

// ── Arduino Nano ESP32 pinMode/digitalWrite/digitalRead macro workaround ──
// The Arduino Nano ESP32 board core defines pinMode(), digitalRead(), and
// digitalWrite() as MACROS that rewrite any call to those names into a
// version that first translates the pin number via digitalPinToGPIONumber().
// The C++ preprocessor does blind text substitution, so it can't tell the
// difference between a real call on an actual Arduino pin and the
// Adafruit_MCP23X17 library's OWN methods of the same names (which
// control the MCP23017 chip's 16 virtual pins -- nothing to do with real
// board pins). Left unguarded, this corrupts both the library's own
// declarations and every mcp_.pinMode()/digitalWrite() call below. See
// the identical fix (and fuller explanation) in the bioreactor project's
// Mcp.h -- push_macro/pop_macro temporarily hides the macros for exactly
// this file, then restores them immediately after, so every OTHER file's
// real pinMode()/digitalWrite() calls on actual board pins (e.g. the
// level sensors in GasChannel.cpp) are unaffected.
#if defined(ARDUINO_ARCH_ESP32)
  #pragma push_macro("pinMode")
  #pragma push_macro("digitalRead")
  #pragma push_macro("digitalWrite")
  #undef pinMode
  #undef digitalRead
  #undef digitalWrite
#endif

#include <Adafruit_MCP23X17.h>
#include "Config.h"

class Mcp
{
public:
  bool begin();
  // Call once at startup: connects to the MCP23X17 chip over I2C and
  // configures all 6 valve pins as outputs. Returns false if the chip
  // couldn't be found/contacted (e.g. wiring problem).

  void setValve(int channelId, bool on);
  // Directly open (true) or close (false) one channel's valve. No queue,
  // no auto-close, no safety timeout -- the caller (GasChannel) is fully
  // responsible for deciding when to call this and for how long the
  // valve should stay open.

  void resetState();
  // Force every valve closed (used at boot, right after connecting to
  // the chip, so every valve starts in a known, safe, closed state).

private:
  Adafruit_MCP23X17 mcp_;
  // The actual connection to the physical MCP23X17 chip. Everything this
  // class does ultimately goes through this one object.
};

// Restore pinMode/digitalRead/digitalWrite to their normal (real-board-pin)
// behavior for every file that includes Mcp.h after this point.
#if defined(ARDUINO_ARCH_ESP32)
  #pragma pop_macro("digitalWrite")
  #pragma pop_macro("digitalRead")
  #pragma pop_macro("pinMode")
#endif
