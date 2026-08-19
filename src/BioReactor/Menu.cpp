#include "Menu.h"
#include "Sd.h"
#include "Wifi.h"


Menu::Menu(ConfigState* config, EzoBoard* phSensors, EzoBoard* orpSensors, Lcd& lcd, SdLogger& sd, WifiTime& wifi)
    : config_(config), sd_(&sd), wifi_(&wifi), lcd(lcd)
{
    for (int i = 0; i < 3; i++)
    {
        phSensors_[i]  = &phSensors[i];
        orpSensors_[i] = &orpSensors[i];
    }
    activePh_  = 0;
    activeOrp_ = 0;
}

void Menu::enter() 
{
    if (state_ == MenuState::Off) 
    {
        state_ = MenuState::Idle;
        needsFullRedraw_ = true;
        joystick.setSelectedItem(0);
        joystick.setMaxIndex(3);   // Main menu: 4 items (0-3)
    }
}

void Menu::update()
{
    const bool pressed = joystick.isPressed();
    const int before = joystick.getSelectedItem();

    const bool allowMenuScroll =
        state_ != MenuState::Thresholds || thresholdEdit_ == ThresholdEdit::None;

    if (!pressed && allowMenuScroll)
    {
        joystick.move();
        if (joystick.getSelectedItem() != before)
            needsFullRedraw_ = true;
    }

    switch (state_)
    {
        case MenuState::Idle:
            handleMainMenu(pressed);
            break;
        case MenuState::Calibrating:
            handleProbesMenu(pressed);
            break;
        case MenuState::PhCalibration:
            handlePhCalibrationSelection(pressed);
            break;
        case MenuState::OrpCalibration:
            handleORPSelection(pressed);
            break;
        case MenuState::Settings:
            handleSettingsMenu(pressed);
            break;
        case MenuState::ShowMac:
            handleShowMac(pressed);
            break;
        case MenuState::Thresholds:
            if (thresholdEdit_ != ThresholdEdit::None)
            {
                if (pressed)
                    thresholdEdit_ = ThresholdEdit::None;
                else
                    adjustThresholdFromJoystick();
            }
            else if (pressed)
                handleThresholdsMenu(pressed);
            break;
        case MenuState::Off:
            break;
    }
}

void Menu::draw() 
{
    lcd.updateBlink(*config_);

    static bool lastBlink = true;
    bool blinkToggled = (config_->blinkState != lastBlink);
    if (blinkToggled) lastBlink = config_->blinkState;

    if (!needsFullRedraw_ && !blinkToggled) 
    {
        if (!uiTimer_.isReady()) return;
    }

    if (state_ != lastDrawnState_) 
    {
        needsFullRedraw_ = true;
        lastDrawnState_ = state_;
    }

    switch (state_)
    {
        case MenuState::Idle:
            displayMainMenu();
            break;
        case MenuState::Calibrating:
            displayProbesMenu();
            break;
        case MenuState::PhCalibration:
            displayPhMenu();
            break;
        case MenuState::OrpCalibration:
            displayORPCalibrationMenu();
            break;
        case MenuState::Settings:
            displaySettingsMenu();
            break;
        case MenuState::ShowMac:
            displayShowMac();
            break;
        case MenuState::Thresholds:
            displayThresholdsMenu();
            break;
        case MenuState::Off:
            break;
    }

    uiTimer_.reset();
    needsFullRedraw_ = false;
    lastSelectedItem_ = joystick.getSelectedItem();
}

void Menu::handleMainMenu(bool pressed)
{
    if (!pressed) return;

    switch (joystick.getSelectedItem()) 
    {
        case 0:
            joystick.setSelectedItem(0);
            joystick.setMaxIndex(MENU_DONE);
            state_ = MenuState::Calibrating;
            break;
        case 1:
            joystick.setSelectedItem(0);
            joystick.setMaxIndex(3);   // Settings: 4 items (0-3)
            state_ = MenuState::Settings;
            break;
        case 2:
            config_->requestResetErrors = true;
            break;
        case 3:
            state_ = MenuState::Off;
            lcd.clear();                 
            needsFullRedraw_ = true;    
            break;
    }
    needsFullRedraw_ = true;
}

void Menu::handleSettingsMenu(bool pressed)
{
    if (!pressed) return;

    switch (joystick.getSelectedItem())
    {
        case 0:
            // pH/ORP valves: direct toggle, no submenu needed.
            config_->valvesDisabled = !config_->valvesDisabled;
            break;
        case 1:
            joystick.setSelectedItem(0);
            joystick.setMaxIndex(6);   // pH/ORP settings: 7 items (0-6)
            thresholdEdit_ = ThresholdEdit::None;
            state_ = MenuState::Thresholds;
            break;
        case 2:
            joystick.setSelectedItem(0);
            joystick.setMaxIndex(1);   // Show MAC and WiFi: 2 items (Sync Now / Return)
            state_ = MenuState::ShowMac;
            break;
        case 3:
            joystick.setSelectedItem(0);
            joystick.setMaxIndex(3);   // back to Main menu (4 items)
            state_ = MenuState::Idle;
            lcd.clear();
            break;
    }
    needsFullRedraw_ = true;
}

void Menu::handleProbesMenu(bool pressed)
{
    if (!pressed) return;

    switch (joystick.getSelectedItem()) 
    {
        // ----- pH probes -----
        case MENU_PH1:
            activePh_ = 0;
            joystick.setSelectedItem(BUFFER_4);
            joystick.setMaxIndex(4);
            state_ = MenuState::PhCalibration;
            break;

        case MENU_PH2:
            activePh_ = 1;
            joystick.setSelectedItem(BUFFER_4);
            joystick.setMaxIndex(4);
            state_ = MenuState::PhCalibration;
            break;

        case MENU_PH3:
            activePh_ = 2;
            joystick.setSelectedItem(BUFFER_4);
            joystick.setMaxIndex(4);
            state_ = MenuState::PhCalibration;
            break;

        // ----- ORP probes -----
        case MENU_ORP1:
            activeOrp_ = 0;
            joystick.setSelectedItem(0);
            joystick.setMaxIndex(1);
            state_ = MenuState::OrpCalibration;
            break;

        case MENU_ORP2:
            activeOrp_ = 1;
            joystick.setSelectedItem(0);
            joystick.setMaxIndex(1);
            state_ = MenuState::OrpCalibration;
            break;

        case MENU_ORP3:
            activeOrp_ = 2;
            joystick.setSelectedItem(0);
            joystick.setMaxIndex(1);
            state_ = MenuState::OrpCalibration;
            break;

        // ----- done / back -----
        case MENU_DONE:
            joystick.setSelectedItem(0);
            joystick.setMaxIndex(3);   // back to Main menu (4 items)
            state_ = MenuState::Idle;
            lcd.clear();
            break;
    }
    needsFullRedraw_ = true;
}

void Menu::handleShowMac(bool pressed)
{
    if (!pressed) return;

    switch (joystick.getSelectedItem())
    {
        case 0:
            // Manually kick off a connect+sync attempt right now. Stays on
            // this screen so the live WiFi/sync status lines above show the
            // attempt play out in real time.
            wifi_->syncNow();
            break;
        case 1:
            joystick.setSelectedItem(0);
            joystick.setMaxIndex(3);   // back to Settings (4 items)
            state_ = MenuState::Settings;
            break;
    }
    needsFullRedraw_ = true;
}

void Menu::handleThresholdsMenu(bool pressed)
{
    if (!pressed) return;

    switch (joystick.getSelectedItem())
    {
        case 0: thresholdEdit_ = ThresholdEdit::Ph;         break;
        case 1: thresholdEdit_ = ThresholdEdit::Orp;        break;
        case 2: thresholdEdit_ = ThresholdEdit::PhMaxTrig;  break;
        case 3: thresholdEdit_ = ThresholdEdit::OrpMaxTrig; break;
        case 4: thresholdEdit_ = ThresholdEdit::PhValveT;   break;
        case 5:
            // item 5 is OrpValveT (edit) on
            // the joystick selection
            thresholdEdit_ = ThresholdEdit::OrpValveT;
            break;
        case 6:
            thresholdEdit_ = ThresholdEdit::None;
            joystick.setSelectedItem(0);
            joystick.setMaxIndex(3);   // back to Settings
            state_ = MenuState::Settings;
            break;
    }
    needsFullRedraw_ = true;
}

void Menu::adjustThresholdFromJoystick()
{
    const int step = joystick.yAxisStep();
    if (step == 0) return;

    if (thresholdEdit_ == ThresholdEdit::Ph)
    {
        config_->phMinimum += step * ThresholdLimits::PH_STEP;
        config_->phMinimum = constrain(
            config_->phMinimum,
            ThresholdLimits::PH_MIN,
            ThresholdLimits::PH_MAX);
    }
    else if (thresholdEdit_ == ThresholdEdit::Orp)
    {
        config_->orpMinimum += step * ThresholdLimits::ORP_STEP;
        config_->orpMinimum = constrain(
            config_->orpMinimum,
            ThresholdLimits::ORP_MIN,
            ThresholdLimits::ORP_MAX);
    }
    else if (thresholdEdit_ == ThresholdEdit::PhMaxTrig)
    {
        config_->phMaxTrig += step * TriggerLimitBounds::STEP;
        config_->phMaxTrig = constrain(
            config_->phMaxTrig,
            TriggerLimitBounds::MIN,
            TriggerLimitBounds::MAX);
    }
    else if (thresholdEdit_ == ThresholdEdit::OrpMaxTrig)
    {
        config_->orpMaxTrig += step * TriggerLimitBounds::STEP;
        config_->orpMaxTrig = constrain(
            config_->orpMaxTrig,
            TriggerLimitBounds::MIN,
            TriggerLimitBounds::MAX);
    }
    else if (thresholdEdit_ == ThresholdEdit::PhValveT)
    {
        config_->phValveTimerSec += step * ValveTimerBounds::STEP_SEC;
        config_->phValveTimerSec = constrain(
            config_->phValveTimerSec,
            ValveTimerBounds::MIN_SEC,
            ValveTimerBounds::MAX_SEC);
    }
    else if (thresholdEdit_ == ThresholdEdit::OrpValveT)
    {
        config_->orpValveTimerSec += step * ValveTimerBounds::STEP_SEC;
        config_->orpValveTimerSec = constrain(
            config_->orpValveTimerSec,
            ValveTimerBounds::MIN_SEC,
            ValveTimerBounds::MAX_SEC);
    }

    needsFullRedraw_ = true;
}

bool Menu::persistPhBufferCalibration(int bufferIdx)
{
    char payload[CalibrationStorage::EXPORT_PAYLOAD_MAX];

    if (!phSensors_[activePh_]->exportCalibration(payload, sizeof(payload)))
        return false;

    if (!sd_->savePhBufferCalibration(activePh_, bufferIdx, payload))
        return false;

    config_->phBufferCalibrated[activePh_][bufferIdx] = true;
    return true;
}

bool Menu::persistOrpCalibration()
{
    char payload[CalibrationStorage::EXPORT_PAYLOAD_MAX];

    if (!orpSensors_[activeOrp_]->exportCalibration(payload, sizeof(payload)))
        return false;

    if (!sd_->saveOrpCalibration(activeOrp_, payload))
        return false;

    config_->orpCalibrated[activeOrp_] = true;
    return true;
}

void Menu::handlePhCalibrationSelection(bool pressed)
{
    if (!pressed) return;
    switch (joystick.getSelectedItem())
    {
        case BUFFER_4:
            phSensors_[activePh_]->resetReadState();
            phSensors_[activePh_]->sendCmd("Cal,low,4.00");
            delay(TimingIntervals::EZO_CAL_SETTLE_MS);
            persistPhBufferCalibration(0);
            break;
        case BUFFER_7:
            phSensors_[activePh_]->resetReadState();
            phSensors_[activePh_]->sendCmd("Cal,mid,7.00");
            delay(TimingIntervals::EZO_CAL_SETTLE_MS);
            persistPhBufferCalibration(1);
            break;
        case BUFFER_10:
            phSensors_[activePh_]->resetReadState();
            phSensors_[activePh_]->sendCmd("Cal,high,10.00");
            delay(TimingIntervals::EZO_CAL_SETTLE_MS);
            persistPhBufferCalibration(2);
            break;
        case BUFFER_CLEAR:
            phSensors_[activePh_]->sendCmd("Cal,clear");
            sd_->clearPhProbeCalibration(activePh_);
            config_->phBufferCalibrated[activePh_][0] = false;
            config_->phBufferCalibrated[activePh_][1] = false;
            config_->phBufferCalibrated[activePh_][2] = false;
            break;
        case BUFFER_DONE:
            joystick.setSelectedItem(0);
            joystick.setMaxIndex(MENU_DONE);
            state_ = MenuState::Calibrating;
            break;
    }
    needsFullRedraw_ = true;
}

void Menu::handleORPSelection(bool pressed) 
{
    if (!pressed) return;
    switch (joystick.getSelectedItem())
    {
        case 0:
            orpSensors_[activeOrp_]->resetReadState();
            orpSensors_[activeOrp_]->sendCmd("Cal,222");
            delay(TimingIntervals::EZO_CAL_SETTLE_MS);
            persistOrpCalibration();
            lcd.setCursor(COL_LEFT, ROW_3);
            lcd.print("Calibrated @222mV   ");
            delay(350);
            joystick.setSelectedItem(0);
            joystick.setMaxIndex(MENU_DONE);
            state_ = MenuState::Calibrating;
            break;
        case 1:
            joystick.setSelectedItem(0);
            joystick.setMaxIndex(MENU_DONE);
            state_ = MenuState::Calibrating;
            break;
    }
    needsFullRedraw_ = true;
}

void Menu::displayMainMenu() 
{
    const int sel = joystick.getSelectedItem();

    // 4 items (0-3), 3 display rows (ROW_1-ROW_3) — same sliding-window
    // pattern as displaySettingsMenu, since one more item no longer fits
    // statically on ROW_1..ROW_3.
    int first = sel - 1;
    if (first < 0) first = 0;
    if (first > 1) first = 1;

    if (needsFullRedraw_) 
    {
        lcd.clear();
        lcd.setCursor(COL_LEFT, ROW_TITLE);
        lcd.print("MAIN MENU");
    }

    char labelBuf[4][20];
    snprintf(labelBuf[0], sizeof(labelBuf[0]), "%-19s", "Calibrate Probes");
    snprintf(labelBuf[1], sizeof(labelBuf[1]), "%-19s", "Settings");
    snprintf(labelBuf[2], sizeof(labelBuf[2]), "%-19s", "Reset Errors");
    snprintf(labelBuf[3], sizeof(labelBuf[3]), "%-19s", "Exit");
    const char* labels[] = { labelBuf[0], labelBuf[1], labelBuf[2], labelBuf[3] };

    // Repaint all three visible rows each cycle so a shorter label never
    // leaves ghost characters from a longer one that was previously there.
    for (int r = 0; r < 3; ++r)
    {
        const int itemIdx = first + r;
        const int row     = ROW_1 + r;

        if (itemIdx == sel)
            printMenuItem(COL_LEFT, row, labels[itemIdx], itemIdx);
        else
        {
            lcd.setCursor(COL_LEFT, row);
            lcd.print(labels[itemIdx]);
        }
    }
}

void Menu::displayProbesMenu() 
{
    if (needsFullRedraw_) 
    {
        lcd.clear();
        lcd.setCursor(COL_LEFT, ROW_TITLE);
        lcd.print("Calibrate Probe:");
        for (const auto& item : calMenuChoices_) 
        {
            lcd.setCursor(item.col, item.row);
            lcd.print(item.label);
        }
        lcd.setCursor(COL_FAR_RIGHT, ROW_2);
        lcd.print("Done");
        joystick.setMaxIndex(MENU_DONE);
    }

    if (!needsFullRedraw_ && lastSelectedItem_ != -1 && lastSelectedItem_ != joystick.getSelectedItem()) 
    {
        if (lastSelectedItem_ == MENU_DONE) 
        {
            lcd.setCursor(COL_FAR_RIGHT, ROW_2); lcd.print("Done");
        } 
        else if (lastSelectedItem_ >= 0 && lastSelectedItem_ < 6) 
        {
            const MenuItem& prev = calMenuChoices_[lastSelectedItem_];
            lcd.setCursor(prev.col, prev.row);
            lcd.print(prev.label);
        }
    }

    switch (joystick.getSelectedItem()) 
    {
        case MENU_DONE:
            printMenuItem(COL_FAR_RIGHT, ROW_2, "Done", MENU_DONE);
            break;
        default:
            if (joystick.getSelectedItem() >= 0 && joystick.getSelectedItem() < 6) 
            {
                const MenuItem& it = calMenuChoices_[joystick.getSelectedItem()];
                printMenuItem(it.col, it.row, it.label, it.index);
            }
            break;
    }
}

void Menu::displaySettingsMenu()
{
    const int sel = joystick.getSelectedItem();

    // 4 items (0-3), 3 display rows (ROW_1–ROW_3).
    // Slide the window so the selection is always visible without overlap.
    // first is clamped to [0, 1] so items first..first+2 fit in 3 rows.
    int first = sel - 1;
    if (first < 0) first = 0;
    if (first > 1) first = 1;

    if (needsFullRedraw_)
    {
        lcd.clear();
        lcd.setCursor(COL_LEFT, ROW_TITLE);
        lcd.print("Settings");
        joystick.setMaxIndex(3);
    }

    char labelBuf[4][19];
    snprintf(labelBuf[0], sizeof(labelBuf[0]), "%-18s",
             config_->valvesDisabled ? "pH/ORP valves:OFF" : "pH/ORP valves:ON");
    snprintf(labelBuf[1], sizeof(labelBuf[1]), "%-18s", "pH/ORP settings");
    snprintf(labelBuf[2], sizeof(labelBuf[2]), "%-18s", "Show MAC and WiFi");
    snprintf(labelBuf[3], sizeof(labelBuf[3]), "%-18s", "Return");
    const char* labels[] = { labelBuf[0], labelBuf[1], labelBuf[2], labelBuf[3] };

    // Repaint all three visible rows each cycle. The trailing spaces in each
    // label guarantee that a shorter label never leaves ghost characters from
    // a longer one that previously occupied the same row.
    for (int r = 0; r < 3; ++r)
    {
        const int itemIdx = first + r;
        const int row     = ROW_1 + r;

        if (itemIdx == sel)
            printMenuItem(COL_LEFT, row, labels[itemIdx], itemIdx);
        else
        {
            lcd.setCursor(COL_LEFT, row);
            lcd.print(labels[itemIdx]);
        }
    }
}

void Menu::displayShowMac()
{
    const int sel = joystick.getSelectedItem();

    if (needsFullRedraw_)
    {
        lcd.clear();
        joystick.setMaxIndex(1);   // 2 items: Sync Now / Return
    }

    // MAC address, WiFi connection state, and sync status are all printed
    // every render pass (not gated behind needsFullRedraw_) so this screen
    // stays live while the user is looking at it — e.g. watching WiFi go
    // from Disconnected to Connected, or a manual sync complete in real
    // time after pressing "Sync Now".
    char line[20];

    snprintf(line, sizeof(line), "%-19s",
             config_->macAddress[0] ? config_->macAddress : "(not available)");
    lcd.setCursor(COL_LEFT, ROW_TITLE);
    lcd.print(line);

    snprintf(line, sizeof(line), "%-19s",
             wifi_->isConnected() ? "WiFi Connected" : "WiFi Disconnected");
    lcd.setCursor(COL_LEFT, ROW_1);
    lcd.print(line);

    char syncStatus[24];
    wifi_->syncStatus(syncStatus, sizeof(syncStatus));
    snprintf(line, sizeof(line), "%-19s", syncStatus);
    lcd.setCursor(COL_LEFT, ROW_2);
    lcd.print(line);

    // Action row: the radio is deliberately disconnected between scheduled
    // syncs, so "Disconnected" alone doesn't tell you much — this lets you
    // force an attempt on demand and watch the lines above update live.
    if (sel == 0)
        printMenuItem(COL_LEFT, ROW_3, "Sync Now", 0);
    else
    {
        lcd.setCursor(COL_LEFT, ROW_3);
        lcd.print("Sync Now");
    }

    if (sel == 1)
        printMenuItem(COL_RIGHT, ROW_3, "Return", 1);
    else
    {
        lcd.setCursor(COL_RIGHT, ROW_3);
        lcd.print("Return");
    }
}

void Menu::displayThresholdsMenu()
{
    if (needsFullRedraw_)
    {
        lcd.clear();
        joystick.setMaxIndex(6);  // items 0-5 are editable, item 6 is Return
    }

    // ── Active edit screens ────────────────────────────────────────────────
    if (thresholdEdit_ == ThresholdEdit::Ph)
    {
        lcd.setCursor(COL_LEFT, ROW_TITLE); lcd.print("Edit pHmin      ");
        lcd.setCursor(COL_LEFT, ROW_1);     lcd.print("Up/Dn  Press=OK ");
        lcd.setCursor(COL_LEFT, ROW_2);
        lcd.print("pH: ");
        lcd.print(config_->phMinimum, 2);
        lcd.print("           ");
        return;
    }

    if (thresholdEdit_ == ThresholdEdit::Orp)
    {
        lcd.setCursor(COL_LEFT, ROW_TITLE); lcd.print("Edit ORPmin     ");
        lcd.setCursor(COL_LEFT, ROW_1);     lcd.print("Up/Dn  Press=OK ");
        lcd.setCursor(COL_LEFT, ROW_2);
        lcd.print("ORP: ");
        lcd.print((int)config_->orpMinimum);
        lcd.print(" mV         ");
        return;
    }

    if (thresholdEdit_ == ThresholdEdit::PhMaxTrig)
    {
        lcd.setCursor(COL_LEFT, ROW_TITLE); lcd.print("Edit pHMaxTrig  ");
        lcd.setCursor(COL_LEFT, ROW_1);     lcd.print("Up/Dn  Press=OK ");
        lcd.setCursor(COL_LEFT, ROW_2);
        lcd.print("pH trig/hr: ");
        lcd.print(config_->phMaxTrig);
        lcd.print("   ");
        return;
    }

    if (thresholdEdit_ == ThresholdEdit::OrpMaxTrig)
    {
        lcd.setCursor(COL_LEFT, ROW_TITLE); lcd.print("Edit ORPMaxTrig ");
        lcd.setCursor(COL_LEFT, ROW_1);     lcd.print("Up/Dn  Press=OK ");
        lcd.setCursor(COL_LEFT, ROW_2);
        lcd.print("ORP trig/hr: ");
        lcd.print(config_->orpMaxTrig);
        lcd.print("   ");
        return;
    }

    if (thresholdEdit_ == ThresholdEdit::PhValveT)
    {
        lcd.setCursor(COL_LEFT, ROW_TITLE); lcd.print("Edit pH ValveTmr");
        lcd.setCursor(COL_LEFT, ROW_1);     lcd.print("Up/Dn  Press=OK ");
        lcd.setCursor(COL_LEFT, ROW_2);
        lcd.print("pH valve: ");
        lcd.print(config_->phValveTimerSec);
        lcd.print(" s   ");
        return;
    }

    if (thresholdEdit_ == ThresholdEdit::OrpValveT)
    {
        lcd.setCursor(COL_LEFT, ROW_TITLE); lcd.print("Edit ORP ValveTmr");
        lcd.setCursor(COL_LEFT, ROW_1);     lcd.print("Up/Dn  Press=OK ");
        lcd.setCursor(COL_LEFT, ROW_2);
        lcd.print("ORP valve: ");
        lcd.print(config_->orpValveTimerSec);
        lcd.print(" s   ");
        return;
    }

    // ── Browser (no active edit) ───────────────────────────────────────────
    // The 20×4 LCD has rows 0-3.  Row 0 = title "Thresholds".
    // Rows 1-3 show a rolling window of items; each item label is on the
    // left and its current value on the right.  "Return" (item 6) appears
    // on ROW_3 only when selected.

    if (needsFullRedraw_)
    {
        lcd.setCursor(COL_LEFT, ROW_TITLE);
        lcd.print("pH/ORP settings     ");
    }

    // Helper lambda-style local: print one row from the 7-item list.
    // Items 0-5 are editable values; item 6 is Return.
    // We display items [sel-1 .. sel+1] centred on rows 1-3 when possible,
    // but for simplicity we use a fixed mapping: always show items 0-2 on
    // rows 1-3 and scroll the highlight label onto the appropriate row.
    // Because we have 7 items and only 3 data rows, we use a simple
    // 3-row sliding window based on the selected index.
    const int sel = joystick.getSelectedItem();
    // Window: first item on ROW_1
    int first = sel - 1;
    if (first < 0) first = 0;
    if (first > 4) first = 4;  // leave room for 3 rows (items first..first+2)

    // Labels and current values for each item
    auto printRow = [&](int row, int itemIdx)
    {
        lcd.setCursor(COL_LEFT, row);
        switch (itemIdx)
        {
            case 0:
                lcd.print("pH min:");
                lcd.print(config_->phMinimum, 2);
                lcd.print(" ");
                break;
            case 1:
                lcd.print("ORP min:");
                lcd.print((int)config_->orpMinimum);
                lcd.print("  ");
                break;
            case 2:
                lcd.print("pHMaxTrig:");
                lcd.print(config_->phMaxTrig);
                lcd.print("   ");
                break;
            case 3:
                lcd.print("ORPMaxTrig:");
                lcd.print(config_->orpMaxTrig);
                lcd.print("   ");
                break;
            case 4:
                lcd.print("pH valve:");
                lcd.print(config_->phValveTimerSec);
                lcd.print("s  ");
                break;
            case 5:
                lcd.print("ORP valve:");
                lcd.print(config_->orpValveTimerSec);
                lcd.print("s  ");
                break;
            case 6:
                lcd.print("Return    ");
                break;
        }
    };

    printRow(ROW_1, first);
    printRow(ROW_2, first + 1);
    printRow(ROW_3, first + 2);

    // Highlight (blink) the selected item on its row
    const int selRow = ROW_1 + (sel - first);
    if (selRow >= ROW_1 && selRow <= ROW_3)
    {
        if (sel == 6)
            printMenuItem(COL_LEFT, selRow, "Return              ", sel);
        else
        {
            // Blink just the "Set" label on the right side
            const char* labels[] = { "Set", "Set", "Set",
                                     "Set", "Set", "Set" };
            printMenuItem(COL_FAR_RIGHT, selRow, labels[sel], sel);
        }
    }
}

void Menu::displayPhMenu() 
{
    if (needsFullRedraw_) 
    {
        lcd.clear();
        lcd.setCursor(COL_LEFT, ROW_TITLE);
        lcd.print("Select Buffer:");
        for (const auto& item : phMenuChoices_) 
        {
            lcd.setCursor(item.col, item.row);
            lcd.print(item.label);
        }

        // Stars for calibrated buffers of the active pH probe
        if (config_->phBufferCalibrated[activePh_][0]) { lcd.setCursor(COL_MID, ROW_1); lcd.print("*"); }
        if (config_->phBufferCalibrated[activePh_][1]) { lcd.setCursor(COL_MID, ROW_2); lcd.print("*"); }
        if (config_->phBufferCalibrated[activePh_][2]) { lcd.setCursor(COL_MID, ROW_3); lcd.print("*"); }

        joystick.setMaxIndex(4);
    }

    // get live ph reading
    lcd.setCursor(COL_RIGHT + 1, ROW_3);
    lcd.print(config_->phValues[activePh_], 2);
    lcd.print("    ");

    // unhighlight the previous menu item if there's one
    if (!needsFullRedraw_ && lastSelectedItem_ != -1 && lastSelectedItem_ != joystick.getSelectedItem()) 
    {
        const MenuItem& prev = phMenuChoices_[lastSelectedItem_];
        lcd.setCursor(prev.col, prev.row);
        lcd.print(prev.label);
    }

    // print the selected menu item with highlight
    const MenuItem& cur = phMenuChoices_[joystick.getSelectedItem()];
    printMenuItem(cur.col, cur.row, cur.label, cur.index);
}

void Menu::displayORPCalibrationMenu() 
{
    if (needsFullRedraw_) 
    {
        lcd.clear();
        lcd.setCursor(COL_LEFT, ROW_TITLE);
        lcd.print("Calibrate when ready");
        lcd.setCursor(COL_LEFT, ROW_1); lcd.print("Calibrate at 222mV");
        lcd.setCursor(COL_LEFT, ROW_2); lcd.print("Done");
        lcd.setCursor(COL_LEFT, ROW_3);
        lcd.print("ORP");
        lcd.print(activeOrp_ + 1);
        lcd.print(": ");
        joystick.setMaxIndex(1);
    }

    // Live ORP reading — outside needsFullRedraw_ so it refreshes on every
    // render pass (driven by uiTimer_), matching how displayPhMenu keeps the
    // pH value current. Previously this was inside the needsFullRedraw_ block,
    // so the value was printed only once when the screen first appeared and
    // never updated again while the user waited for the probe to stabilise.
    lcd.setCursor(COL_LEFT + 6, ROW_3);  // column 6: after "ORP" + digit + ": "
    lcd.print((int)config_->orpValues[activeOrp_]);
    lcd.print("mV   ");

    if (!needsFullRedraw_ && lastSelectedItem_ != -1 && lastSelectedItem_ != joystick.getSelectedItem()) 
    {
        switch (lastSelectedItem_) 
        {
            case 0: lcd.setCursor(COL_LEFT, ROW_1); lcd.print("Calibrate at 222mV"); break;
            case 1: lcd.setCursor(COL_LEFT, ROW_2); lcd.print("Done               "); break;
        }
    }

    switch (joystick.getSelectedItem()) 
    {
        case 0: printMenuItem(COL_LEFT, ROW_1, "Calibrate at 222mV", 0); break;
        case 1: printMenuItem(COL_LEFT, ROW_2, "Done",               1); break;
    }
}

/* ===== Helpers ===== */

bool Menu::allPHBuffersCalibrated() 
{
    return config_->phBufferCalibrated[activePh_][0] &&
           config_->phBufferCalibrated[activePh_][1] &&
           config_->phBufferCalibrated[activePh_][2];
}

void Menu::displayError(const String& error) 
{
    lcd.setCursor(COL_LEFT, ROW_TITLE); lcd.print("WARNING:           ");
    lcd.setCursor(COL_LEFT, ROW_1);     lcd.print(error);
    lcd.setCursor(COL_LEFT, ROW_2);     lcd.print("Unlock System?    ");
}

void Menu::printMenuItem(int col, int row, const char* label, int itemIndex) 
{
    lcd.setCursor(col, row);
    if (itemIndex == joystick.getSelectedItem() && config_->blinkState) 
    {
        for (int i = 0; i < (int)strlen(label); ++i) lcd.print(' ');
    } 
    else 
    {
        lcd.print(label);
    }
}
