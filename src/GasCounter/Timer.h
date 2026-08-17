// ============================================================================
// Timer.h
//
// PLAIN-ENGLISH SUMMARY:
// This defines a REPEATING timer, different from ActionTimer's "one-shot"
// stopwatch. You give it an interval (e.g. "2000" for 2 seconds), and from
// then on you just keep asking isReady()? every time through the main
// program loop. Every time isReady() answers "yes," it automatically resets
// itself and starts counting toward the next "yes." This is exactly the
// pattern used for things like "read the pH probes every 2 seconds" or
// "log to the SD card every 30 seconds" — you don't have to manually
// restart it each time.
//
// This file (Timer.h) only DECLARES what a Timer can do (its "shape").
// The actual step-by-step instructions for each function live in the
// matching Timer.cpp file, right next to this one. Splitting a class into
// a .h file (the "menu of what it can do") and a .cpp file (the "recipe for
// how it does it") is a common C++ pattern used throughout this project.
// ============================================================================

#pragma once
// See ActionTimer.h for what this does — avoids processing this file twice.

#include <Arduino.h>
// This line pulls in Arduino's built-in toolbox: things like millis()
// (current time since power-on) and basic number/text types. Almost every
// file in this project starts with this line for that reason.

class Timer
{
private:
  // ── Internal bookkeeping (only Timer itself touches these) ─────────────
  unsigned long lastTrigger_;
  // The millis() timestamp of the last time isReady() returned true (or of
  // when the Timer was created/reset).

  unsigned long interval_;
  // How many milliseconds must pass between one "ready" and the next.

public:
  // ── The public "menu" of things other code can do with a Timer ─────────

  Timer(unsigned long interval);
  // The constructor. Create a Timer by giving it an interval in
  // milliseconds, e.g. "Timer myTimer(2000);" for "check every 2 seconds."
  // Notice there's no {} body here — just a declaration ending in a
  // semicolon. The actual behaviour is written in Timer.cpp.

  /**
  * @brief Compares time elapsed - last trigger to interval
  * @returns True if the timer is ready, false otherwise
  */
  // This comment block (using /** ... */) is a standard documentation
  // style. @brief is a one-line description, @returns explains what you
  // get back. Many editors show this text as a tooltip when you use the
  // function elsewhere, which is why it's written this way.
  bool isReady();
  // Call this every time through your loop. It returns true exactly once
  // per interval (and automatically resets the countdown when it does).

  /**
  * @brief sets lastTrigger_ to current time elapsed
  */
  void reset();
  // Manually restart the countdown from right now, without waiting for
  // isReady() to naturally return true. Useful if something else already
  // "used up" this timing window and you want to avoid double-triggering.
};
// End of the Timer class "shape." See Timer.cpp for how each function
// actually works.
