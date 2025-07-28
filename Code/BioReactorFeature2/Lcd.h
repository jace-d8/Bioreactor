//
// Created by Jace Dunn on 7/14/25.
//

#ifndef VALVE_H
#define VALVE_H

class Valve{
private:
  int pin;
  bool isOpen;
  bool enable;
  unsigned int startTime;
  const unsigned long openDuration;
public:
  Valve(int p, unsigned long d) : pin(p), openDuration(d), isOpen(false), enable(true), startTime(0) {}

  void open()
  {
    if(!isOpen && enable)
    {
      digitalWrite(pin, HIGH);
      startTime = millis();
      isOpen = true;
    }
  }
  void update()
  {
    if (isOpen && (millis() - startTime >= openDuration))
    {
      digitalWrite(pin, LOW);
      isOpen = false;
    }
  }
  bool isValveOpen()
  {
    return isOpen;
  }
  void switchValve()
  {
    enable = !enable;
  }
  bool isEnabled()
  {
    return enable;
  }
};
