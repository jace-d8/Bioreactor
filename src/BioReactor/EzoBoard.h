#pragma once
#include <Ezo_i2c.h>
#include "Timer.h"
#include "Config.h"

class EzoBoard {
private:
  Ezo_board ezo_;
  float lastValue_{0};
  bool valid_{false};
  char response_[32];
  Timer readTimer_{TimingIntervals::EZO_READ_INTERVAL};

public:
  enum class ProbeState { Reading, Waiting, Receiving } state_ = ProbeState::Reading;

  EzoBoard(int address, String s);

  float read();
  bool hasValidReading() const { return valid_; }

  void sendCmd(const char* cmd);
  bool exportCalibration(char* out, size_t outLen);
  bool importCalibration(const char* payload);
  void resetReadState();


  void setTemperature(float tempC);
};