#include "Reactor.h"
#include "Mcp.h"
#include "Sd.h"

Reactor::Reactor(int id, uint8_t lsAPin, uint8_t lsBPin, int fvMcpPin, int wvMcpPin)
  : id_(id),
    fvPin_(fvMcpPin),
    wvPin_(wvMcpPin),
    lsAInput_(lsAPin, ReactorTimings::LS_A_CONFIRM_MS),
    lsBInput_(lsBPin, ReactorTimings::LS_B_CONFIRM_MS),
    lsAConfirmed_(false),
    lsBConfirmed_(false),
    state_(ReactorState::Recirculating),
    fvOpen_(false),
    wvOpen_(false),
    fvOpenedAt_(0),
    wvOpenedAt_(0),
    holdStartedAt_(0),
    holding_(false),
    lsAError_(false),
    lsBError_(false)
{}

void Reactor::begin()
{
  lsAInput_.begin();
  lsBInput_.begin();
  lsAConfirmed_ = !lsAInput_.read();
  lsBConfirmed_ = !lsBInput_.read();
}

void Reactor::updateSensors(ConfigState& config)
{
  lsAInput_.update();
  lsBInput_.update();

  lsAConfirmed_ = !lsAInput_.read();
  lsBConfirmed_ = !lsBInput_.read();

  config.lsA[id_] = lsAConfirmed_;
  config.lsB[id_] = lsBConfirmed_;
}

bool Reactor::wantsFill() const
{
  if (errorLatched()) return false;
  if (fvOpen_ || wvOpen_) return false;
  if (state_ == ReactorState::Draining || state_ == ReactorState::WaitingForDrain) return false;
  if (state_ == ReactorState::Filling) return false;
  if (!lsBConfirmed_ && !lsAConfirmed_) return true;
  if (lsBConfirmed_ && !lsAConfirmed_) return true;
  return false;
}

bool Reactor::wantsDrain() const
{
  if (errorLatched()) return false;
  if (fvOpen_ || wvOpen_) return false;
  if (state_ != ReactorState::Holding && state_ != ReactorState::WaitingForDrain) return false;
  if (millis() - holdStartedAt_ < ReactorTimings::RECIRC_HOLD_MS) return false;
  return true;
}

void Reactor::markQueuedForFill(ConfigState& config)
{
  if (state_ == ReactorState::Recirculating || state_ == ReactorState::Holding)
  {
    state_ = ReactorState::WaitingForFill;
    writeStateToConfig(config);
  }
}

void Reactor::markQueuedForDrain(ConfigState& config)
{
  if (state_ == ReactorState::Holding)
  {
    state_ = ReactorState::WaitingForDrain;
    writeStateToConfig(config);
  }
}

void Reactor::startFill(Mcp& mcp, SdLogger& sd, ConfigState& config)
{
  if (fvOpen_) return;
  fvOpen_ = true;
  fvOpenedAt_ = millis();
  mcp.setPin(fvPin_, true);
  state_ = ReactorState::Filling;
  holding_ = false;
  sd.logReactorEvent(id_, "FV_OPEN", 0);
  writeStateToConfig(config);
}

void Reactor::startDrain(Mcp& mcp, SdLogger& sd, ConfigState& config)
{
  if (wvOpen_) return;
  wvOpen_ = true;
  wvOpenedAt_ = millis();
  mcp.setPin(wvPin_, true);
  state_ = ReactorState::Draining;
  holding_ = false;
  sd.logReactorEvent(id_, "WV_OPEN", 0);
  writeStateToConfig(config);
}

void Reactor::closeFv(Mcp& mcp, SdLogger& sd, ConfigState& config)
{
  if (!fvOpen_) return;
  mcp.setPin(fvPin_, false);
  unsigned long duration = millis() - fvOpenedAt_;
  fvOpen_ = false;
  sd.logReactorEvent(id_, "FV_CLOSE", duration);
  writeStateToConfig(config);
}

void Reactor::closeWv(Mcp& mcp, SdLogger& sd, ConfigState& config)
{
  if (!wvOpen_) return;
  mcp.setPin(wvPin_, false);
  unsigned long duration = millis() - wvOpenedAt_;
  wvOpen_ = false;
  sd.logReactorEvent(id_, "WV_CLOSE", duration);
  writeStateToConfig(config);
}

void Reactor::serviceFill(Mcp& mcp, SdLogger& sd, ConfigState& config)
{
  if (!fvOpen_) return;

  if (lsAConfirmed_ && lsBConfirmed_)
  {
    closeFv(mcp, sd, config);
    state_ = ReactorState::Holding;
    holdStartedAt_ = millis();
    holding_ = true;
    writeStateToConfig(config);
    return;
  }

  if (millis() - fvOpenedAt_ >= ReactorTimings::FILL_TIMEOUT_MS && !lsAConfirmed_)
  {
    closeFv(mcp, sd, config);
    lsAError_ = true;
    state_ = ReactorState::ErrorLsA;
    sd.logReactorEvent(id_, "LS_A_ERROR", 0);
    writeStateToConfig(config);
  }
}

void Reactor::serviceDrain(Mcp& mcp, SdLogger& sd, ConfigState& config)
{
  if (!wvOpen_) return;

  if (!lsAConfirmed_ && !lsBConfirmed_)
  {
    closeWv(mcp, sd, config);
    state_ = ReactorState::Recirculating;
    writeStateToConfig(config);
    return;
  }

  if (millis() - wvOpenedAt_ >= ReactorTimings::DRAIN_TIMEOUT_MS && lsBConfirmed_)
  {
    closeWv(mcp, sd, config);
    lsBError_ = true;
    state_ = ReactorState::ErrorLsB;
    sd.logReactorEvent(id_, "LS_B_ERROR", 0);
    writeStateToConfig(config);
  }
}

void Reactor::tick(ConfigState& config)
{
  if (!fvOpen_ && !wvOpen_ && !errorLatched())
  {
    if (lsAConfirmed_ && !lsBConfirmed_)
    {
      lsBError_ = true;
      state_ = ReactorState::ErrorLsB;
    }
    else if (state_ == ReactorState::Recirculating && lsAConfirmed_ && lsBConfirmed_)
    {
      state_ = ReactorState::Holding;
      holdStartedAt_ = millis();
      holding_ = true;
    }
  }
  writeStateToConfig(config);
}

void Reactor::clearErrors(ConfigState& config)
{
  lsAError_ = false;
  lsBError_ = false;
  if (state_ == ReactorState::ErrorLsA || state_ == ReactorState::ErrorLsB)
    state_ = ReactorState::Recirculating;
  writeStateToConfig(config);
}

void Reactor::manualSetFv(Mcp& mcp, bool on)
{
  mcp.setPin(fvPin_, on);
  fvOpen_ = on;
  if (on) fvOpenedAt_ = millis();
}

void Reactor::manualSetWv(Mcp& mcp, bool on)
{
  mcp.setPin(wvPin_, on);
  wvOpen_ = on;
  if (on) wvOpenedAt_ = millis();
}

void Reactor::writeStateToConfig(ConfigState& config)
{
  config.reactorState[id_] = state_;
  config.fvOpen[id_] = fvOpen_;
  config.wvOpen[id_] = wvOpen_;
  config.lsAError[id_] = lsAError_;
  config.lsBError[id_] = lsBError_;
}
