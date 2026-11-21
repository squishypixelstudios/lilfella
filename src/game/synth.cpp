#include "synth.h"
#include <math.h>
#include "lcd.h"   // (PI_F, TAU_F)

namespace lf {

// A note: one stretch of one track.
//   DROP  a water drop / bloop: a sine that glides f0 -> f1 fast, then rings out (exp decay)
//   SINE  a plain soft tone, f0 -> f1 (vib = vibrato depth)
//   COO   their voice: a round tone with a little throat in it (vib = wobble depth)
//   SQSH  a squelch: noise squeezed through a moving resonance, f0 -> f1 (vib = how many wobbles in the note)
//   BUB   bubbles: a froth of tiny rising chirps between f0 and f1 (vib = bubbles per second)
//   PUFF  soft air (low-passed noise), f0 -> f1 = brightness sweep (0..1)
//   PURR  a low rumble, chopped
//   REST  silence
struct SynthNote {
  uint8_t wave;
  float f0, f1;
  uint16_t ms;
  float amp;
  float vib;
};
enum { REST, DROP, SINE, COO, SQSH, BUB, PUFF, PURR };

#define T(name) static const SynthNote name[] =
// ---- touch ----
T(A_TAP) {{DROP, 420, 760, 55, 0.55f, 0}};
T(A_KEY) {{DROP, 620, 980, 40, 0.40f, 0}};
T(A_MENU) {{DROP, 380, 620, 60, 0.50f, 0}, {DROP, 520, 900, 80, 0.50f, 0}};
T(A_BACK) {{DROP, 700, 480, 60, 0.45f, 0}, {DROP, 520, 320, 90, 0.45f, 0}};
T(A_PAGE) {{DROP, 560, 700, 45, 0.40f, 0}, {REST, 0, 0, 15, 0, 0}, {DROP, 680, 860, 45, 0.35f, 0}};
T(A_NOPE) {{DROP, 300, 210, 90, 0.60f, 0}, {REST, 0, 0, 40, 0, 0}, {DROP, 280, 180, 130, 0.60f, 0}};
T(B_NOPE) {{SQSH, 500, 300, 260, 0.25f, 2}};
T(A_LIGHT_ON) {{DROP, 500, 1000, 50, 0.45f, 0}, {DROP, 900, 1400, 90, 0.40f, 0}};
T(A_LIGHT_OFF) {{DROP, 900, 520, 60, 0.45f, 0}, {DROP, 480, 260, 120, 0.45f, 0}};
// ---- their voice ----
T(A_COO_SOFT) {{COO, 560, 860, 110, 0.40f, 0.02f}, {COO, 860, 500, 230, 0.40f, 0.05f}};
T(B_COO_SOFT) {{DROP, 300, 520, 70, 0.30f, 0}};
T(A_COO) {{COO, 460, 860, 140, 0.65f, 0.03f}, {COO, 860, 380, 380, 0.65f, 0.06f}};
T(B_COO) {{DROP, 260, 520, 80, 0.40f, 0}, {REST, 0, 0, 260, 0, 0}, {BUB, 500, 900, 180, 0.18f, 26}};
T(A_COO_LOUD) {{COO, 500, 980, 170, 0.95f, 0.03f}, {COO, 980, 420, 500, 0.95f, 0.07f}, {REST, 0, 0, 110, 0, 0},
               {COO, 480, 900, 120, 0.9f, 0.03f}, {COO, 900, 360, 380, 0.9f, 0.07f}};
T(B_COO_LOUD) {{DROP, 260, 560, 90, 0.45f, 0}, {REST, 0, 0, 690, 0, 0}, {DROP, 250, 520, 90, 0.45f, 0}};
T(A_PET) {{PURR, 62, 62, 650, 0.50f, 0}};
T(B_PET) {{SQSH, 380, 700, 160, 0.35f, 1}, {REST, 0, 0, 120, 0, 0}, {DROP, 500, 760, 70, 0.30f, 0}, {REST, 0, 0, 60, 0, 0}, {DROP, 600, 900, 80, 0.28f, 0}};
T(A_YAWN) {{COO, 330, 420, 220, 0.35f, 0.02f}, {COO, 420, 240, 520, 0.38f, 0.05f}};
T(B_YAWN) {{REST, 0, 0, 600, 0, 0}, {DROP, 300, 220, 120, 0.25f, 0}};
T(A_SAD) {{COO, 500, 300, 420, 0.42f, 0.07f}, {REST, 0, 0, 40, 0, 0}, {DROP, 260, 170, 160, 0.35f, 0}};
T(A_GRUMBLE) {{SQSH, 260, 140, 200, 0.70f, 3}, {SQSH, 200, 110, 240, 0.65f, 3}};
T(B_GRUMBLE) {{DROP, 170, 110, 180, 0.45f, 0}, {REST, 0, 0, 40, 0, 0}, {DROP, 150, 95, 200, 0.45f, 0}};
T(A_SICK) {{SQSH, 320, 180, 260, 0.50f, 4}, {REST, 0, 0, 60, 0, 0}, {SQSH, 260, 420, 220, 0.45f, 3}};
T(B_SICK) {{BUB, 180, 420, 540, 0.30f, 14}};
// ---- food and care ----
T(A_EAT) {{SQSH, 900, 380, 70, 0.60f, 1}, {DROP, 300, 190, 70, 0.50f, 0}};
T(A_GULP) {{DROP, 520, 200, 110, 0.60f, 0}, {REST, 0, 0, 30, 0, 0}, {DROP, 240, 560, 120, 0.55f, 0}};
T(B_GULP) {{SQSH, 700, 300, 140, 0.30f, 1}};
T(A_MEDS) {{DROP, 480, 300, 80, 0.50f, 0}, {REST, 0, 0, 50, 0, 0}, {DROP, 440, 280, 80, 0.50f, 0}, {REST, 0, 0, 50, 0, 0}, {DROP, 400, 700, 160, 0.50f, 0}};
T(B_MEDS) {{BUB, 500, 1100, 420, 0.22f, 22}};
T(A_COIN) {{DROP, 700, 1050, 70, 0.50f, 0}, {DROP, 940, 1500, 170, 0.50f, 0}};
T(A_HEART) {{DROP, 640, 1280, 110, 0.45f, 0}};
T(A_PLOP) {{DROP, 460, 150, 120, 0.70f, 0}};
T(B_PLOP) {{REST, 0, 0, 30, 0, 0}, {SQSH, 600, 250, 110, 0.40f, 1}};
T(A_SCRUB) {{SQSH, 1300, 2300, 60, 0.38f, 1}, {SQSH, 2300, 1200, 60, 0.34f, 1}};
T(A_BUBBLE) {{DROP, 500, 1500, 45, 0.55f, 0}};
T(B_BUBBLE) {{PUFF, 0.7f, 0.7f, 30, 0.25f, 0}};
T(A_SPLAT) {{SQSH, 1500, 300, 160, 0.75f, 1}};
T(B_SPLAT) {{DROP, 300, 110, 150, 0.55f, 0}};
T(A_SPARKLE) {{BUB, 900, 2200, 380, 0.34f, 30}};
T(B_SPARKLE) {{DROP, 660, 990, 90, 0.35f, 0}, {DROP, 880, 1320, 90, 0.35f, 0}, {DROP, 1100, 1760, 200, 0.35f, 0}};
T(A_SCOOP) {{SQSH, 500, 1400, 110, 0.50f, 1}, {DROP, 380, 620, 70, 0.40f, 0}};
T(A_POOF) {{PUFF, 0.5f, 0.2f, 130, 0.50f, 0}};
T(B_POOF) {{DROP, 330, 200, 120, 0.40f, 0}};
T(A_SCRATCH) {{SQSH, 800, 500, 70, 0.45f, 1}};
// ---- springy things ----
T(A_BOING) {{DROP, 240, 720, 130, 0.60f, 0}};
T(B_BOING) {{SQSH, 400, 900, 90, 0.25f, 1}};
T(A_BOUNCE) {{DROP, 300, 520, 80, 0.55f, 0}};
T(B_BOUNCE) {{SQSH, 700, 350, 70, 0.35f, 1}};
T(A_BLOW) {{BUB, 300, 700, 260, 0.50f, 34}, {PUFF, 0.5f, 0.15f, 160, 0.40f, 0}};
T(B_BLOW) {{SINE, 700, 330, 420, 0.20f, 0.04f}};
T(A_STRETCH) {{SQSH, 500, 900, 80, 0.70f, 2}};
T(A_WHOOSH) {{PUFF, 0.15f, 0.8f, 200, 0.50f, 0}, {PUFF, 0.8f, 0.2f, 160, 0.35f, 0}};
T(B_WHOOSH) {{DROP, 260, 980, 220, 0.50f, 0}};
T(A_LOCK) {{DROP, 520, 300, 50, 0.55f, 0}, {REST, 0, 0, 20, 0, 0}, {DROP, 260, 170, 110, 0.65f, 0}};
T(B_LOCK) {{REST, 0, 0, 70, 0, 0}, {SQSH, 900, 400, 60, 0.35f, 1}};
T(A_UNLOCK) {{DROP, 240, 420, 60, 0.55f, 0}, {REST, 0, 0, 20, 0, 0}, {DROP, 460, 900, 130, 0.55f, 0}};
T(B_UNLOCK) {{REST, 0, 0, 80, 0, 0}, {BUB, 700, 1400, 160, 0.22f, 28}};
// ---- results ----
T(A_WIN) {{DROP, 440, 523, 95, 0.50f, 0}, {DROP, 560, 659, 95, 0.50f, 0}, {DROP, 670, 784, 95, 0.50f, 0}, {DROP, 880, 1047, 300, 0.55f, 0}};
T(B_WIN) {{REST, 0, 0, 285, 0, 0}, {BUB, 900, 2000, 300, 0.20f, 30}};
// level up / new look: a bright rising run, higher and happier than WIN, with a sparkle on top
T(A_LEVEL) {{DROP, 784, 880, 75, 0.50f, 0}, {DROP, 988, 1109, 75, 0.50f, 0}, {DROP, 1175, 1319, 75, 0.50f, 0}, {DROP, 1568, 1760, 130, 0.52f, 0}, {DROP, 1319, 1480, 70, 0.45f, 0}, {DROP, 1760, 2093, 320, 0.52f, 0}};
T(B_LEVEL) {{REST, 0, 0, 280, 0, 0}, {BUB, 1500, 2800, 400, 0.20f, 30}};
T(A_LOSE) {{DROP, 420, 300, 190, 0.50f, 0}, {DROP, 310, 170, 330, 0.50f, 0}};
T(B_LOSE) {{REST, 0, 0, 190, 0, 0}, {SQSH, 400, 180, 300, 0.30f, 3}};
T(A_MELT) {{SINE, 800, 60, 1300, 0.45f, 0.03f}, {SQSH, 300, 120, 300, 0.40f, 3}};
T(B_MELT) {{BUB, 900, 200, 1500, 0.30f, 18}};

// ---- the title tune: a bouncy march in C, 8 bars of 2.5 s (96 bpm), one sound per bar (the title page plays them in turn;
//      these are never detuned, see start()). A = the melody, B = oom-pah bass ----
T(A_TITLE1) {{DROP, 461, 523.3, 625, 0.50f, 0}, {DROP, 580, 659.3, 625, 0.50f, 0}, {DROP, 690, 784.0, 625, 0.50f, 0}, {DROP, 580, 659.3, 625, 0.50f, 0}};
T(B_TITLE1) {{DROP, 222, 261.6, 625, 0.42f, 0}, {DROP, 329.6, 329.6, 625, 0.22f, 0}, {DROP, 392.0, 392.0, 625, 0.34f, 0}, {DROP, 329.6, 329.6, 625, 0.22f, 0}};
T(A_TITLE2) {{DROP, 774, 880.0, 625, 0.50f, 0}, {DROP, 690, 784.0, 625, 0.50f, 0}, {DROP, 580, 659.3, 625, 0.50f, 0}, {DROP, 461, 523.3, 625, 0.50f, 0}};
T(B_TITLE2) {{DROP, 187, 220.0, 625, 0.42f, 0}, {DROP, 261.6, 261.6, 625, 0.22f, 0}, {DROP, 329.6, 329.6, 625, 0.34f, 0}, {DROP, 261.6, 261.6, 625, 0.22f, 0}};
T(A_TITLE3) {{DROP, 615, 698.5, 625, 0.50f, 0}, {DROP, 774, 880.0, 625, 0.50f, 0}, {DROP, 921, 1046.5, 625, 0.50f, 0}, {DROP, 774, 880.0, 625, 0.50f, 0}};
T(B_TITLE3) {{DROP, 148, 174.6, 625, 0.42f, 0}, {DROP, 220.0, 220.0, 625, 0.22f, 0}, {DROP, 261.6, 261.6, 625, 0.34f, 0}, {DROP, 220.0, 220.0, 625, 0.22f, 0}};
T(A_TITLE4) {{DROP, 869, 987.8, 625, 0.50f, 0}, {DROP, 690, 784.0, 625, 0.50f, 0}, {DROP, 517, 587.3, 625, 0.50f, 0}, {DROP, 690, 784.0, 625, 0.50f, 0}};
T(B_TITLE4) {{DROP, 167, 196.0, 625, 0.42f, 0}, {DROP, 246.9, 246.9, 625, 0.22f, 0}, {DROP, 293.7, 293.7, 625, 0.34f, 0}, {DROP, 246.9, 246.9, 625, 0.22f, 0}};
T(A_TITLE5) {{DROP, 461, 523.3, 312, 0.50f, 0}, {DROP, 580, 659.3, 312, 0.50f, 0}, {DROP, 690, 784.0, 625, 0.50f, 0}, {DROP, 921, 1046.5, 625, 0.50f, 0}, {DROP, 690, 784.0, 625, 0.50f, 0}};
T(B_TITLE5) {{DROP, 222, 261.6, 625, 0.42f, 0}, {DROP, 329.6, 329.6, 625, 0.22f, 0}, {DROP, 392.0, 392.0, 625, 0.34f, 0}, {DROP, 329.6, 329.6, 625, 0.22f, 0}};
T(A_TITLE6) {{DROP, 774, 880.0, 625, 0.50f, 0}, {DROP, 921, 1046.5, 625, 0.50f, 0}, {DROP, 774, 880.0, 312, 0.50f, 0}, {DROP, 690, 784.0, 312, 0.50f, 0}, {DROP, 580, 659.3, 625, 0.50f, 0}};
T(B_TITLE6) {{DROP, 187, 220.0, 625, 0.42f, 0}, {DROP, 261.6, 261.6, 625, 0.22f, 0}, {DROP, 329.6, 329.6, 625, 0.34f, 0}, {DROP, 261.6, 261.6, 625, 0.22f, 0}};
T(A_TITLE7) {{DROP, 615, 698.5, 625, 0.50f, 0}, {DROP, 774, 880.0, 625, 0.50f, 0}, {DROP, 690, 784.0, 625, 0.50f, 0}, {DROP, 869, 987.8, 625, 0.50f, 0}};
T(B_TITLE7) {{DROP, 167, 196.0, 625, 0.42f, 0}, {DROP, 246.9, 246.9, 625, 0.22f, 0}, {DROP, 293.7, 293.7, 625, 0.34f, 0}, {DROP, 246.9, 246.9, 625, 0.22f, 0}};
T(A_TITLE8) {{DROP, 921, 1046.5, 625, 0.50f, 0}, {DROP, 690, 784.0, 625, 0.50f, 0}, {DROP, 580, 659.3, 625, 0.50f, 0}, {DROP, 517, 587.3, 625, 0.50f, 0}};
T(B_TITLE8) {{DROP, 222, 261.6, 625, 0.42f, 0}, {DROP, 329.6, 329.6, 625, 0.22f, 0}, {DROP, 392.0, 392.0, 625, 0.34f, 0}, {DROP, 329.6, 329.6, 625, 0.22f, 0}};
#undef T

struct SoundDef { const SynthNote* a; uint8_t na; const SynthNote* b; uint8_t nb; };
#define ONE(a) {a, (uint8_t)(sizeof(a) / sizeof(a[0])), nullptr, 0}
#define TWO(a, b) {a, (uint8_t)(sizeof(a) / sizeof(a[0])), b, (uint8_t)(sizeof(b) / sizeof(b[0]))}
// (same order as enum Sound in platform.h)
static const SoundDef kSounds[SND_COUNT] = {
  ONE(A_TAP), TWO(A_COO_SOFT, B_COO_SOFT), TWO(A_COO, B_COO), TWO(A_COO_LOUD, B_COO_LOUD), TWO(A_PET, B_PET), ONE(A_EAT), ONE(A_COIN),
  ONE(A_SCOOP), TWO(A_BLOW, B_BLOW), TWO(A_BOING, B_BOING), TWO(A_WIN, B_WIN), TWO(A_LOSE, B_LOSE), ONE(A_SAD), TWO(A_MELT, B_MELT),
  ONE(A_SCRATCH), TWO(A_YAWN, B_YAWN), TWO(A_POOF, B_POOF), TWO(A_GRUMBLE, B_GRUMBLE),
  ONE(A_KEY), ONE(A_MENU), ONE(A_BACK), ONE(A_PAGE), TWO(A_NOPE, B_NOPE), ONE(A_LIGHT_ON), ONE(A_LIGHT_OFF),
  TWO(A_SICK, B_SICK), TWO(A_GULP, B_GULP), TWO(A_MEDS, B_MEDS), ONE(A_HEART), TWO(A_PLOP, B_PLOP), ONE(A_SCRUB),
  TWO(A_BUBBLE, B_BUBBLE), TWO(A_SPLAT, B_SPLAT), TWO(A_SPARKLE, B_SPARKLE), TWO(A_BOUNCE, B_BOUNCE), ONE(A_STRETCH),
  TWO(A_WHOOSH, B_WHOOSH), TWO(A_LOCK, B_LOCK), TWO(A_UNLOCK, B_UNLOCK), TWO(A_LEVEL, B_LEVEL),
  TWO(A_TITLE1, B_TITLE1), TWO(A_TITLE2, B_TITLE2), TWO(A_TITLE3, B_TITLE3), TWO(A_TITLE4, B_TITLE4),
  TWO(A_TITLE5, B_TITLE5), TWO(A_TITLE6, B_TITLE6), TWO(A_TITLE7, B_TITLE7), TWO(A_TITLE8, B_TITLE8)};

static constexpr float REF_SR = 16000.0f;   // the sample rate the filter coefficients below were tuned at (scaled to the real rate)

int synthMillis(Sound s) {
  if (s >= SND_COUNT) return 0;
  const SoundDef& d = kSounds[s];
  int a = 0, b = 0;
  for (int i = 0; i < d.na; i++) a += d.a[i].ms;
  for (int i = 0; i < d.nb; i++) b += d.b[i].ms;
  return a > b ? a : b;
}

float SynthVoice::noise() {
  rng_ ^= rng_ << 13; rng_ ^= rng_ >> 17; rng_ ^= rng_ << 5;
  return ((int32_t)(rng_ & 0xFFFF) - 32768) / 32768.0f;
}

void SynthVoice::begin(Track& t) {
  t.i = 0; t.pos = 0; t.phase = 0; t.lp = t.bp = t.bl = 0; t.bubLen = 0; t.bubPos = 0;
  t.len = t.notes ? (uint32_t)t.notes[0].ms * sr_ / 1000 : 0;
}

void SynthVoice::start(Sound s, int sampleRate, uint32_t seed) {
  a_ = Track(); b_ = Track();
  if (s >= SND_COUNT) return;
  sr_ = sampleRate;
  rng_ = seed ? seed : 0x1234567u;
  noise(); noise();
  tune_ = 1.0f + 0.05f * noise();   // never quite the same twice
  if (s >= SND_TITLE1 && s <= SND_TITLE8) tune_ = 1.0f;   // (music stays in tune)
  const SoundDef& d = kSounds[s];
  a_.notes = d.a; a_.n = d.na; b_.notes = d.b; b_.n = d.nb;
  begin(a_); begin(b_);
}

float SynthVoice::run(Track& t) {
  if (!t.notes) return 0;
  while (t.pos >= t.len) {
    if (++t.i >= t.n) { t.notes = nullptr; return 0; }
    t.pos = 0; t.len = (uint32_t)t.notes[t.i].ms * sr_ / 1000;
    if (t.notes[t.i].wave != t.notes[t.i - 1].wave) { t.bubLen = 0; t.bubPos = 0; }   // (a glide carries its phase over)
  }
  const SynthNote& n = t.notes[t.i];
  const float u = (float)t.pos / (float)t.len, sec = (float)t.pos / sr_;
  t.pos++;
  const float inv = 1.0f / sr_;
  float s = 0;
  switch (n.wave) {
    case REST: return 0;
    case DROP: {   // most of the glide happens at the start; the tail rings and dies away
      const float g = 1.0f - (1.0f - u) * (1.0f - u) * (1.0f - u);
      const float f = (n.f0 + (n.f1 - n.f0) * g) * tune_;
      t.phase += f * inv; if (t.phase >= 1) t.phase -= 1;
      const float atk = sec < 0.004f ? sec / 0.004f : 1.0f;
      const float env = atk * expf(-3.2f * u) * (u > 0.85f ? (1.0f - u) / 0.15f : 1.0f);
      const float p = TAU_F * t.phase;
      s = (sinf(p) + 0.18f * sinf(2 * p)) * env;
      break;
    }
    case SINE: case COO: {
      float f = (n.f0 + (n.f1 - n.f0) * u) * tune_;
      if (n.vib > 0) f *= 1.0f + n.vib * sinf(TAU_F * 6.0f * sec);
      t.phase += f * inv; if (t.phase >= 1) t.phase -= 1;
      float env = (u < 0.08f ? u / 0.08f : 1.0f) * (u > 0.75f ? (1.0f - u) / 0.25f : 1.0f);
      const float p = TAU_F * t.phase;
      if (n.wave == SINE) s = sinf(p) * env;
      else {   // a round, wet little throat: the overtones swell and fade (a gargle), not a steady buzz
        const float wet = 0.5f + 0.5f * sinf(TAU_F * 23.0f * sec);
        s = (0.62f * sinf(p) + (0.20f + 0.18f * wet) * sinf(2 * p) + 0.14f * wet * sinf(3 * p + 0.5f)) * env * (0.88f + 0.12f * wet);
      }
      break;
    }
    case SQSH: {   // noise through a state-variable band-pass whose centre slides and wobbles
      const float wob = n.vib > 0 ? 0.5f - 0.5f * cosf(TAU_F * n.vib * u) : 1.0f;
      float fc = (n.f0 + (n.f1 - n.f0) * u) * (0.7f + 0.6f * wob) * tune_;
      if (fc > sr_ * 0.22f) fc = sr_ * 0.22f;
      const float k = 2.0f * sinf(PI_F * fc * inv), q = 0.22f;
      const float w = noise();
      t.bl += k * t.bp;
      const float hp = w - t.bl - q * t.bp;
      t.bp += k * hp;
      const float env = (u < 0.12f ? u / 0.12f : 1.0f) * (u > 0.6f ? (1.0f - u) / 0.4f : 1.0f);
      s = t.bp * 0.55f * env * (0.35f + 0.65f * wob);
      if (s > 1.5f) s = 1.5f; else if (s < -1.5f) s = -1.5f;
      break;
    }
    case BUB: {   // one tiny rising chirp after another, each at its own pitch
      if (t.bubPos >= t.bubLen) {
        const float r = 0.5f + 0.5f * noise();
        const float lo = n.f0 + (n.f1 - n.f0) * u;   // the froth drifts from f0 towards f1
        t.bubF = lo * (0.75f + 0.6f * r) * tune_;
        const float per = n.vib > 1 ? 1.0f / n.vib : 0.04f;
        t.bubLen = (uint32_t)(per * (0.6f + 0.8f * (0.5f + 0.5f * noise())) * sr_);
        if (t.bubLen < 8) t.bubLen = 8;
        t.bubPos = 0;
      }
      const float bu = (float)t.bubPos / (float)t.bubLen;
      t.bubPos++;
      t.phase += t.bubF * (1.0f + 0.9f * bu) * inv; if (t.phase >= 1) t.phase -= 1;
      const float env = (u < 0.06f ? u / 0.06f : 1.0f) * (u > 0.7f ? (1.0f - u) / 0.3f : 1.0f);
      s = sinf(TAU_F * t.phase) * sinf(PI_F * bu) * env;
      break;
    }
    case PUFF: {
      const float c = n.f0 + (n.f1 - n.f0) * u;
      t.lp += (noise() - t.lp) * (0.04f + 0.5f * c) * (REF_SR * inv);
      const float env = (u < 0.1f ? u / 0.1f : 1.0f) * (1.0f - u);
      s = t.lp * 1.6f * env;
      break;
    }
    default: {   // PURR
      t.lp += (noise() - t.lp) * 0.07f * (REF_SR * inv);
      t.phase += n.f0 * 0.4f * inv; if (t.phase >= 1) t.phase -= 1;
      const float chop = 0.3f + 0.7f * (0.5f + 0.5f * sinf(TAU_F * t.phase));
      const float env = (u < 0.1f ? u / 0.1f : 1.0f) * (u > 0.8f ? (1.0f - u) / 0.2f : 1.0f);
      s = (t.lp * 3.0f + 0.5f * sinf(TAU_F * t.phase * 2.5f)) * chop * env;
      break;
    }
  }
  return s * n.amp;
}

float SynthVoice::next() { return run(a_) + run(b_); }

}  // namespace lf
