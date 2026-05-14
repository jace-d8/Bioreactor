#pragma once
#include <Arduino.h>

class DebouncedInput
{
public:
  DebouncedInput(uint8_t pin, unsigned long confirmMs)
    : pin_(pin), confirmMs_(confirmMs), current_(false), last_(false), lastChange_(0) {}

  void begin()
  {
    pinMode(pin_, INPUT);
    bool raw = digitalRead(pin_);
    current_ = raw;
    last_ = raw;
    lastChange_ = millis();
  }

  void update()
  {
    bool raw = digitalRead(pin_);
    if (raw != last_)
    {
      last_ = raw;
      lastChange_ = millis();
    }
    if (raw != current_ && (millis() - lastChange_) >= confirmMs_)
    {
      current_ = raw;
    }
  }

  bool read() const { return current_; }
  void setConfirmMs(unsigned long ms) { confirmMs_ = ms; }

private:
  uint8_t pin_;
  unsigned long confirmMs_;
  bool current_;
  bool last_;
  unsigned long lastChange_;
};
