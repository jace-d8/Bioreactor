// ============================================================================
// ActionTimer.h
//
// PLAIN-ENGLISH SUMMARY:
// This file defines a reusable "stopwatch" that the rest of the program uses
// to wait for a certain amount of time before doing something — for example,
// "keep this valve open for 10 seconds" or "wait 1 second before opening the
// next valve." You start the stopwatch, and later you ask it "are you done
// yet?" It doesn't do anything by itself — it just keeps track of time so
// other code can check it.
//
// This is a "class" — think of a class as a blueprint for creating an
// object that bundles together some data (how long to wait, when it started,
// whether it's currently running) with the actions you can perform on that
// data (start it, stop it, ask if it's done). Every ActionTimer you create
// in the rest of the code gets its own private copy of that data.
// ============================================================================

#pragma once
// "#pragma once" is a instruction to the compiler (the program that turns
// this code into something the Arduino/ESP32 can run), not part of the
// program logic itself. It just says "if this file gets included more than
// once by accident, only process it the first time." You never need to
// change this line.

class ActionTimer {
// This line starts the definition of the ActionTimer "blueprint" (class).
// Everything between this "{" and the matching "}" near the bottom of the
// file belongs to ActionTimer.

public:
  // Everything below "public:" can be used by other files/code that create
  // an ActionTimer. Think of "public" as "the buttons and dials on the
  // outside of the stopwatch that anyone is allowed to press."

  ActionTimer(unsigned long durationMs)
    : duration_(durationMs), start_(0), running_(false) {}
  // This is the "constructor" — the special function that runs automatically
  // whenever someone creates a new ActionTimer. You always give it one
  // number: how many milliseconds (1000 milliseconds = 1 second) the timer
  // should run for once started. For example, "ActionTimer myTimer(3000);"
  // creates a timer that will take 3 seconds to finish once started.
  //
  // The part after the colon (:) sets the timer's starting values:
  //   duration_ = durationMs   (remember how long a full run should take)
  //   start_    = 0            (hasn't been started yet)
  //   running_  = false        (not currently counting down)
  // The empty {} at the end means "and there's nothing else to do here."

  void start()
  // Call this when you want the stopwatch to begin counting. "void" means
  // this function doesn't hand back any value when it finishes — it just
  // does something (starts the timer) and returns.
  {
    start_ = millis();
    // millis() is a built-in Arduino function that returns "how many
    // milliseconds has this device been powered on for." We save that
    // number as the moment we started, so later we can measure how much
    // time has passed since then.
    running_ = true;
    // Mark the timer as actively running.
  }

  void stop()
  // Call this to cancel/reset the timer back to an idle (not running) state.
  {
    running_ = false;
    // We don't need to change start_ or duration_ — just flipping this flag
    // off is enough to make done() report "yes, done" again (see below).
  }

  bool done() const
  // Call this any time to ask "has the timer finished counting down?"
  // It gives back a true/false answer ("bool" = boolean = true or false).
  // The "const" at the end is a promise to the compiler that calling this
  // function will never change anything about the timer — it only reads.
  {
    if (!running_) return true;  // idle == done
    // "!running_" means "NOT running." If the timer was never started (or
    // was stopped), we treat it as "done" — there's nothing to wait for.

    return millis() - start_ >= duration_;
    // If the timer IS running, we calculate how much time has passed
    // (current time minus the time we started) and check whether that's
    // greater than or equal to the duration we were asked to wait for.
    // If yes, the timer is done; if no, it's still counting down.
  }

private:
  // Everything below "private:" is internal bookkeeping that only this
  // class itself is allowed to touch directly — other code can't reach in
  // and change these numbers except through start()/stop()/done() above.
  // Think of this as "the gears inside the stopwatch that you're not
  // supposed to touch by hand."

  unsigned long duration_;
  // How many milliseconds this timer should run for once started.
  // "unsigned long" is a whole number (no decimals) that can only be zero
  // or positive, and can hold very large values — perfect for a running
  // millisecond counter that only ever goes up.

  unsigned long start_;
  // The millis() value recorded at the moment start() was last called.

  bool running_;
  // true if the timer is currently counting down, false if it's idle.
};
// This closing brace ends the ActionTimer class definition. The semicolon
// after it is required by C++ whenever you close a class definition.
