#include "EzoBoard.h"
#include <stdlib.h>

EzoBoard::EzoBoard(int address, String s)
  : ezo_(address, s.c_str()) {}

float EzoBoard::read()
{
  switch (state_)
  {
    case ProbeState::Reading:
      ezo_.send_cmd("R");
      readTimer_.reset();
      state_ = ProbeState::Waiting;
      return lastValue_;

    case ProbeState::Waiting:
      if (readTimer_.isReady())
        state_ = ProbeState::Receiving;
      return lastValue_;

    case ProbeState::Receiving:
      ezo_.receive_cmd(response_, sizeof(response_));

      char* end;
      float val = strtof(response_, &end);

      if (end != response_)
      {
        lastValue_ = val;
        valid_ = true;
      }
      else
      {
        valid_ = false;
      }

      state_ = ProbeState::Reading;
      return lastValue_;
  }
}

void EzoBoard::setTemperature(float tempC)
{
  String cmd = "T," + String(tempC, 1);
  ezo_.send_cmd(cmd.c_str());
}

void EzoBoard::sendCmd(String cmd)
{
  ezo_.send_cmd(cmd.c_str());
}
