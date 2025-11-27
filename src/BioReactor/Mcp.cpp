#include "Mcp.h"
#include "Timer.h"

Timer queueTimer(12000); // 10 seconds (for now)
Timer valveTimer(10000);

void Mcp::enqueue(int activeValveId)
{
  if (!deactivate_ && activeValveId > 0 && activeValveId <= 6) 
  {
    valveQueue_.push(activeValveId);
  }
}

void Mcp::update()
{
  if(queueTimer.isReady() && !valveQueue_.empty() && !deactivate_ && !openValve_) 
  {
    currentValve_ = valveQueue_.front(); 
    valveQueue_.pop();

    mcp_.digitalWrite(currentValve_ - 1, HIGH);
    openValve_ = true; 

    queueTimer.reset();
    valveTimer.reset();
  } 

  if(valveTimer.isReady() && openValve_)
  {
    openValve_ = false; 
    mcp_.digitalWrite(currentValve_ - 1, LOW);
    currentValve_ = -1;
  }
}

// push into queue once
// pop front of queue
// open valve for n seconds 
// "shutdown" queue while valve open




