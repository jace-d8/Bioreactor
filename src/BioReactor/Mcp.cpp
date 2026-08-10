#include "Mcp.h"
#include "Config.h"

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
  const bool isPh = (valveId % 2 == 0);
  return isPh ? phQueue_.push(valveId) : orpQueue_.push(valveId);
}

int Mcp::processSide_(ValveQueue<3>& q,
                      bool& valveOpen, int& currentValve,
                      unsigned long& valveStarted, unsigned long& lastDequeue,
                      unsigned long valveMs, unsigned long queueMs,
                      bool queued[])
{
  const unsigned long now = millis();
  int opened = -1;

  // Close the valve once it has been open long enough.
  if (valveOpen && (now - valveStarted >= valveMs))
  {
    mcp_.digitalWrite(currentValve, LOW);
    valveOpen    = false;
    currentValve = -1;
  }

  // Open the next queued valve if nothing is open and enough time has
  // passed since the last dequeue (queue timer = valve duration + 2 s).
  if (!valveOpen && !q.empty() && (now - lastDequeue >= queueMs))
  {
    currentValve = q.front();
    q.pop();

    mcp_.digitalWrite(currentValve, HIGH);
    valveOpen    = true;
    valveStarted = now;
    lastDequeue  = now;

    if (queued)
      queued[currentValve] = false;
    opened = currentValve;
  }

  return opened;
}

int Mcp::update(bool queued[], int phValveTimerSec, int orpValveTimerSec, int opened[2])
{
  const unsigned long phMs  = (unsigned long)phValveTimerSec  * 1000UL;
  const unsigned long orpMs = (unsigned long)orpValveTimerSec * 1000UL;

  int count = 0;

  // Process each side independently and record every valve that opens.
  // Both can open in the same call (independent queues and timers), so
  // both must be reported — a single return value would silently drop one.
  int r = processSide_(phQueue_,  phValveOpen_,  phCurrentValve_,
                       phValveStarted_, phLastDequeue_,
                       phMs, phMs + 2000UL, queued);
  if (r >= 0) opened[count++] = r;

  r = processSide_(orpQueue_, orpValveOpen_, orpCurrentValve_,
                   orpValveStarted_, orpLastDequeue_,
                   orpMs, orpMs + 2000UL, queued);
  if (r >= 0) opened[count++] = r;

  return count;
}

void Mcp::resetState(bool queued[])
{
  phQueue_.clear();
  orpQueue_.clear();

  for (int i = 0; i < 6; ++i)
  {
    mcp_.digitalWrite(i, LOW);
    if (queued)
      queued[i] = false;
  }

  phValveOpen_     = false;
  phCurrentValve_  = -1;
  orpValveOpen_    = false;
  orpCurrentValve_ = -1;

  phValveStarted_ = orpValveStarted_ = 0;
  phLastDequeue_  = orpLastDequeue_  = 0;
}
