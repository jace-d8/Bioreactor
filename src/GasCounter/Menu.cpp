// ============================================================================
// Menu.cpp
//
// PLAIN-ENGLISH SUMMARY:
// The full implementation behind Menu.h. Much shorter than the
// bioreactor project's Menu.cpp since there's only one real screen
// (Settings) beyond the main menu. The LCD flicker-avoidance trick from
// the bioreactor project is reused throughout: rather than clearing and
// redrawing the whole screen every single time, most screens only redraw
// the ONE row that actually changed (erase the previous highlight, draw
// the new one). A full lcd.clear() + redraw only happens when
// needsFullRedraw_ is true.
// ============================================================================

#include "Menu.h"
#include "Sd.h"

Menu::Menu(ConfigState* config, Lcd& lcd, SdLogger& sd)
    : config_(config), sd_(&sd), lcd(lcd) {}
// Constructor. Notice sd_ is stored as "&sd" (converting the REFERENCE
// parameter "sd" into a POINTER for storage) while lcd is stored directly
// as a reference -- the same small, deliberate style inconsistency the
// bioreactor project's Menu constructor has (both approaches work).

void Menu::enter()
// Called from GasCounter.ino's loop() when the joystick button is
// pressed while the menu wasn't already showing.
{
    if (state_ == MenuState::Off)
    // Only actually do anything if the menu was truly off -- guards
    // against enter() being called again while already active.
    {
        state_ = MenuState::Idle;
        needsFullRedraw_ = true;
        joystick.setSelectedItem(0);
        joystick.setMaxIndex(3);   // Main menu: Settings / Reset Error / Valves / Exit
    }
}

void Menu::update()
// Called every loop iteration while the menu is active. Handles joystick
// movement generically, then hands off to whichever screen-specific
// handle...Menu() function matches the current state_.
{
    const bool pressed = joystick.isPressed();
    const int before = joystick.getSelectedItem();
    // Remember the highlighted item BEFORE processing this update, so we
    // can tell afterward whether it actually changed.

    const bool allowMenuScroll =
        (state_ != MenuState::Settings || settingsEdit_ == SettingsEdit::None);
    // Normally, moving the joystick up/down should move the menu
    // highlight. But if we're on the Settings screen AND actively editing
    // a value (settingsEdit_ isn't None), the joystick's up/down instead
    // means "change the number" -- so in that specific situation, the
    // generic joystick.move() call below must NOT also shift the
    // highlight.

    if (!pressed && allowMenuScroll)
    {
        joystick.move();
        if (joystick.getSelectedItem() != before)
            needsFullRedraw_ = true;
    }

    switch (state_)
    // Dispatch to whichever screen's own input-handling function matches
    // the current state.
    {
        case MenuState::Idle:
            handleMainMenu(pressed);
            break;
        case MenuState::Settings:
            if (settingsEdit_ != SettingsEdit::None)
            // Currently editing a specific setting.
            {
                if (pressed)
                    settingsEdit_ = SettingsEdit::None;
                    // Pressing the button while editing means "done
                    // editing this value" -- return to browsing the list.
                else
                    adjustSettingsFromJoystick();
                    // Any joystick movement adjusts the number instead of
                    // moving a highlight.
            }
            else if (pressed)
                handleSettingsMenu(pressed);
                // Not currently editing anything -- a button press means
                // "start editing whichever item is highlighted" (or
                // toggle/act on it, for the non-numeric items).
            break;
        case MenuState::Off:
            break;
            // Nothing to do -- the menu isn't showing.
    }
}

void Menu::draw()
// Called every loop iteration while the menu is active. Decides WHETHER
// to redraw, then dispatches to whichever screen's own display...()
// function matches the current state.
{
    lcd.updateBlink(*config_);
    // "*config_" DEREFERENCES the config_ pointer -- turns "a pointer to
    // the ConfigState" back into "the actual ConfigState itself" so it
    // can be passed to updateBlink(), which expects a real reference.

    static bool lastBlink = true;
    // "static" on a variable INSIDE a function means this variable keeps
    // its value between calls, rather than resetting every time draw()
    // runs.
    bool blinkToggled = (config_->blinkState != lastBlink);
    if (blinkToggled) lastBlink = config_->blinkState;
    // Check whether the blink state has flipped since the last time
    // draw() ran, and if so, remember the new value.

    if (!needsFullRedraw_ && !blinkToggled)
    {
        if (!uiTimer_.isReady()) return;
        // Nothing has changed AND it's not yet time for a routine redraw
        // pass -- skip drawing entirely this cycle.
    }

    if (state_ != lastDrawnState_)
    {
        needsFullRedraw_ = true;
        lastDrawnState_ = state_;
        // Switched to a DIFFERENT screen since the last draw -- force a
        // full redraw (the old screen's leftover text needs clearing).
    }

    switch (state_)
    {
        case MenuState::Idle:
            displayMainMenu();
            break;
        case MenuState::Settings:
            displaySettingsMenu();
            break;
        case MenuState::Off:
            break;
    }

    uiTimer_.reset();
    needsFullRedraw_ = false;
    lastSelectedItem_ = joystick.getSelectedItem();
    // Bookkeeping after any actual draw: restart the routine-redraw
    // timer, clear the full-redraw flag, and remember which item was
    // highlighted this time.
}

void Menu::handleMainMenu(bool pressed)
// Reacts to input while showing the 4-item main menu (Settings / Reset
// Error / Valves On-Off / Exit). Reset Error and Valves used to live one
// screen deeper, inside Settings -- they now live here instead, so an
// operator can silence a rate-error lockout or pause all valves in one
// click from the very first screen the menu shows (see Menu.h's class
// comment for the reasoning).
{
    if (!pressed) return;
    // This screen only reacts to button PRESSES (joystick movement is
    // already handled generically by update() above).

    switch (joystick.getSelectedItem())
    {
        case 0:
            // "Settings" selected.
            joystick.setSelectedItem(0);
            joystick.setMaxIndex(3);   // Settings: 4 items (0-3)
            settingsEdit_ = SettingsEdit::None;
            state_ = MenuState::Settings;
            break;
        case 1:
            // "Reset Error" selected -- just set the flag, same as this
            // option used to work inside Settings. GasCounter.ino's
            // loop() notices config_->requestResetErrors and performs the
            // actual reset. We deliberately stay on the main menu after
            // this (no state_ change) so the user gets visual
            // confirmation nothing navigated away.
            config_->requestResetErrors = true;
            break;
        case 2:
            // "Valves: ON/OFF" selected -- flip the global pause switch:
            // on becomes off, off becomes on. Checked in
            // GasChannel::update() -- while true (valvesDisabled), no
            // channel will trigger its valve, pausing the whole system
            // for maintenance without powering the device down.
            config_->valvesDisabled = !config_->valvesDisabled;
            break;
        case 3:
            // "Exit" selected -- leave the menu system entirely.
            state_ = MenuState::Off;
            lcd.clear();
            needsFullRedraw_ = true;
            break;
    }
    needsFullRedraw_ = true;
}

void Menu::handleSettingsMenu(bool pressed)
// Reacts to input on the Settings screen (Pulse Sec / Cal Factor /
// Max Trig per Min / Return). Valves On-Off and Reset Error used to be
// items 3 and 4 here -- they've moved to the main menu (see
// handleMainMenu() above), so Settings is now just the 3 numeric values
// plus Return.
{
    if (!pressed) return;

    switch (joystick.getSelectedItem())
    {
        case 0: settingsEdit_ = SettingsEdit::PulseSec;     break;
        case 1: settingsEdit_ = SettingsEdit::CalFactor;    break;
        case 2: settingsEdit_ = SettingsEdit::MaxTrigPerMin;break;
        case 3:
            joystick.setSelectedItem(0);
            joystick.setMaxIndex(3);
            state_ = MenuState::Idle;
            lcd.clear();
            // "Return" selected -- go back to the main menu.
            break;
    }
    needsFullRedraw_ = true;
}

void Menu::adjustSettingsFromJoystick()
// Called every update() cycle WHILE actively editing one specific
// setting. Reads which direction the joystick is currently tilted and
// nudges the matching ConfigState field up or down by one step, clamped
// within its allowed range.
{
    const int step = joystick.yAxisStep();
    // -1 (up), +1 (down), or 0 (centered) -- see Joystick.cpp.
    if (step == 0) return;

    if (settingsEdit_ == SettingsEdit::PulseSec)
    {
        config_->pulseSec += step * GasChannelBounds::PULSE_SEC_STEP;
        // "step * STEP" -- if step is -1 (up), this subtracts one step's
        // worth; if step is +1 (down), it adds one step's worth. One line
        // handles both directions.
        config_->pulseSec = constrain(
            config_->pulseSec,
            GasChannelBounds::PULSE_SEC_MIN,
            GasChannelBounds::PULSE_SEC_MAX);
        // constrain() is a built-in Arduino function: clamp this value so
        // it never goes below MIN or above MAX.
    }
    else if (settingsEdit_ == SettingsEdit::CalFactor)
    {
        config_->calFactor += step * GasChannelBounds::CAL_FACTOR_STEP;
        config_->calFactor = constrain(
            config_->calFactor,
            GasChannelBounds::CAL_FACTOR_MIN,
            GasChannelBounds::CAL_FACTOR_MAX);
    }
    else if (settingsEdit_ == SettingsEdit::MaxTrigPerMin)
    {
        config_->maxTrigPerMin += step * GasChannelBounds::MAX_TRIG_PER_MIN_STEP;
        config_->maxTrigPerMin = constrain(
            config_->maxTrigPerMin,
            GasChannelBounds::MAX_TRIG_PER_MIN_MIN,
            GasChannelBounds::MAX_TRIG_PER_MIN_MAX);
    }
    // Three near-identical blocks, one per editable setting -- each reads
    // config_->[fieldName] += step * [that field's STEP constant], then
    // clamps it.

    needsFullRedraw_ = true;
    // Force a redraw so the newly-adjusted number shows up immediately.
}

// Returns the padded label text for one main-menu item -- pulled into its
// own helper since, unlike the old 2-item menu, the CURRENT text of item
// 2 ("Valves: ON " vs "Valves: OFF") depends on live ConfigState, and
// that same text is needed in three different places below (initial
// draw, erasing the previous highlight, and drawing the current one).
// All four labels are padded to the same 11-character width (the longest,
// "Valves: OFF", is exactly 11) so that switching the highlight or the
// Valves text never leaves stray leftover characters on screen.
const char* Menu::mainMenuLabel_(int itemIndex) const
{
    switch (itemIndex)
    {
        case 0: return "Settings   ";
        case 1: return "Reset Error";
        case 2: return config_->valvesDisabled ? "Valves: OFF" : "Valves: ON ";
        case 3: return "Exit       ";
        default: return "";
    }
}

void Menu::displayMainMenu()
// Draws the 4-item main menu: Settings / Reset Error / Valves On-Off /
// Exit. Unlike the old 2-item version, there's no separate "MAIN MENU"
// title row -- with 4 items and only 4 physical LCD rows available
// (ROW_TITLE through ROW_3), every row is needed for an item.
{
    if (needsFullRedraw_)
    // Only runs right after switching TO this screen (or right after
    // boot) -- clears the screen and draws every line of static text once.
    {
        lcd.clear();
        lcd.setCursor(COL_LEFT, ROW_TITLE); lcd.print(mainMenuLabel_(0));
        lcd.setCursor(COL_LEFT, ROW_1);     lcd.print(mainMenuLabel_(1));
        lcd.setCursor(COL_LEFT, ROW_2);     lcd.print(mainMenuLabel_(2));
        lcd.setCursor(COL_LEFT, ROW_3);     lcd.print(mainMenuLabel_(3));
    }

    if (!needsFullRedraw_ && lastSelectedItem_ != -1 && lastSelectedItem_ != joystick.getSelectedItem())
    // We're NOT doing a full redraw, but the highlighted item DID change
    // -- erase the PREVIOUS highlight by reprinting that row in plain
    // (non-blinking) text.
    {
        const int row = ROW_TITLE + lastSelectedItem_;
        lcd.setCursor(COL_LEFT, row);
        lcd.print(mainMenuLabel_(lastSelectedItem_));
    }

    // Draw the CURRENT highlight, via printMenuItem() (see the bottom of
    // this file), which knows how to make the selected row blink.
    // Toggling Valves (case 2 in handleMainMenu()) always sets
    // needsFullRedraw_, so the branch at the top of this function already
    // redraws all 4 rows -- including the Valves row's new text -- any
    // time the toggle actually changes; nothing extra is needed here.
    const int selRow = ROW_TITLE + joystick.getSelectedItem();
    printMenuItem(COL_LEFT, selRow, mainMenuLabel_(joystick.getSelectedItem()), joystick.getSelectedItem());
}

void Menu::displaySettingsMenu()
// Draws the Settings screen. Like the bioreactor project's Thresholds
// screen, this has TWO very different modes: an active-EDIT mode (a
// dedicated full-screen view for one value being adjusted) and a
// BROWSING mode (a scrolling 4-item list). Now just the 3 numeric
// settings plus Return -- Valves and Reset Error moved to the main menu
// (see handleMainMenu() in this file), so this screen no longer needs
// the 6-item sliding window it used to.
{
    if (needsFullRedraw_)
    {
        lcd.clear();
        joystick.setMaxIndex(3);  // items 0-2 editable, item 3 is Return
    }

    // ── Active edit screens ────────────────────────────────────────────────
    if (settingsEdit_ == SettingsEdit::PulseSec)
    // Each of these 3 blocks draws a dedicated full-screen editing view
    // for exactly one setting, then RETURNS IMMEDIATELY -- so while
    // actively editing, none of the browsing-mode code further down even
    // runs.
    {
        lcd.setCursor(COL_LEFT, ROW_TITLE); lcd.print("Edit PulseCD   ");
        lcd.setCursor(COL_LEFT, ROW_1);     lcd.print("Up/Dn  Press=OK ");
        lcd.setCursor(COL_LEFT, ROW_2);
        lcd.print("PulseCD: ");
        lcd.print(config_->pulseSec);
        lcd.print(" s      ");
        return;
    }

    if (settingsEdit_ == SettingsEdit::CalFactor)
    {
        lcd.setCursor(COL_LEFT, ROW_TITLE); lcd.print("Edit VolCali  ");
        lcd.setCursor(COL_LEFT, ROW_1);     lcd.print("Up/Dn  Press=OK ");
        lcd.setCursor(COL_LEFT, ROW_2);
        lcd.print("Vol_Cali: ");
        lcd.print(config_->calFactor, 2);
        // "lcd.print(config_->calFactor, 2)" uses the float-with-decimals
        // overload of print() -- 2 means "show 2 digits after the
        // decimal point," matching the 0.05 mL editing step's precision.
        lcd.print(" mL/ct   ");
        return;
    }

    if (settingsEdit_ == SettingsEdit::MaxTrigPerMin)
    {
        lcd.setCursor(COL_LEFT, ROW_TITLE); lcd.print("Edit MaxTrig/Min");
        lcd.setCursor(COL_LEFT, ROW_1);     lcd.print("Up/Dn  Press=OK ");
        lcd.setCursor(COL_LEFT, ROW_2);
        lcd.print("Max: ");
        lcd.print(config_->maxTrigPerMin);
        lcd.print(" /min    ");
        return;
    }

    // ── Browser (no active edit) ───────────────────────────────────────────
    // 4 items (0-3), 3 display rows -- same sliding-window technique as
    // the bioreactor project's Settings/Thresholds screens: the window
    // scrolls so the highlighted item is always visible.
    const int sel = joystick.getSelectedItem();
    int first = sel - 1;
    // Try to keep the selected item roughly in the middle of the visible
    // window by starting the window one item before it.
    if (first < 0) first = 0;
    // Don't scroll past the very first item.
    if (first > 1) first = 1;
    // Don't scroll so far that we'd try to show an item PAST the end of
    // the 4-item list (we always show exactly 3 rows, so the window start
    // can be at most item 1, showing items 1, 2, 3).

    if (needsFullRedraw_)
    {
        lcd.setCursor(COL_LEFT, ROW_TITLE);
        lcd.print("Settings            ");
    }

    auto printRow = [&](int row, int itemIdx)
    // This declares a LAMBDA -- a small, nameless function defined right
    // here, stored in a local variable "printRow" so it can be called a
    // few lines below. "[&]" means "this lambda can use any variable from
    // the surrounding function (like config_) by reference." Lambdas are
    // handy for a small chunk of logic only used in one place.
    {
        lcd.setCursor(COL_LEFT, row);
        switch (itemIdx)
        {
            case 0:
                lcd.print("PulseCD:");
                lcd.print(config_->pulseSec);
                lcd.print("s      ");
                break;
            case 1:
                lcd.print("VolCali:");
                lcd.print(config_->calFactor, 2);
                lcd.print("   ");
                break;
            case 2:
                lcd.print("MaxTrig/min:");
                lcd.print(config_->maxTrigPerMin);
                lcd.print("  ");
                break;
            case 3:
                lcd.print("Return          ");
                break;
        }
    };

    printRow(ROW_1, first);
    printRow(ROW_2, first + 1);
    printRow(ROW_3, first + 2);
    // Call the lambda 3 times -- once per visible row -- each time with a
    // different item index from the current sliding window. Unlike
    // displayMainMenu's per-item switch statements, this screen simply
    // REDRAWS all 3 visible rows every time draw() runs (not just when
    // the highlight moves) -- simpler code, at the small cost of a few
    // extra characters being rewritten each pass (not noticeable to the
    // human eye).

    const int selRow = ROW_1 + (sel - first);
    // Work out which of the 3 physical rows the currently-selected item
    // landed on (since "first" scrolls, the same item can appear on
    // different physical rows at different times).
    if (selRow >= ROW_1 && selRow <= ROW_3)
    {
        if (sel <= 2)
        {
            // Blink a "Set" indicator to the right of the 3 editable
            // rows, keeping the label/value visible during the blink
            // (same technique as the bioreactor project's Thresholds
            // screen).
            printMenuItem(COL_FAR_RIGHT + 4, selRow, "Set", sel);
        }
        else
        {
            // sel == 3: "Return" -- blink the SAME text printRow() already
            // drew (rather than a separate, different-looking label), so
            // the row stays visually consistent through the blink cycle.
            printMenuItem(COL_LEFT, selRow, "Return          ", sel);
        }
    }
}

void Menu::printMenuItem(int col, int row, const char* label, int itemIndex)
// The single most-called helper in this file -- draws one menu item,
// making it blink if (and only if) it's currently the highlighted one.
{
    lcd.setCursor(col, row);
    if (itemIndex == joystick.getSelectedItem() && config_->blinkState)
    // Only blink if BOTH: this is the currently-selected item, AND the
    // shared blinkState flag (toggled by Lcd::updateBlink()) currently
    // says "blink off" -- i.e. this is the "off" half of the blink cycle.
    {
        for (int i = 0; i < (int)strlen(label); ++i) lcd.print(' ');
        // Print exactly as many blank spaces as the label has
        // characters, completely erasing it for this half of the blink
        // cycle.
    }
    else
    {
        lcd.print(label);
        // Either this isn't the selected item, or it is but we're in the
        // "on" half of the blink cycle -- either way, print the real text.
    }
}
