#include "Valve.h"

Valve::Valve(int pin, unsigned long duration)
  : pin_(pin), openDuration_(duration), isOpen_(false), enable_(true), startTime_(0) {}

void Valve::open()
{
  if (!isOpen_ && enable_)
  {
    digitalWrite(pin_, HIGH);
    startTime_ = millis();
    isOpen_ = true;
  }
}

void Valve::update()
{
  if (isOpen_ && (millis() - startTime_ >= openDuration_))
  {
    digitalWrite(pin_, LOW);
    isOpen_ = false;
  }
}

bool Valve::isOpen()
{
  return isOpen_;
}

void Valve::switchValve()
{
  enable_ = !enable_;
}

bool Valve::isEnabled()
{
  return enable_;
}
