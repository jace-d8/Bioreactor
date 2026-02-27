#pragma once
#include <Arduino.h>

class Timer 
{
private:
  unsigned long lastTrigger_;
  unsigned long interval_;

public:
  Timer(unsigned long interval);
  /**
  * @brief Compares time elapsed - last trigger to interval
  * @returns True if the timer is ready, false otherwise
  */
  bool isReady();
  /**
  * @brief sets lastTrigger_ to current time elapsed
  */
  void reset();
};
