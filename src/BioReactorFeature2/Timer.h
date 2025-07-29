class Timer {
  unsigned long lastTrigger;
  unsigned long interval;
public:
  Timer(unsigned long interval) : lastTrigger(0), interval(interval) {}

  bool isReady() 
  {
    unsigned long current = millis();
    if (current - lastTrigger >= interval) 
    {
      lastTrigger = current;
      return true;
    }
    return false;
  }

  void reset() { lastTrigger = millis(); }
};
