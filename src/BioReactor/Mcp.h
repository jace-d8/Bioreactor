#pragma once
#include <Adafruit_MCP23X17.h>
#include <queue>
#define VALVE_COUNT 6

// Valve id numbers will be 1-6
class Mcp
{ 
private:  
  Adafruit_MCP23X17 mcp_;
  std::queue<int> valveQueue_;
  int currentValve_{-1};
  bool openValve_{false};
  bool deactivate_{false};

public: 
  Mcp() = default;
  
  bool begin()               
  {
    if (!mcp_.begin_I2C())   // optional: check return
      return false;

    for (int i = 0; i < VALVE_COUNT; ++i)
      mcp_.pinMode(i, OUTPUT);

    return true;
  }

  void enqueue(int activeValveId);  // just push into queue
  void update(bool queued[]);   
};
