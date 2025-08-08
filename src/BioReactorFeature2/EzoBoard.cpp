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
    case ProbeState::Reading:   
      ezo_.send_cmd("R");
      ezoTimer.reset();
      state_ = ProbeState::Waiting;
      return lastValue_;

    case ProbeState::Waiting:
      if(ezoTimer.isReady())
      {
        state_ = ProbeState::Receiving;
      }
      return lastValue_;

    case ProbeState::Receiving:
      ezo_.receive_cmd(response_, sizeof(response_));
      lastValue_= atof(response_); // or set "probe value" to this 
      state_ = ProbeState::Reading;
      return lastValue_;
  }
}

void EzoBoard::sendCmd(String cmd)
{
  ezo_.send_cmd(cmd.c_str());
}
