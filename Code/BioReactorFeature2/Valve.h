#pragma once
#include <Arduino.h>

class Valve {
private:
  int pin_;
  bool isOpen_;
  bool enable_;
  unsigned int startTime_;
  const unsigned long openDuration_;

public:
  Valve(int p, unsigned long d)
    : pin_(p), openDuration_(d), isOpen_(false), enable_(true), startTime_(0) {}

  void open()
  {
    if (!isOpen_ && enable_)
    {
      digitalWrite(pin_, HIGH);
      startTime_ = millis();
      isOpen_ = true;
    }
  }

  void update()
  {
    if (isOpen_ && (millis() - startTime_ >= openDuration_))
    {
      digitalWrite(pin_, LOW);
      isOpen_ = false;
    }
  }

  bool isValveOpen()
  {
    return isOpen_;
  }

  void switchValve()
  {
    enable_ = !enable_;
  }

  bool isEnabled()
  {
    return enable_;
  }
};
