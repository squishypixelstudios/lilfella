// Detects "someone picked me up" and "someone is shaking me" from accelerometer samples.
#pragma once
#include <stdint.h>

namespace lf {

class MotionDetector {
 public:
  void feed(float ax, float ay, float az, uint32_t nowMs);  // call at ~20-60 Hz
  bool shake();    // returns true once per detected shake gesture
  bool pickup();   // returns true once per detected lift / tilt / carry
  uint32_t flatFor(uint32_t nowMs) const { return flatSince_ ? nowMs - flatSince_ : 0; }   // ms lying flat and still (0 = not)
 private:
  bool init_ = false;
  float fast_[3] = {0, 0, 1}, slow_[3] = {0, 0, 1};
  float dev_ = 0;
  uint32_t peaks_[3] = {0, 0, 0};
  uint32_t lastPeak_ = 0;
  uint32_t lastPickup_ = 0;
  uint32_t carryStart_ = 0;
  uint32_t flatSince_ = 0;
  bool shakeFlag_ = false, pickFlag_ = false;
  bool pickCool(uint32_t now) const { return lastPickup_ && (now - lastPickup_) < 3000; }
};

}  // namespace lf
