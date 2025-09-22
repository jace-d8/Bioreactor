#pragma once
#include <Arduino.h>

class Valve 
{
private:
  int pin_;
  bool isOpen_;
  bool enable_;
  unsigned long startTime_;
  const unsigned long openDuration_;

public:
  Valve(int pin, unsigned long duration);
  void open();
  void close(); 
  void update();
  bool isOpen();
  void switchValve();
  bool isEnabled();
};
