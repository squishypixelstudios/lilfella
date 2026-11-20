// The Lil Fella's voice and every other sound, synthesised: wet bloops, squelches and bubbles (no beeps).
// One definition table, shared by the firmware (streamed to the codec) and the web page (rendered to a buffer),
// so both sound the same.
#pragma once
#include <stdint.h>
#include "platform.h"

namespace lf {

// One playing sound. start() it, then pull samples (-1..1) with next() until done().
class SynthVoice {
 public:
  void start(Sound s, int sampleRate, uint32_t seed);
  float next();
  bool done() const { return !a_.notes && !b_.notes; }
 private:
  struct Track {
    const struct SynthNote* notes = nullptr;
    int n = 0, i = 0;
    uint32_t pos = 0, len = 0;
    float phase = 0, lp = 0, bp = 0, bl = 0;   // oscillator phase; noise filters
    float bubF = 0; uint32_t bubPos = 0, bubLen = 0;   // the bubble being blown (BUB)
  };
  float run(Track& t);
  void begin(Track& t);
  float noise();
  Track a_, b_;
  int sr_ = 16000;
  float tune_ = 1.0f;
  uint32_t rng_ = 1;
};

// Length of a sound in milliseconds (its longer track).
int synthMillis(Sound s);

}  // namespace lf
