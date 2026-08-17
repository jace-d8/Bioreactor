// ============================================================================
// Joystick.cpp
//
// PLAIN-ENGLISH SUMMARY:
// The actual step-by-step code for everything declared in Joystick.h.
// ============================================================================

#include "Joystick.h"
#include "Config.h"

void Joystick::move()
{
    int yVal = analogRead(pinY_);
    // Read the current raw value from the analog joystick pin. On this
    // hardware this is typically a number from roughly 0 to 4095, where
    // the middle of that range means "joystick is centered / not touched."

    const int center = JoystickConfig::ANALOG_CENTER;
    // The expected "resting" reading when nobody is touching the stick
    // (from Config.h — tune this there if your specific joystick reads a
    // different center value).

    const int deadZone = JoystickConfig::ANALOG_DEADZONE;
    // How far away from "center" the reading has to be before we treat it
    // as a deliberate up/down movement rather than tiny natural jitter.

    if (yVal < center - deadZone)
    { // Up
        // The reading is far enough BELOW center to count as "pushed up."
        if (!scrollTimer_.isReady()) return;
        // BUG FIX: non-blocking replacement for the old
        // delay(SCROLL_DELAY_MS) call -- see Joystick.h's comment on
        // scrollTimer_ for why blocking here was a problem. isReady()
        // returns false (without pausing) if we scrolled too recently,
        // so we simply skip moving the highlight THIS pass and try again
        // next loop() -- and true at most once per SCROLL_DELAY_MS,
        // which is what actually rate-limits the scrolling.
        if (selectedItem_ > 0)
            selectedItem_--;
            // Move the highlight up by one item (as long as we're not
            // already at the very first item).
        else
            selectedItem_ = maxIndex_; // Wrap to last
            // Already at the first item — wrap around to the last item
            // instead of getting stuck.
    }
    else if (yVal > center + deadZone)
    { // Down
        // The reading is far enough ABOVE center to count as "pushed down."
        // Mirror image of the "Up" block above.
        if (!scrollTimer_.isReady()) return;
        if (selectedItem_ < maxIndex_)
            selectedItem_++;
        else
            selectedItem_ = 0; // Wrap to first
    }
    // If neither condition was true, the stick is within the dead zone
    // (roughly centered) — do nothing, selectedItem_ stays the same.
}

int Joystick::yAxisStep()
{
    const int yVal = analogRead(pinY_);
    const int center = JoystickConfig::ANALOG_CENTER;
    const int deadZone = JoystickConfig::ANALOG_DEADZONE;
    // Same three readings as move() above.

    if (yVal < center - deadZone)
    {
        if (!scrollTimer_.isReady()) return 0;
        // BUG FIX: same non-blocking replacement as move() above, using
        // the same shared scrollTimer_ -- move() and yAxisStep() are
        // never called in the same update() cycle (see Menu::update()'s
        // allowMenuScroll), so sharing one timer between them is safe and
        // keeps the scroll pacing consistent between browsing and editing.
        return -1;
        // Report "up" without touching selectedItem_ — the caller decides
        // what to do with that direction.
    }
    if (yVal > center + deadZone)
    {
        if (!scrollTimer_.isReady()) return 0;
        return 1;
        // Report "down."
    }
    return 0;
    // Centered / no meaningful movement.
}

bool Joystick::isPressed()
{
    bool pressed = !digitalRead(pinSW_); // Active low
    // digitalRead(pinSW_) gives HIGH (true-ish) when the button is NOT
    // pressed, and LOW (false-ish) when it IS pressed (because of the
    // pull-up wiring set up in the constructor). The "!" flips that around
    // so "pressed" reads as true exactly when the button is physically
    // held down — much easier to reason about in the rest of the code.

    if (pressed && debounceTimer_.isReady())
    // Only count this as a "new" press if BOTH:
    //   1) the button is currently held down, AND
    //   2) enough time has passed since the last accepted press
    //      (debounceTimer_.isReady() — see Timer.h/.cpp).
    // This stops a single physical button press — which can flicker
    // between pressed/not-pressed several times within milliseconds due
    // to the mechanical switch "bouncing" — from being counted as many
    // separate presses.
    {
        debounceTimer_.reset(); // Restart debounce timer
        // Restart the debounce countdown so the NEXT press also has to
        // wait out the full debounce gap.
        return true;
        // Tell the caller "yes, a fresh button press just happened."
    }
    return false;
    // Either the button isn't pressed right now, or it is but we're still
    // within the debounce cooldown from the last accepted press.
}

int Joystick::getSelectedItem() const
{
    return selectedItem_;
}

void Joystick::setSelectedItem(int index)
{
    if (index >= 0 && index <= maxIndex_)
        // Only accept the new value if it's within the valid range for the
        // current menu (0 up to maxIndex_) — protects against accidentally
        // setting the highlight to an item that doesn't exist.
        selectedItem_ = index;
}

int Joystick::getMaxIndex() const
{
  return maxIndex_;
}

void Joystick::setMaxIndex(int newMaxIndex)
{
  maxIndex_ = newMaxIndex;
}
