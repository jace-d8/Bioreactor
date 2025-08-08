#include "EzoBoard.h"
#include "Timer.h"
#include "Config.h"
#include <Arduino.h>

Timer ezoTimer(TimingIntervals::EZO_READ_INTERVAL);


EzoBoard::EzoBoard(int a, String s)
  : ezo_(a, s.c_str()), address_(a), probeType_(s) {}

float EzoBoard::read()
{
  switch (state_)
  {
    case State::Reading:   
      ezo_.send_cmd("R");
      ezoTimer.reset();
      state_ = State::Waiting;
      return lastValue_;

    case State::Waiting:
      if(ezoTimer.isReady())
      {
        state_ = State::Receiving;
      }
      return lastValue_;

    case State::Receiving:
      ezo_.receive_cmd(response_, sizeof(response_));
      lastValue_= atof(response_); // or set "probe value" to this 
      state_ = State::Reading;
      return lastValue_;
  }
}

void EzoBoard::sendCmd(String cmd)
{
  ezo_.send_cmd(cmd.c_str());
}
