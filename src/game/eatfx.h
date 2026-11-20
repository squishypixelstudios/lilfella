// One-of-a-kind eating routines, one per look (Lil Blob keeps the absorb in view.cpp).
// A line-for-line port of the Eating Lab (art/eating/engine.js): same maths, same rounding, so the game
// draws exactly what the previews showed. Each routine draws a whole 40x28 scene for a time t (seconds).
#pragma once
#include <stdint.h>

namespace lf {
namespace eatfx {

enum Routine : uint8_t {
  R_NONE,
  R_WOBBLES, R_JELLY, R_PUDDLE, R_TOASTY, R_MARSHY, R_BOOP, R_CURLY, R_PUFF, R_LOAF, R_NIMBUS, R_BALLBOI,
  R_SPROUT, R_FUBZ, R_DAPPER, R_BEAN, R_BUN, R_SLOSH, R_MALLOW, R_SPIKY, R_BLOBBERT, R_MOCHI, R_CHEEKS,
  R_BIG_BLOBBERT, R_BIG_BALLBOI, R_ALIEN,
  R_COUNT
};

// how long the routine runs (seconds)
float duration(int routine);
// draw routine r at time t into buf (40x28 shades 0..3, cleared first). meal: the big meal (else the snack).
// variant: Marshy's wall (0 left, 1 right). Returns true when the frame should be shown inverted (a lightning flash).
bool render(int routine, double t, bool meal, int variant, uint8_t* buf);
// the sound cues: times (seconds) at which a chomp/gulp sound plays; n returned through count
const float* chomps(int routine, int& count);

}  // namespace eatfx
}  // namespace lf
