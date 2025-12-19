class ActionTimer {
public:
  ActionTimer(unsigned long durationMs)
    : duration_(durationMs), start_(0), running_(false) {}

  void start() {
    start_ = millis();
    running_ = true;
  }

  void stop() {
    running_ = false;
  }

  bool done() const {
    if (!running_) return true;  // idle == done
    return millis() - start_ >= duration_;
  }

private:
  unsigned long duration_;
  unsigned long start_;
  bool running_;
};
