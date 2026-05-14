#pragma once

#include <queue>
#include <Adafruit_MCP23X17.h>

class Mcp
{
public:
  bool begin();
  bool enqueue(int valveId);
  int update(bool queued[]);
  void resetState(bool queued[]);
  void setPin(int pin, bool high);

private:
  Adafruit_MCP23X17 mcp_;
  std::queue<int> valveQueue_;
  bool openValve_{false};
  int currentValve_{-1};
};