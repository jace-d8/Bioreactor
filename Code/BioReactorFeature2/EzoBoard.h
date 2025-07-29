#pragma once
#include <Ezo_i2c.h>

class EzoBoard {
private:
  Ezo_board ezo_;
  int address_;
  String probeType_;
  char response_[32];

public:
  EzoBoard(int a, String s)
    : ezo_(a, s.c_str()), address_(a), probeType_(s) {}

  float read()
  {
    ezo_.send_cmd("R");
    delay(900);
    ezo_.receive_cmd(response_, sizeof(response_));
    return atof(response_);
  }

  void sendCmd(String cmd)
  {
    ezo_.send_cmd(cmd.c_str());
  }
};
