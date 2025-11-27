#pragma once
#include <Adafruit_MCP23X17.h>
#include <queue>

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
  Mcp() 
  {
    mcp_.begin_I2C();
    for(int i = 0; i < 6; ++ i)
    {
      mcp_.pinMode(i, OUTPUT);
    }
  }
  void enqueue(int activeValveId);  // just push into queue
  void update();   
};
