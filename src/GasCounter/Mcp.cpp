// ============================================================================
// Mcp.cpp
//
// PLAIN-ENGLISH SUMMARY:
// The actual implementation behind Mcp.h -- three short functions, no
// queueing machinery to speak of (compare against the bioreactor
// project's much longer Mcp.cpp, which needs several ValveQueue-related
// functions).
// ============================================================================

#include "Mcp.h"
#include "Config.h"

// Same ESP32 pinMode/digitalRead/digitalWrite macro workaround as Mcp.h
// (see the comment there for the full explanation). This is a SEPARATE,
// independent push/undef/pop -- Mcp.h already restored the macros to
// normal at its own end, so every mcp_.pinMode()/digitalWrite() call
// written directly in this .cpp file (not inside Mcp.h) needs its own
// suppression too.
#if defined(ARDUINO_ARCH_ESP32)
  #pragma push_macro("pinMode")
  #pragma push_macro("digitalRead")
  #pragma push_macro("digitalWrite")
  #undef pinMode
  #undef digitalRead
  #undef digitalWrite
#endif

bool Mcp::begin()
{
  if (!mcp_.begin_I2C()) return false;
  // Try to establish contact with the MCP23X17 chip over I2C. If that
  // fails (chip missing, wired wrong, etc.), give up immediately and tell
  // the caller (GasCounter.ino's setup()) that valve control isn't
  // available.

  for (int i = 0; i < ChannelMappings::TOTAL_VALVES; ++i)
    mcp_.pinMode(i, OUTPUT);
  // Configure every one of the 6 valve pins (ids 0-5) as an OUTPUT pin.
  // "for (int i = 0; i < N; ++i)" is a standard counting loop: start i at
  // 0, keep going as long as i is less than N, add 1 to i after each pass.

  resetState();
  // Make sure every valve starts fully closed before returning.
  return true;
}

void Mcp::setValve(int channelId, bool on)
{
  mcp_.digitalWrite(channelId, on ? HIGH : LOW);
  // "on ? HIGH : LOW" is the ternary operator -- a compact one-line
  // if/else. If "on" is true, send the electrical signal HIGH (open the
  // valve); otherwise LOW (close it). Since valve id == channel id in
  // this project (see Config.h's ChannelMappings), no id translation is
  // needed here at all.
}

void Mcp::resetState()
{
  for (int i = 0; i < ChannelMappings::TOTAL_VALVES; ++i)
    mcp_.digitalWrite(i, LOW);
}

// Restore pinMode/digitalRead/digitalWrite to their normal (real-board-pin)
// behavior for the rest of this file.
#if defined(ARDUINO_ARCH_ESP32)
  #pragma pop_macro("digitalWrite")
  #pragma pop_macro("digitalRead")
  #pragma pop_macro("pinMode")
#endif
