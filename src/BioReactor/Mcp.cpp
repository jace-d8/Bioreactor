#include "Mcp.h"
#include "Timer.h"
#include "Config.h"

Timer queueTimer(TimingIntervals::QUEUE_TIMER); // 12 seconds 
Timer valveTimer(TimingIntervals::VALVE_TIMER); // 10 seconds

void Mcp::enqueue(int activeValveId)
{
  if (valveQueue_.size() >= 6) 
  {
    return;
  }
  if (!deactivate_ && activeValveId > 0 && activeValveId <= 6) 
  {
    valveQueue_.push(activeValveId);
  }
}

void Mcp::update(bool queued[])
{
  // openValve_ = UPDATE VALVE LOCK HERE
  if(queueTimer.isReady() && !valveQueue_.empty() && !deactivate_ && !openValve_) 
  {
    currentValve_ = valveQueue_.front(); 
    queued[currentValve_] = false;
    valveQueue_.pop();

    mcp_.digitalWrite(currentValve_, HIGH);
    openValve_ = true; 

    queueTimer.reset();
    valveTimer.reset();
  } 

  if(valveTimer.isReady() && openValve_)
  {
    openValve_ = false; 
    mcp_.digitalWrite(currentValve_, LOW); // removed -1, if code disfunctions this is why
    currentValve_ = -1;
  }
}




