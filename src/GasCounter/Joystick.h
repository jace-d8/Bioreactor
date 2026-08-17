// ============================================================================
// Joystick.h
//
// PLAIN-ENGLISH SUMMARY:
// This defines the Joystick class: everything needed to read the little
// analog joystick + button that the operator uses to navigate the LCD
// menu. It tracks "which menu item is currently highlighted" and gives
// other code (mainly Menu.cpp) simple yes/no answers like "did the user
// just push the button?" and "did they just nudge up or down?"
//
// The joystick's Y-axis (up/down) is an ANALOG input — instead of just
// on/off, it reports a number (roughly 0-4095 on this hardware) depending
// on how far it's tilted. Center rest position reads around the middle of
// that range. "Up" and "down" are detected by checking whether that number
// has moved far enough away from center.
// ============================================================================

#pragma once
#include <Arduino.h>
#include "Config.h"
// Config.h holds the actual pin numbers and tuning constants (like how big
// the "dead zone" around center should be) so they can be changed in one
// place without touching this file.
#include "Timer.h"
// We use a Timer (see Timer.h) to "debounce" the button — see isPressed()
// below for what that means.

class Joystick
{
private:
    // ── Internal state (not directly touchable from outside this class) ──
    int pinY_, pinSW_;
    // The two Arduino pin numbers this joystick is wired to:
    //   pinY_  = the analog pin reading the up/down tilt
    //   pinSW_ = the digital pin reading the built-in push-button
    // Declaring two variables on one line separated by a comma ("int a, b;")
    // is just a shorthand for "int a; int b;" — both are still separate
    // whole-number variables.

    int deadZone_, range_;
    // These two are declared here but, looking through Joystick.cpp,
    // they're actually not used anywhere (the dead zone used in practice
    // comes straight from JoystickConfig::ANALOG_DEADZONE in Config.h
    // instead). They're harmless leftovers — you could remove them without
    // changing behaviour, but it's also fine to leave them as-is.

    unsigned long debounceMs_;
    // How many milliseconds must pass between two button presses being
    // counted as separate presses (prevents one physical press from being
    // read as several rapid presses due to tiny electrical "bounces").

    int selectedItem_;
    // Which menu item is currently highlighted, counting from 0 (0 = first
    // item, 1 = second item, and so on).

    int maxIndex_;
    // The index of the LAST selectable item in whatever menu is currently
    // showing (e.g. if a menu has 4 items, maxIndex_ is 3, since counting
    // starts at 0). Menu.cpp updates this every time it switches to a
    // different screen with a different number of items.

    Timer debounceTimer_;
    // A Timer object (see Timer.h) used to enforce the debounceMs_ gap
    // between accepted button presses.

    Timer scrollTimer_{JoystickConfig::SCROLL_DELAY_MS};
    // BUG FIX: move()/yAxisStep() used to enforce the "don't scroll too
    // fast" gap with a blocking delay(SCROLL_DELAY_MS) call -- but this
    // whole project's central design rule (see GUIDE.md section 3,
    // "never wait") is that loop() must never block for more than a few
    // milliseconds, specifically because GasChannel::update() has to run
    // every single loop() pass to catch a level sensor's rising edge
    // promptly. A 200 ms delay() called from inside Menu::update() (see
    // GasCounter.ino's loop(), which only calls gasChannel.update() once
    // per pass, BEFORE menu.update()) meant that every time the operator
    // nudged the joystick while the menu was open, gas-channel sensor
    // reading paused for up to 200 ms -- long enough to miss a rising
    // edge (and therefore a real gas count) on a fast-cycling channel.
    // This Timer replaces that blocking delay with the same non-blocking
    // pattern already used for debounceTimer_ above: isReady() answers
    // immediately, and only returns true once per SCROLL_DELAY_MS.

public:
    // ── Setup ────────────────────────────────────────────────────────────
    Joystick(int pinY, int pinSW, int maxIndex)
        : pinY_(pinY), pinSW_(pinSW), maxIndex_(maxIndex),
        selectedItem_(0), debounceMs_(JoystickConfig::SWITCH_DEBOUNCE_MS), debounceTimer_(debounceMs_)
    // This is the constructor: it runs once, when a Joystick object is
    // first created (see BioReactor.ino and Menu.h, where a single
    // `joystick` object is created for the whole program to share). You
    // give it which pins it's wired to and how many menu items the very
    // first screen it shows will have.
    //
    // The part after the colon (:) sets each variable's starting value in
    // one step, before the {} body below even runs:
    //   pinY_          = pinY               (remember the analog pin)
    //   pinSW_         = pinSW               (remember the button pin)
    //   maxIndex_      = maxIndex            (remember the starting menu size)
    //   selectedItem_  = 0                   (start highlighting the first item)
    //   debounceMs_    = the debounce time from Config.h
    //   debounceTimer_ = a Timer set up with that debounce time
    {
        pinMode(pinSW_, INPUT_PULLUP); // Active-low button
        // Tell the Arduino hardware "the button pin should be read as a
        // digital input, and use the chip's internal pull-up resistor."
        // A pull-up resistor makes the pin read HIGH (1) by default when
        // nothing is pressed, and the button pulls it LOW (0) when
        // physically pushed — hence "active-low" (pressed = LOW, not HIGH).
    }

    // ── Things other code can ask the Joystick to do ────────────────────
    void move();
    // Reads the analog stick and, if it's tilted up or down, shifts
    // selectedItem_ by one (wrapping around at the ends of the menu). Used
    // for simple single-column menu navigation.

    int yAxisStep();
    // A lighter-weight alternative to move(): just tells you which
    // direction the stick is currently tilted, WITHOUT changing
    // selectedItem_ itself. Returns -1 for "up", 1 for "down", or 0 for
    // "centered / no input." Used when the caller wants to decide for
    // itself what "up"/"down" should mean (e.g. adjusting a number instead
    // of moving a menu highlight).

    bool isPressed();
    // Returns true exactly once per physical button press (debounced —
    // see the .cpp file for how). Returns false the rest of the time,
    // including while the button is being held down.

    int getSelectedItem() const;
    // Returns which item index is currently highlighted. The "const" at
    // the end promises this function only reads data, never changes it.

    void setSelectedItem(int index);
    // Forces the highlight to a specific item index (used when switching
    // screens, so the highlight always starts at a sensible spot).

    int getMaxIndex() const;
    // Returns how many items (minus one) the current menu screen has.

    void setMaxIndex(int newMaxIndex);
    // Changes how many items (minus one) the current menu screen has —
    // called every time Menu.cpp switches to a different screen, since
    // different screens have different numbers of options.
};
