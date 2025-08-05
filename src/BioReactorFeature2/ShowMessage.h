#pragma once
#include <Arduino.h>
#include "Lcd.h"
#include "Timer.h"

struct MessageOptions 
{
    String message;
    unsigned long durationMs;
    bool clearAfter;
};

class ShowMessage
{
private:
    String message_;
    Timer timer_;
    unsigned long durationMs_;
    bool isActive_;
    bool clearAfter_;

public:
    ShowMessage();
    void show(const MessageOptions& options);
    void update();
    bool isActive() const;
};

