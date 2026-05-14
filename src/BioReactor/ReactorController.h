#pragma once
#include <Arduino.h>
#include <queue>
#include "Config.h"
#include "Reactor.h"

class Mcp;
class SdLogger;

class ReactorController
{
public:
  ReactorController(Mcp& mcp, SdLogger& sd);
  void begin();
  void update(ConfigState& config);

  void resetLsErrors(ConfigState& config);
  void manualOverrideFv(int reactorIdx, bool on);
  void manualOverrideWv(int reactorIdx, bool on);
  void manualOverrideWv0(bool open);

  Reactor& reactor(int i) { return reactors_[i]; }

  int activeFvReactor() const { return activeFv_; }
  int activeWvReactor() const { return activeWv_; }
  bool wv0Open() const { return wv0Open_; }

private:
  void evaluateQueues(ConfigState& config);
  void serviceActive(ConfigState& config);
  void updateWv0(ConfigState& config);
  bool queueContains(const std::queue<int>& q, int idx) const;

  Mcp& mcp_;
  SdLogger& sd_;

  Reactor reactors_[ReactorMappings::NUM_REACTORS];

  std::queue<int> feedQueue_;
  std::queue<int> wasteQueue_;

  int activeFv_;
  int activeWv_;
  bool wv0Open_;
};
