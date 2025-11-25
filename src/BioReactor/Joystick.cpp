#include "Joystick.h"

void Joystick::move()
{
    int yVal = analogRead(pinY_);
    const int deadZone = 400; 
    const int center = 1980; 

    if (yVal < center - deadZone) 
    { // Up
        if (selectedItem_ > 0) 
            selectedItem_--;
        else 
            selectedItem_ = maxIndex_; // Wrap to last
        delay(200); // Prevent rapid scrolling
    } 
    else if (yVal > center + deadZone) 
    { // Down
        if (selectedItem_ < maxIndex_) 
            selectedItem_++;
        else 
            selectedItem_ = 0; // Wrap to first
        delay(200);
    }
}

bool Joystick::isPressed()
{
    bool pressed = !digitalRead(pinSW_); // Active low

    if (pressed && debounceTimer_.isReady()) 
    {
        debounceTimer_.reset(); // Restart debounce timer
        return true;
    }
    return false;
}

int Joystick::getSelectedItem() const
{
    return selectedItem_;
}

void Joystick::setSelectedItem(int index)
{
    if (index >= 0 && index <= maxIndex_)
        selectedItem_ = index;
}

int Joystick::getMaxIndex() const
{
  return maxIndex_; 
}

void Joystick::setMaxIndex(int newMaxIndex)
{
  maxIndex_ = newMaxIndex;
}

