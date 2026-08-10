#pragma once
#include <Arduino.h>
#include "Config.h"
#include "Timer.h"

class Joystick 
{
private:
    int pinY_, pinSW_;
    int deadZone_, range_;
    unsigned long debounceMs_;
    int selectedItem_;
    int maxIndex_;
    Timer debounceTimer_;
public:

    Joystick(int pinY, int pinSW, int maxIndex)
        : pinY_(pinY), pinSW_(pinSW), maxIndex_(maxIndex),
        selectedItem_(0), debounceMs_(JoystickConfig::SWITCH_DEBOUNCE_MS), debounceTimer_(debounceMs_)
    {
        pinMode(pinSW_, INPUT_PULLUP); // Active-low button
    }

    void move();
    int yAxisStep();
    bool isPressed();

    int getSelectedItem() const;
    void setSelectedItem(int index);

    int getMaxIndex() const;
    void setMaxIndex(int newMaxIndex);
};
