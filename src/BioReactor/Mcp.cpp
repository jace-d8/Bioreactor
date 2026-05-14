#include "Mcp.h"
#include "Config.h"
#include "Timer.h"

Timer queueTimer(TimingIntervals::QUEUE_TIMER);
Timer valveTimer(TimingIntervals::VALVE_TIMER);

bool Mcp::begin()
{
  if (!mcp_.begin_I2C()) return false;

  for (int i = 0; i < 6; ++i)
    mcp_.pinMode(i, OUTPUT);

  resetState(nullptr);
  return true;
}

bool Mcp::enqueue(int valveId)
{
  if (valveQueue_.size() >= 6)
    return false;

  valveQueue_.push(valveId);
  return true;
}

int Mcp::update(bool queued[])
{
  int openedValve = -1;

  if (queueTimer.isReady() && !valveQueue_.empty() && !openValve_)
  {
    currentValve_ = valveQueue_.front();
    valveQueue_.pop();

    mcp_.digitalWrite(currentValve_, HIGH);
    openValve_ = true;

    queued[currentValve_] = false;
    openedValve = currentValve_;

    valveTimer.reset();
  }

  if (valveTimer.isReady() && openValve_)
  {
    mcp_.digitalWrite(currentValve_, LOW);
    openValve_ = false;
    currentValve_ = -1;
  }

  return openedValve;
}

void Mcp::resetState(bool queued[])
{
  while (!valveQueue_.empty())
    valveQueue_.pop();

  for (int i = 0; i < 6; ++i)
  {
    mcp_.digitalWrite(i, LOW);

    if (queued)
      queued[i] = false;
  }

  openValve_ = false;
  currentValve_ = -1;

  queueTimer.reset();
  valveTimer.reset();
}