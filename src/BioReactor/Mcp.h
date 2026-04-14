#pragma once
#include <Adafruit_MCP23X17.h>
#include <queue>

class Mcp
{
private:
  Adafruit_MCP23X17 mcp_;
  std::queue<int> valveQueue_;
  int currentValve_{-1};
  bool openValve_{false};

public:
  bool begin();
  bool enqueue(int valveId);
  int update(bool queued[]);
};