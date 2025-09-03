#include "Timer.h"

Timer::Timer(unsigned long interval)
  : lastTrigger_(0), interval_(interval) {}

bool Timer::isReady()
{
  unsigned long current = millis();
  if (current - lastTrigger_ >= interval_)
  {
    lastTrigger_ = current;
    return true;
  }
  return false;
}

void Timer::reset()
{
  lastTrigger_ = millis();
}
