#include "ShowMessage.h"

ShowMessage::ShowMessage()
    : timer_(0), durationMs_(0), isActive_(false), clearAfter_(true) {}

void ShowMessage::show(const MessageOptions& options)
{
    message_ = options.message;
    durationMs_ = options.durationMs;
    clearAfter_ = options.clearAfter;
    timer_ = Timer(durationMs_);
    isActive_ = true;

    lcd.clear();
    lcd.print(message_);
}

void ShowMessage::update()
{
    if (isActive_ && clearAfter_ && timer_.isReady())
    {
        lcd.clear();
        isActive_ = false;
    }
}

bool ShowMessage::isActive() const
{
  return isActive_;
}
