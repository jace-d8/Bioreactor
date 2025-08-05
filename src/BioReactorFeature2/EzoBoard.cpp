#include "EzoBoard.h"
#include "Timer.h"
#include "Config.h"
#include <Arduino.h>

Timer ezoTimer(TimingIntervals::EZO_READ_INTERVAL);

EzoBoard::EzoBoard(int a, String s)
  : ezo_(a, s.c_str()), address_(a), probeType_(s) {}

float EzoBoard::read()
{
  ezo_.send_cmd("R");
  // if(ezoTimer.isReady())
  // {
  delay(900);
  ezo_.receive_cmd(response_, sizeof(response_));
  return atof(response_);
  // }
}

void EzoBoard::sendCmd(String cmd)
{
  ezo_.send_cmd(cmd.c_str());
}
