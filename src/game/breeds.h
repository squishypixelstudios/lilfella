// Hidden breeds: decided by care when they turn 1 day old. They change how easily they put on weight and
// how often they blow bubbles; a Void fella is also drawn filled in dark grey.
#pragma once
#include <stdint.h>

namespace lf {

// Order matters (saved): matches BREEDS[].
enum Species : uint8_t { SP_PLAIN, SP_PALE, SP_SPECKLE, SP_VOID, SP_PATCHY, SP_SMOOTH, SP_BIG, SP_COUNT };

struct Breed {
  const char* name;
  float gain;         // how easily it gains weight
  float bubbles;      // how often they blow bubbles (multiplier)
};

constexpr int NUM_BREEDS = 7;
extern const Breed BREEDS[NUM_BREEDS];

}  // namespace lf
