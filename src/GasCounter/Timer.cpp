// ============================================================================
// Timer.cpp
//
// PLAIN-ENGLISH SUMMARY:
// This is the "recipe" that implements the Timer class described in
// Timer.h. If you're new to C++: a line like "Timer::isReady()" means
// "here is the actual code for the isReady function that belongs to the
// Timer class." The "Timer::" part is required so the compiler knows which
// class's function you're writing, since different classes are allowed to
// have functions with the same name.
// ============================================================================

#include "Timer.h"
// Pull in the "shape" (declarations) of Timer so this file knows what
// lastTrigger_, interval_, isReady(), etc. are supposed to look like.

Timer::Timer(unsigned long interval)
  : lastTrigger_(0), interval_(interval) {}
// This is the constructor's actual implementation. When someone writes
// "Timer myTimer(2000);" elsewhere in the code, this is what runs:
//   lastTrigger_ = 0            (no trigger has happened yet)
//   interval_    = interval     (remember the interval we were given, e.g. 2000ms)
// The empty {} means there's nothing extra to run afterward.

bool Timer::isReady()
{
  unsigned long current = millis();
  // Ask Arduino "how many milliseconds since power-on right now?" and
  // remember that as "current."

  if (current - lastTrigger_ >= interval_)
  // Compare how much time has passed since the last trigger
  // (current - lastTrigger_) against how much time we're supposed to wait
  // (interval_). If enough time has passed...
  {
    lastTrigger_ = current;
    // ...remember "now" as the new last-trigger time, so the next check
    // measures from this moment forward (this is the "automatic reset"
    // behaviour described in Timer.h).

    return true;
    // Tell the caller "yes, the interval has elapsed — go ahead and do
    // your periodic action now."
  }

  return false;
  // Not enough time has passed yet — tell the caller "not yet."
}

void Timer::reset()
{
  lastTrigger_ = millis();
  // Manually set "last trigger" to right now, restarting the countdown
  // without waiting for isReady() to naturally return true first.
}
