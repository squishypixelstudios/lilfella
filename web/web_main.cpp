// WebAssembly host: runs the real game core in the browser (same code as the firmware).
#include "../src/game/game.h"
#include "../src/game/synth.h"
#include <string.h>
using namespace lf;

namespace {
struct WebPlat : Platform {
  uint32_t ms = 0;
  int64_t epochOffset = 0;
  bool idleAutoOff() override { return false; }   // the page stays lit while you watch (off after 3 untouched minutes)
  uint8_t store[2048]; uint32_t storeLen = 0; bool storeDirty = false;
  int16_t sndq[32]; int sh = 0, st = 0;
  uint32_t seed = 1;
  uint32_t millis() override { return ms; }
  bool rtcGet(DateTime& dt) override { dt = fromEpoch((uint32_t)(epochOffset + ms / 1000)); return true; }
  void rtcSet(const DateTime& dt) override { epochOffset = (int64_t)toEpoch(dt) - ms / 1000; }
  bool loadSave(void* buf, uint32_t len) override { if (storeLen != len) return false; memcpy(buf, store, len); return true; }
  bool storeSave(const void* buf, uint32_t len) override { if (len > sizeof store) return false; memcpy(store, buf, len); storeLen = len; storeDirty = true; return true; }
  void playSound(Sound s, uint8_t vol) override { int n = (st + 1) & 31; if (n != sh) { sndq[st] = (int16_t)(s | (vol << 8)); st = n; } }
  uint32_t randomSeed() override { return seed; }
};
WebPlat P;
Game* G = nullptr;
Frame FB;
uint8_t OUT[368 * 448 * 4];
}  // namespace

#define EXP(n) __attribute__((export_name(n)))
extern "C" {
EXP("fb_ptr") uint8_t* fb_ptr() { return OUT; }
EXP("save_ptr") uint8_t* save_ptr() { return P.store; }
EXP("save_len") int save_len() { return (int)P.storeLen; }
EXP("save_set_len") void save_set_len(int n) { P.storeLen = (uint32_t)n; }
EXP("save_dirty") int save_dirty() { int d = P.storeDirty; P.storeDirty = false; return d; }
EXP("snd_pop") int snd_pop() { if (P.sh == P.st) return -1; int s = P.sndq[P.sh]; P.sh = (P.sh + 1) & 31; return s; }
// a sound, rendered whole: returns the number of samples written at snd_buf() (floats, -1..1)
static float SNDBUF[48000 * 2];
EXP("snd_buf") float* snd_buf() { return SNDBUF; }
EXP("snd_count") int snd_count() { return SND_COUNT; }
EXP("snd_render") int snd_render(int id, int rate, uint32_t seed) {
  if (id < 0 || id >= SND_COUNT) return 0;
  SynthVoice v; v.start((Sound)id, rate, seed);
  int n = 0;
  while (!v.done() && n < (int)(sizeof SNDBUF / sizeof SNDBUF[0])) SNDBUF[n++] = v.next();
  return n;
}
EXP("brightness") int brightness() { return G ? G->brightness() : 200; }
EXP("display_on") int display_on() { return G ? G->displayOn() : 1; }
EXP("screen") int screen() { return G ? (int)G->scr : 0; }
EXP("set_clock") void set_clock(int y, int mo, int d, int h, int mi, int s) { P.rtcSet({(uint16_t)y, (uint8_t)mo, (uint8_t)d, (uint8_t)h, (uint8_t)mi, (uint8_t)s}); }
EXP("init") void init(uint32_t seed, int y, int mo, int d, int h, int mi, int s) {
  P.seed = seed; P.ms = 1000;
  P.rtcSet({(uint16_t)y, (uint8_t)mo, (uint8_t)d, (uint8_t)h, (uint8_t)mi, (uint8_t)s});
  if (!G) G = new Game(&P);
  G->begin();
}
EXP("tick") int tick(uint32_t ms, int down, int x, int y) {   // x,y in 368x448 physical px
  P.ms = ms + 1000;
  bool r = G->frame(FB, down != 0, x, y);
  if (r) {
    const Palette& pal = G->palette();
    uint8_t rgb[4][3];
    for (int i = 0; i < 4; i++) { uint16_t c = pal.c[i]; rgb[i][0] = (uint8_t)(((c >> 11) & 31) * 255 / 31); rgb[i][1] = (uint8_t)(((c >> 5) & 63) * 255 / 63); rgb[i][2] = (uint8_t)((c & 31) * 255 / 31); }
    for (int py = 0; py < 448; py++) {
      const uint8_t* src = FB.px + (py / 2) * W;
      uint8_t* o = OUT + py * 368 * 4;
      for (int px = 0; px < 368; px++) { const uint8_t* c = rgb[src[px / 2] & 3]; o[0] = c[0]; o[1] = c[1]; o[2] = c[2]; o[3] = 255; o += 4; }
    }
  }
  return r ? 1 : 0;
}
EXP("fast_forward") void fast_forward(int minutes) { if (G) G->simulate(minutes * 60.0f, false); }
EXP("timescale") void timescale(int n) { if (G) G->timeScale = n < 1 ? 1 : n; }
EXP("debug_act") void debug_act(int w) { if (G) G->debugAct(w); }
EXP("set_age_hours") void set_age_hours(int h) { if (G && G->sv.hasBlob) { G->pet.d.age = h * 3600.0f; if (G->pet.d.age >= BABY_END) G->pet.d.pad2 |= 0x40FF; } }   // jumping past the Lil Blob skips its lessons
}
