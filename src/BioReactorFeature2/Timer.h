#pragma once
#include <Arduino.h>

class Timer 
{
private:
  unsigned long lastTrigger_;
  unsigned long interval_;

public:
  Timer(unsigned long interval);
  bool isReady();
  void reset();
};
