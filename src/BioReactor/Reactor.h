#pragma once
#include <Arduino.h>
#include "Config.h"
#include "DebouncedInput.h"

class Mcp;
class SdLogger;

class Reactor
{
public:
  Reactor(int id, uint8_t lsAPin, uint8_t lsBPin, int fvMcpPin, int wvMcpPin);

  void begin();
  void updateSensors(ConfigState& config);

  bool wantsFill() const;
  bool wantsDrain() const;
  bool isFvActive() const { return fvOpen_; }
  bool isWvActive() const { return wvOpen_; }
  bool errorLatched() const { return lsAError_ || lsBError_; }

  void markQueuedForFill(ConfigState& config);
  void markQueuedForDrain(ConfigState& config);
  void startFill(Mcp& mcp, SdLogger& sd, ConfigState& config);
  void startDrain(Mcp& mcp, SdLogger& sd, ConfigState& config);

  void serviceFill(Mcp& mcp, SdLogger& sd, ConfigState& config);
  void serviceDrain(Mcp& mcp, SdLogger& sd, ConfigState& config);

  void tick(ConfigState& config);

  void clearErrors(ConfigState& config);

  void manualSetFv(Mcp& mcp, bool on);
  void manualSetWv(Mcp& mcp, bool on);

  int id() const { return id_; }
  ReactorState state() const { return state_; }
  bool lsA() const { return lsAConfirmed_; }
  bool lsB() const { return lsBConfirmed_; }
  bool lsAError() const { return lsAError_; }
  bool lsBError() const { return lsBError_; }

private:
  void closeFv(Mcp& mcp, SdLogger& sd, ConfigState& config);
  void closeWv(Mcp& mcp, SdLogger& sd, ConfigState& config);
  void writeStateToConfig(ConfigState& config);

  int id_;
  int fvPin_;
  int wvPin_;

  DebouncedInput lsAInput_;
  DebouncedInput lsBInput_;

  bool lsAConfirmed_;
  bool lsBConfirmed_;

  ReactorState state_;

  bool fvOpen_;
  bool wvOpen_;
  unsigned long fvOpenedAt_;
  unsigned long wvOpenedAt_;

  unsigned long holdStartedAt_;
  bool holding_;

  bool lsAError_;
  bool lsBError_;
};
