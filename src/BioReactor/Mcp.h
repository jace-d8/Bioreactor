#pragma once

#include <Adafruit_MCP23X17.h>
#include "Config.h"

// Fixed-size ring buffer — no heap allocation, safe for long-running embedded use.
template<int N>
struct ValveQueue {
    int  buf[N]  = {};
    int  head    = 0;
    int  tail    = 0;
    int  count_  = 0;

    bool push(int v)  { if (count_ >= N) return false; buf[tail] = v; tail = (tail + 1) % N; ++count_; return true; }
    int  front() const { return buf[head]; }
    void pop()         { head = (head + 1) % N; --count_; }
    bool empty() const { return count_ == 0; }
    int  size()  const { return count_; }
    void clear()       { head = tail = count_ = 0; }
};

class Mcp
{
public:
  bool begin();

  // Route a valve into the correct (pH or ORP) queue.
  // pH valves: ids 0, 2, 4  |  ORP valves: ids 1, 3, 5
  bool enqueue(int valveId);

  // Call every loop iteration.
  // phValveTimerSec / orpValveTimerSec are the runtime-editable open durations.
  // Queue timers are automatically set to (valveTimer + 2 s).
  // Fills opened[0..1] with the valve ids opened this cycle (pH side first,
  // then ORP side).  Returns the number of valves opened (0, 1, or 2).
  // Using an output array instead of a single return value ensures that
  // simultaneous pH + ORP opens are both logged and counted.
  int update(bool queued[], int phValveTimerSec, int orpValveTimerSec, int opened[2]);

  void resetState(bool queued[]);

private:
  Adafruit_MCP23X17 mcp_;

  // pH queue (valves 0, 2, 4) — max 3 entries
  ValveQueue<3>  phQueue_;
  bool           phValveOpen_{false};
  int            phCurrentValve_{-1};
  unsigned long  phValveStarted_{0};
  unsigned long  phLastDequeue_{0};

  // ORP queue (valves 1, 3, 5) — max 3 entries
  ValveQueue<3>  orpQueue_;
  bool           orpValveOpen_{false};
  int            orpCurrentValve_{-1};
  unsigned long  orpValveStarted_{0};
  unsigned long  orpLastDequeue_{0};

  int processSide_(ValveQueue<3>& q,
                   bool& valveOpen, int& currentValve,
                   unsigned long& valveStarted, unsigned long& lastDequeue,
                   unsigned long valveMs, unsigned long queueMs,
                   bool queued[]);
};
