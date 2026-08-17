// ============================================================================
// Menu.h
//
// PLAIN-ENGLISH SUMMARY:
// This defines the Menu class -- everything to do with the joystick-driven
// screens shown on the LCD. Much simpler than the bioreactor project's
// Menu: with no probes to calibrate here, there's no need for the nested
// Calibrate Probes / per-buffer screens at all. Just a 4-item main menu
// (Settings / Reset Error / Valves On-Off / Exit) and one flat Settings
// screen with this project's 3 editable numbers.
//
// Reset Error and the Valves on/off toggle used to live inside the
// Settings screen, but they're single-tap actions/toggles rather than
// numbers you dial in, and they're also the two controls an operator is
// most likely to need in a hurry (silencing a rate-error lockout, or
// pausing all valves for maintenance) -- so they now sit directly on the
// main menu, one joystick click away, instead of being buried a screen
// deeper. Settings itself now holds only the 3 numeric, dial-in-a-value
// settings (PulseSec / CalFactor / MaxTrigPerMin).
//
// The same 3-part pattern from the bioreactor project's Menu still
// applies here, just with fewer screens to apply it to:
//   1) A MenuState enum value representing "we're currently showing this
//      screen."
//   2) A "handle...Menu(bool pressed)" function -- reacts to joystick
//      button presses/movement while this screen is showing.
//   3) A "display...Menu()" function -- actually draws this screen's text
//      onto the LCD.
// ============================================================================

#pragma once
#include "Lcd.h"
#include "Config.h"
#include "Joystick.h"
#include "Timer.h"

class SdLogger;
// Forward declaration -- Menu only needs a pointer to an SdLogger, not
// its full details (this header doesn't need to know how SdLogger works
// internally). See Sd.h for the real definition.

class Menu {
private:
    // ── Core state / dependencies ───────────────────────────────────────
    ConfigState* config_;
    // Pointer to the one shared ConfigState -- Menu reads AND writes many
    // fields here (e.g. writing a new pulseSec when the user edits it).
    SdLogger* sd_;
    // Pointer to the shared SD logger (kept for parity with the
    // bioreactor project's Menu constructor shape; not currently used for
    // anything inside Menu itself in this simpler project, since there's
    // no calibration data to save here).
    Lcd& lcd;
    // A REFERENCE (not a pointer) to the one shared Lcd object -- a style
    // choice that guarantees Menu is always given a real, valid Lcd to
    // draw to.

    // ── Menu state ───────────────────────────────────────────────────────
    enum class MenuState { Idle, Settings, Off };
    MenuState state_ = MenuState::Off;
    // Which screen is currently showing. Off = the menu system isn't
    // active at all (the LCD instead shows the normal gas readout via
    // Lcd::printData). Idle = the menu's own main menu screen -- which
    // now handles Reset Error and the Valves toggle directly (see
    // handleMainMenu() in Menu.cpp), in addition to entering Settings.

    enum class SettingsEdit {
        None,
        PulseSec,      // valve pulse duration (seconds)
        CalFactor,     // mL of gas per trigger
        MaxTrigPerMin  // trigger-rate error threshold
    };
    SettingsEdit settingsEdit_ = SettingsEdit::None;
    // While state_ == MenuState::Settings, this tracks whether we're just
    // BROWSING the list of settings (settingsEdit_ == None), or actively
    // EDITING one specific value -- needed because the joystick's
    // up/down movement means something different in each mode (move the
    // highlight vs. change a number).

    int lastSelectedItem_ = -1;
    MenuState lastDrawnState_ = MenuState::Off;
    bool needsFullRedraw_ = true;
    // These three work together to avoid needlessly redrawing the ENTIRE
    // screen every single time through the loop (which would cause
    // visible flicker) -- draw() only does a full redraw when something
    // has actually changed since the last time it ran.
    Timer uiTimer_{TimingIntervals::UI_RENDER_INTERVAL};
    // Limits how often the screen is redrawn at all.

public:
    Menu(ConfigState* config, Lcd& lcd, SdLogger& sd);
    // Constructor: hand Menu everything it needs pointers/references to.

    void enter();
    // Call this to activate the menu system -- switches state_ to Idle
    // and shows the main menu.

    void update();
    // Call every loop iteration while the menu is active: reads the
    // joystick and reacts accordingly for whichever screen is currently
    // showing.

    void draw();
    // Call every loop iteration while the menu is active: redraws the LCD
    // if needed for whichever screen is currently showing.

    bool isActive() const { return state_ != MenuState::Off; }
    // Quick check: is the menu currently showing anything?

    Joystick joystick{PinConfigurations::PIN_Y, PinConfigurations::PIN_SW, 3};
    // The ONE Joystick object for the whole program lives here, same as
    // in the bioreactor project's Menu. Initial max index of 3 matches
    // the 4-item main menu (Settings/Reset Error/Valves/Exit) -- enter()
    // sets this explicitly too every time the menu is opened, so this
    // starting value only matters before the very first enter() call.

private:
    void handleMainMenu(bool pressed);
    void handleSettingsMenu(bool pressed);
    void adjustSettingsFromJoystick();
    // Reacts to joystick movement while actively editing one of the 3
    // settings.

    void displayMainMenu();
    void displaySettingsMenu();

    const char* mainMenuLabel_(int itemIndex) const;
    // Returns the (already space-padded) label text for one of the 4
    // main-menu rows -- see Menu.cpp for why this is centralized rather
    // than inlined at each of its 3 call sites.

    void printMenuItem(int col, int row, const char* label, int itemIndex);
    // Draws one menu item, making it blink if (and only if) it's
    // currently the highlighted one.
};
