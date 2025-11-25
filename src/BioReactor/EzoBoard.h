// EzoBoard.h
#pragma once
#include <Ezo_i2c.h>
#include "Timer.h"

class EzoBoard {
private:
  Ezo_board ezo_;
  int address_;
  float lastValue_{0};
  String probeType_;
  char response_[32];
  Timer readTimer_{900}; // replace with config constant 900U
public:
  enum class ProbeState { Reading, Waiting, Receiving } state_ = ProbeState::Reading;
  EzoBoard(int address, String s);
  float read();
  void sendCmd(String cmd);
};
