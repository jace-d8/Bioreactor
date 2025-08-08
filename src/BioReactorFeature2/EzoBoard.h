#pragma once
#include <Ezo_i2c.h>

class EzoBoard 
{
private:
  Ezo_board ezo_;
  int address_;
  float lastValue_; 
  String probeType_;
  char response_[32];

public:
  enum class ProbeState { Reading, Waiting, Receiving } state_ = ProbeState::Reading;

  EzoBoard(int a, String s);
  float read();
  void sendCmd(String cmd);
};
