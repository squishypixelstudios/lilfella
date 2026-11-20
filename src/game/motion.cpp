#include "motion.h"
#include <math.h>

namespace lf {

void MotionDetector::feed(float ax, float ay, float az, uint32_t now) {
  float mag = sqrtf(ax * ax + ay * ay + az * az);
  if (mag < 0.2f) return;  // free fall / bad sample
  if (!init_) {
    fast_[0] = slow_[0] = ax; fast_[1] = slow_[1] = ay; fast_[2] = slow_[2] = az;
    init_ = true;
    return;
  }
  dev_ = fabsf(mag - 1.0f);

  // --- shake: three hard jolts inside one second ---
  if (dev_ > 0.7f && (now - lastPeak_) > 90) {
    lastPeak_ = now;
    peaks_[0] = peaks_[1];
    peaks_[1] = peaks_[2];
    peaks_[2] = now;
    if (peaks_[0] && now - peaks_[0] < 1000) {
      shakeFlag_ = true;
      peaks_[0] = peaks_[1] = peaks_[2] = 0;
    }
  }

  // --- pickup: orientation drifted away from the long-term average, or sustained jostling ---
  const float af = 0.2f, as = 0.004f;
  for (int i = 0; i < 3; i++) {
    float v = i == 0 ? ax : (i == 1 ? ay : az);
    fast_[i] += (v - fast_[i]) * af;
    slow_[i] += (v - slow_[i]) * as;
  }
  float fm = sqrtf(fast_[0] * fast_[0] + fast_[1] * fast_[1] + fast_[2] * fast_[2]);
  float sm = sqrtf(slow_[0] * slow_[0] + slow_[1] * slow_[1] + slow_[2] * slow_[2]);
  if (fm > 0.1f && sm > 0.1f) {
    float dot = (fast_[0] * slow_[0] + fast_[1] * slow_[1] + fast_[2] * slow_[2]) / (fm * sm);
    if (dot > 1) dot = 1;
    if (dot < -1) dot = -1;
    float ang = acosf(dot) * 57.2958f;
    if (ang > 20.0f && !pickCool(now)) {
      pickFlag_ = true;
      lastPickup_ = now;
      for (int i = 0; i < 3; i++) slow_[i] = fast_[i];
    }
  }
  // lying flat (face up or down) and still: gravity along the screen's axis, no jostling
  const bool flat = fm > 0.5f && fabsf(fast_[2]) / fm > 0.95f && dev_ < 0.06f;
  if (!flat) flatSince_ = 0;
  else if (!flatSince_) flatSince_ = now ? now : 1;
  if (dev_ > 0.18f) {
    if (!carryStart_) carryStart_ = now;
    if (now - carryStart_ > 700 && !pickCool(now)) {
      pickFlag_ = true;
      lastPickup_ = now;
      carryStart_ = 0;
    }
  } else {
    carryStart_ = 0;
  }
}

bool MotionDetector::shake() { bool r = shakeFlag_; shakeFlag_ = false; return r; }
bool MotionDetector::pickup() { bool r = pickFlag_; pickFlag_ = false; return r; }

}  // namespace lf
