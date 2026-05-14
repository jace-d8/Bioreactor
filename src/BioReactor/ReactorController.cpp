#include "ReactorController.h"
#include "Mcp.h"
#include "Sd.h"

ReactorController::ReactorController(Mcp& mcp, SdLogger& sd)
  : mcp_(mcp),
    sd_(sd),
    reactors_{
      Reactor(0, PinConfigurations::LS_A_PINS[0], PinConfigurations::LS_B_PINS[0], McpPins::FV[0], McpPins::WV[0]),
      Reactor(1, PinConfigurations::LS_A_PINS[1], PinConfigurations::LS_B_PINS[1], McpPins::FV[1], McpPins::WV[1]),
      Reactor(2, PinConfigurations::LS_A_PINS[2], PinConfigurations::LS_B_PINS[2], McpPins::FV[2], McpPins::WV[2])
    },
    activeFv_(-1),
    activeWv_(-1),
    wv0Open_(true)
{}

void ReactorController::begin()
{
  for (int i = 0; i < ReactorMappings::NUM_REACTORS; ++i)
    reactors_[i].begin();
  mcp_.setPin(McpPins::WV0, true);
  wv0Open_ = true;
}

bool ReactorController::queueContains(const std::queue<int>& q, int idx) const
{
  std::queue<int> copy = q;
  while (!copy.empty())
  {
    if (copy.front() == idx) return true;
    copy.pop();
  }
  return false;
}

void ReactorController::update(ConfigState& config)
{
  for (int i = 0; i < ReactorMappings::NUM_REACTORS; ++i)
  {
    reactors_[i].updateSensors(config);
    reactors_[i].tick(config);
  }

  if (!config.reactorAutoEnabled)
  {
    updateWv0(config);
    return;
  }

  serviceActive(config);
  evaluateQueues(config);
  updateWv0(config);
}

void ReactorController::serviceActive(ConfigState& config)
{
  if (activeFv_ >= 0)
  {
    reactors_[activeFv_].serviceFill(mcp_, sd_, config);
    if (!reactors_[activeFv_].isFvActive())
      activeFv_ = -1;
  }

  if (activeWv_ >= 0)
  {
    reactors_[activeWv_].serviceDrain(mcp_, sd_, config);
    if (!reactors_[activeWv_].isWvActive())
      activeWv_ = -1;
  }
}

void ReactorController::evaluateQueues(ConfigState& config)
{
  for (int i = 0; i < ReactorMappings::NUM_REACTORS; ++i)
  {
    if (reactors_[i].wantsFill() && !queueContains(feedQueue_, i) && activeFv_ != i)
    {
      feedQueue_.push(i);
      reactors_[i].markQueuedForFill(config);
    }
    if (reactors_[i].wantsDrain() && !queueContains(wasteQueue_, i) && activeWv_ != i)
    {
      wasteQueue_.push(i);
      reactors_[i].markQueuedForDrain(config);
    }
  }

  if (activeFv_ < 0 && !feedQueue_.empty())
  {
    int idx = feedQueue_.front();
    feedQueue_.pop();
    if (reactors_[idx].wantsFill())
    {
      activeFv_ = idx;
      reactors_[idx].startFill(mcp_, sd_, config);
    }
  }

  if (activeWv_ < 0 && !wasteQueue_.empty())
  {
    int idx = wasteQueue_.front();
    wasteQueue_.pop();
    if (reactors_[idx].wantsDrain())
    {
      activeWv_ = idx;
      reactors_[idx].startDrain(mcp_, sd_, config);
    }
  }
}

void ReactorController::updateWv0(ConfigState& config)
{
  bool anyWv = false;
  for (int i = 0; i < ReactorMappings::NUM_REACTORS; ++i)
  {
    if (reactors_[i].isWvActive()) { anyWv = true; break; }
  }

  bool shouldBeOpen = !anyWv;
  if (shouldBeOpen != wv0Open_)
  {
    wv0Open_ = shouldBeOpen;
    mcp_.setPin(McpPins::WV0, wv0Open_);
    sd_.logReactorEvent(-1, wv0Open_ ? "WV0_OPEN" : "WV0_CLOSE", 0);
  }
  config.wv0Open = wv0Open_;
}

void ReactorController::resetLsErrors(ConfigState& config)
{
  for (int i = 0; i < ReactorMappings::NUM_REACTORS; ++i)
    reactors_[i].clearErrors(config);
  sd_.logMessage("LS errors reset");
}

void ReactorController::manualOverrideFv(int reactorIdx, bool on)
{
  if (reactorIdx < 0 || reactorIdx >= ReactorMappings::NUM_REACTORS) return;
  reactors_[reactorIdx].manualSetFv(mcp_, on);
}

void ReactorController::manualOverrideWv(int reactorIdx, bool on)
{
  if (reactorIdx < 0 || reactorIdx >= ReactorMappings::NUM_REACTORS) return;
  reactors_[reactorIdx].manualSetWv(mcp_, on);
}

void ReactorController::manualOverrideWv0(bool open)
{
  wv0Open_ = open;
  mcp_.setPin(McpPins::WV0, open);
}
