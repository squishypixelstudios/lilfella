#include <time.h>
// Host-side simulator: runs the real game core with a fake board.
//   ./sim shots <outdir>    scripted tour of the screens (PPM frames, 368x448, palette applied)
//   ./sim life              raises fellas with simulated players and prints how they turn out
//   ./sim flows             taps through settings / menus and checks where it ends up
#include "../src/game/game.h"
#include "../src/game/breeds.h"
#include "../src/game/synth.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <algorithm>
#include <math.h>

using namespace lf;

struct SimPlat : Platform {
  uint32_t ms = 1000;
  int64_t epochOffset;
  bool autoOff = false;   // the Auto off timer: off for the scripted tests (they leave the screen untouched for long stretches)
  bool idleAutoOff() override { return autoOff; }
  std::vector<uint8_t> store;
  int soundCount[SND_COUNT] = {0};
  SimPlat() { epochOffset = (int64_t)toEpoch({2026, 11, 2, 18, 9, 0}) - ms / 1000; }
  uint32_t millis() override { return ms; }
  bool rtcGet(DateTime& dt) override { dt = fromEpoch((uint32_t)(epochOffset + ms / 1000)); return true; }
  void rtcSet(const DateTime& dt) override { epochOffset = (int64_t)toEpoch(dt) - ms / 1000; }
  bool button = false;   // PWR / BOOT pressed (one event)
  bool powerButton() override { const bool b = button; button = false; return b; }
  bool loadSave(void* buf, uint32_t len) override { if (store.size() != len) return false; memcpy(buf, store.data(), len); return true; }
  bool storeSave(const void* buf, uint32_t len) override { store.assign((const uint8_t*)buf, (const uint8_t*)buf + len); return true; }
  void playSound(Sound s, uint8_t) override { soundCount[s]++; }
  int batt = -1;   // battery percent (-1: none fitted)
  int batteryPercent() override { return batt; }
  uint32_t seed = 0xC0FFEE;
  uint32_t randomSeed() override { return seed; }
  void setClock(int h, int m) { DateTime d = fromEpoch((uint32_t)(epochOffset + ms / 1000)); d.hour = h; d.minute = m; d.second = 0; rtcSet(d); }
};

static Frame fb;
static int shotNo = 0;
static const char* outDir = ".";

static void step(Game& g, SimPlat& p, bool down = false, int x = 0, int y = 0) { p.ms += 33; g.frame(fb, down, x * 2, y * 2); }
static void run(Game& g, SimPlat& p, float sec) { int n = (int)(sec * 30); for (int i = 0; i < n; i++) step(g, p); }
static void tap(Game& g, SimPlat& p, int x, int y) { step(g, p, true, x, y); step(g, p, true, x, y); step(g, p); step(g, p); }
static void tapDot(Game& g, SimPlat& p, int dx, int dy) { tap(g, p, MX + dx * DOT + 1, MY + dy * DOT + 1); }
// POP (the bubble shooter): a bot. popBotAngle looks along every aim, sees where each shot would land (the game's own
// popTrace) and takes the one that pops the most; failing that, the one that lands beside the most of its own kind,
// as high up as it can. `wobble` (radians) adds a human's unsteady hand; rs is its random state.
static float popBotAngle(const Game& g, float wobble, uint32_t& rs) {
  const Game::Pop& P = g.pop_;
  float bestA = 0; int bestS = -100000;
  for (int i = -50; i <= 50; i++) {
    const float a = i * (POP_MAX_ANG / 50.0f);
    int r, c;
    if (!g.popTrace(a, r, c)) continue;
    bool mark[POP_ROWS][POP_COLS] = {};
    int stR[64], stC[64], sp = 0, n = 0;
    stR[sp] = r; stC[sp++] = c; mark[r][c] = true;
    while (sp) {
      const int rr = stR[--sp], cc = stC[sp]; n++;
      int nr[6], nc[6]; const int k = Game::popNeighbours(rr, cc, nr, nc);
      for (int j = 0; j < k; j++) if (!mark[nr[j]][nc[j]] && P.g[nr[j]][nc[j]] == P.cur) { mark[nr[j]][nc[j]] = true; stR[sp] = nr[j]; stC[sp++] = nc[j]; }
    }
    int score = n >= 3 ? 1000 + n * 50 : n * 40 - (r + P.drops) * 15;
    if (r + P.drops >= POP_SAFE_ROWS) score -= 5000;   // over the line: only if there's nothing else
    score -= abs(i) / 5;                               // (ties: the straighter shot)
    if (score > bestS) { bestS = score; bestA = a; }
  }
  if (wobble > 0) { rs = rs * 1664525u + 1013904223u; const float u = ((rs >> 8) & 0xFFFF) / 65535.0f - 0.5f; bestA += u * 2 * wobble; }
  return bestA < -POP_MAX_ANG ? -POP_MAX_ANG : bestA > POP_MAX_ANG ? POP_MAX_ANG : bestA;
}
static int popAimX(float ang) { return W / 2 + (int)lroundf(ang / POP_MAX_ANG * 74.0f); }   // the finger's x for an aim (the reverse of Game::popAngleAt)
// fire one shot with a finger: down on the LCD at that aim, held a moment, lifted; then wait for it to land
static void popShoot(Game& g, SimPlat& p, float ang) {
  const int x = popAimX(ang);
  step(g, p, true, x, MY + 60); step(g, p, true, x, MY + 60); step(g, p);
  for (int i = 0; i < 30 * 6 && g.scr == S_POP && g.pop_.phase == 2; i++) step(g, p);
}
// play a whole game (from the card to the moment it's over). Returns the shots fired.
static int popPlay(Game& g, SimPlat& p, float wobble = 0, uint32_t seed = 1) {
  uint32_t rs = seed * 2654435761u + 7;
  for (int i = 0; i < 30 * 6 && g.scr == S_POP && g.pop_.card > 0; i++) step(g, p);
  int shots = 0;
  for (; g.scr == S_POP && g.pop_.phase == 1 && shots < 400; shots++) popShoot(g, p, popBotAngle(g, wobble, rs));
  return shots;
}
// Clean: rub a finger back and forth over the LCD until it's clean (returns the seconds it took)
static float scrubSolve(Game& g, SimPlat& p) {
  int i = 0;
  for (; i < 30 * 90 && g.scr == S_SCRUB && !g.sc_.done; i++) { const int k = i % 16, x = MX + 20 + (k < 8 ? k : 16 - k) * 16; step(g, p, true, x, MY + 60); }   // a brisk scrub: 4 dots a frame
  step(g, p);
  return i / 30.0f;
}
// the ending: three squeezes, the dropper rises, then the name page opens by itself
static void endingFlow(Game& g, SimPlat& p) {
  run(g, p, ENDING_WAIT_AT);
  for (int i = 0; i < ENDING_PULLS; i++) { for (int k = 0; k < 4; k++) step(g, p, true, MX + 80, MY + 50); step(g, p); run(g, p, 0.8f); }
  run(g, p, 3.2f);
}
static void tapIcon(Game& g, SimPlat& p, int ic) { tap(g, p, iconX(ic) + 8, iconY(ic) + 8); }
static void shot(Game& g, const char* name) {
  char fn[256];
  snprintf(fn, sizeof fn, "%s/%02d_%s.ppm", outDir, ++shotNo, name);
  FILE* f = fopen(fn, "wb");
  fprintf(f, "P6\n%d %d\n255\n", W * 2, H * 2);
  const Palette& pal = g.palette();
  for (int y = 0; y < H * 2; y++)
    for (int x = 0; x < W * 2; x++) {
      uint16_t c = pal.c[fb.px[(y / 2) * W + x / 2] & 3];
      uint8_t rgb[3] = {(uint8_t)(((c >> 11) & 31) * 255 / 31), (uint8_t)(((c >> 5) & 63) * 255 / 63), (uint8_t)((c & 31) * 255 / 31)};
      fwrite(rgb, 1, 3, f);
    }
  fclose(f);
}

static int cmdShots() {
  SimPlat p;
  Game g(&p);
  g.begin();
  if (g.scr != S_SETUP_CLOCK) printf("FAIL first screen is %d, not the clock\n", (int)g.scr);
  run(g, p, 0.3f); shot(g, "clock");
  tap(g, p, 92, 195);                 // OK
  run(g, p, 7.6f); shot(g, "title"); tap(g, p, 92, 100);   // the title screen: tap to start
  run(g, p, 0.3f); shot(g, "name");
  tap(g, p, 92, 182);                 // FILL IT
  run(g, p, 1.0f); shot(g, "dropper");
  tapDot(g, p, 20, 14); run(g, p, 0.3f); shot(g, "squeeze_1"); run(g, p, 0.8f); shot(g, "squeeze_2"); run(g, p, 0.5f); shot(g, "squeeze_3");
  g.pet.d.pad2 = 0xE0FF;   // lessons done (tested in `lessons`), nap had
  run(g, p, SQUEEZE_SECS - 1.6f + 0.1f);   // the rest of the squeeze and the fall
  g.simulate(61, false); run(g, p, 0.2f); run(g, p, 1.0f); shot(g, "squeezed_out");
  run(g, p, 2.0f); shot(g, "baby");
  tapIcon(g, p, IC_FEED); run(g, p, 0.3f); shot(g, "feed_menu");
  tapDot(g, p, 10, 4); run(g, p, 0.3f); shot(g, "baby_eat");
  run(g, p, 7); tapIcon(g, p, IC_METER); run(g, p, 0.2f); shot(g, "meter_0");
  tapDot(g, p, 20, 14); run(g, p, 0.2f); shot(g, "meter_hungry");
  for (int k = 2; k < 8; k++) { tapIcon(g, p, IC_METER); run(g, p, 0.2f); char nm[24]; snprintf(nm, sizeof nm, "meter_%d", k); shot(g, nm); }
  tapIcon(g, p, IC_METER); run(g, p, 0.3f);
  // stage 0 with mess and gifts
  g.pet.d.age = 3 * HOUR; g.pet.d.pad0[0] = 1; g.pet.d.droppings = 2; g.pet.d.bubblesOnFloor = 2; g.pet.d.hunger = 60; g.pet.d.fun = 60;
  run(g, p, 3); shot(g, "stage0_mess");
  tapIcon(g, p, IC_LITTER); run(g, p, 0.9f); shot(g, "cleaning");
  run(g, p, 1.5f);
  g.pet.d.health = 20; run(g, p, 0.5f); run(g, p, 0.5f); shot(g, "sick");
  tapIcon(g, p, IC_MEDS); run(g, p, 1.0f); shot(g, "medicine");
  run(g, p, 2);
  // play
  tapIcon(g, p, IC_PLAY); run(g, p, 0.2f); shot(g, "play_menu");
  tapDot(g, p, 10, 4); run(g, p, 1.2f); shot(g, "dodge");
  for (int i = 0; i < 300 && g.scr == S_DODGE; i++) step(g, p);
  run(g, p, 0.5f);
  // growing up: the picker opens at each stage; flip through every look, then pick
  g.scr = S_MAIN; g.pet.d.droppings = 0; g.pet.d.bubblesOnFloor = 0; g.pet.d.health = 90; g.pet.d.pad0[0] = 0;
  const float ages[3] = {BABY_END + 60, CHILD_END + 60, TEEN_END + 60};
  const int counts[3] = {3, 3, 7};
  for (int st = 0; st < 3; st++) {
    g.pet.d.age = ages[st]; g.pet.sleeping = false; run(g, p, 0.3f);
    if (g.anim() == AN_LEVELUP) { char nm[32]; for (int k = 0; k < 6; k++) { snprintf(nm, sizeof nm, "levelup_%d_%d", st + 1, k); shot(g, nm); run(g, p, 0.36f); } }
    for (int i = 0; i < 30 * 12 && g.scr != S_EVOLVE; i++) { g.idleSec = 0; step(g, p); }
    if (g.scr != S_EVOLVE) printf("FAIL no picker at stage %d (screen %d an %d locked %d pend %d)\n", st + 1, (int)g.scr, (int)g.anim(), (int)g.locked(), g.pendingEvolution());
    for (int i = 0; i < counts[st]; i++) {
      char nm[32]; snprintf(nm, sizeof nm, "pick%d_%d", st + 1, i); shot(g, nm);
      if (i + 1 < counts[st]) { tapDot(g, p, MW - 2, 14); run(g, p, 0.1f); }
    }
    tapDot(g, p, MW - 2, 14); run(g, p, 0.1f);   // wrap back to the first option
    tapDot(g, p, MW / 2, 20); run(g, p, 0.4f);
    if (g.scr != S_ANIM) printf("FAIL pick at stage %d\n", st + 1);
    run(g, p, 2.5f);
    char nm[32]; snprintf(nm, sizeof nm, "grown_stage%d", st + 1); shot(g, nm);
  }
  g.pet.d.giftOnFloor = GIFT_FLOWER; g.pet.d.bubblesOnFloor = 1; g.pet.d.floorMess = 1; g.pet.d.droppings = 2; run(g, p, 1); shot(g, "adult_gift");
  g.pet.d.giftOnFloor = 0; g.pet.d.bubblesOnFloor = 0; g.pet.d.floorMess = 0; g.pet.d.droppings = 0;
  g.pet.d.pad0[0] = 42; run(g, p, 3); shot(g, "adult_loaf");
  g.pet.d.breed = SP_VOID; g.pet.d.evolving = 0; run(g, p, 1); shot(g, "adult_void");
  g.pet.d.breed = SP_PLAIN;
  // sleeping, light off
  g.pet.sleeping = true; run(g, p, 1); shot(g, "sleeping");
  tapIcon(g, p, IC_LIGHT); run(g, p, 1.0f); shot(g, "lights_off");
  tapIcon(g, p, IC_LIGHT); g.pet.sleeping = false; run(g, p, 1);
  // lock
  p.setClock(11, 0); run(g, p, 1.5f);   // daytime: they're awake
  tapIcon(g, p, IC_LOCK); run(g, p, 0.3f); shot(g, "lock_ask");
  tapDot(g, p, 10, 22); run(g, p, 0.5f); shot(g, "lock_1"); run(g, p, 0.6f); shot(g, "lock_2"); run(g, p, 2.0f); shot(g, "locked_home");
  tapIcon(g, p, IC_LOCK); run(g, p, 0.3f); shot(g, "unlock_ask");
  tapDot(g, p, 10, 22); run(g, p, 0.5f); shot(g, "unlock_1"); run(g, p, 2.5f);
  run(g, p, 2);
  tapDot(g, p, 2, 2); run(g, p, 0.3f); shot(g, "clock_view");
  // settings (long press)
  for (int i = 0; i < 45; i++) step(g, p, true, 92, 70);
  step(g, p); run(g, p, 0.2f); shot(g, "settings");
  g.sv.eq[0] = 1; run(g, p, 0.2f); shot(g, "settings_night");
  tap(g, p, 92, SETTINGS_Y0 + 4 * SETTINGS_STEP + 12); run(g, p, 0.2f); shot(g, "settings_page2");
  tap(g, p, 92, SETTINGS_Y0 + 0 * SETTINGS_STEP + 12); run(g, p, 0.2f); shot(g, "friends_empty");
  tap(g, p, 92, 209); run(g, p, 0.2f); tap(g, p, 92, SETTINGS_Y0 + 1 * SETTINGS_STEP + 12); run(g, p, 0.2f); shot(g, "journal"); { const uint32_t j0 = g.sv.journal; g.sv.journal |= (1u << F_S0B) | (1u << F_S0) | (1u << F_TN_TOASTY) | (1u << F_TN_BUN) | (1u << F_AD_LOAF) | (1u << F_AD_PUDDLE) | (1u << F_AD_FUBZ) | (1u << F_BB_ALIEN) | (1u << F_BB_BALLBOI); tap(g, p, 30, 120); run(g, p, 0.2f); shot(g, "journal_some"); g.sv.journal = j0; } tap(g, p, 92, 209); run(g, p, 0.2f);
  tap(g, p, 92, SETTINGS_Y0 + 0 * SETTINGS_STEP + 12); run(g, p, 0.2f);
  tap(g, p, 92, 209); run(g, p, 0.2f);                                       // friends Back -> Settings
  tap(g, p, 92, SETTINGS_Y0 + 5 * SETTINGS_STEP + 12); run(g, p, 0.2f);   // page 2 Back -> page 1
  tap(g, p, 92, SETTINGS_Y0 + 5 * SETTINGS_STEP + 12); run(g, p, 0.3f); shot(g, "main_night");
  g.sv.eq[0] = 0;
  // ending
  if (getenv("DBGE")) printf("pre scr %d an %d locked %d ended %d\n", (int)g.scr, (int)g.anim(), g.locked(), g.pet.ended);
  g.pet.d.resent = 100; g.pet.d.hunger = 0; g.pet.d.fun = 0; g.simulate(5, false); run(g, p, 2); shot(g, "ending_no_trust");
  if (getenv("DBGE")) printf("post scr %d an %d resent %.0f ended %d\n", (int)g.scr, (int)g.anim(), g.pet.d.resent, g.pet.ended);
  endingFlow(g, p);   // squeeze three times: the name page
  if (getenv("DBGE")) printf("after flow scr %d\n", (int)g.scr);
  // a new blob, then the dissolve: too much chonk
  tap(g, p, 92, 120); run(g, p, 0.3f); tap(g, p, 92, 182); run(g, p, 0.5f);
  if (g.scr != S_MAIN) printf("FAIL no new blob after the no-trust ending (screen %d)\n", (int)g.scr);
  tapDot(g, p, 20, 14);
  { const float at[] = {0.6f, 1.8f, 2.6f, 3.6f, 4.6f, 5.3f, 6.2f}; float done = 0; int k = 0;
    for (float a : at) { run(g, p, a - done); done = a; char nm[24]; snprintf(nm, sizeof nm, "fall_%d", k++); shot(g, nm); } }
  run(g, p, 0.5f);
  g.pet.d.pad2 = 0xE0FF; g.pet.d.nextSick = 1e9f; g.pet.d.age = BABY_END - 600; run(g, p, 0.5f);
  g.pet.d.chonk = 101; g.simulate(2, false); run(g, p, 0.2f);
  if (g.scr != S_ENDING) printf("FAIL no dissolve ending (screen %d)\n", (int)g.scr);
  { const float at[] = {0.5f, 1.8f, 2.8f, 4.0f, 5.5f}; float done = 0.2f; int k = 0;
    for (float a : at) { run(g, p, a - done); done = a; char nm[24]; snprintf(nm, sizeof nm, "dissolve_%d", k++); shot(g, nm); } }
  run(g, p, 3.0f); shot(g, "dissolve_5_still_waiting");                       // the dropper waits for you
  for (int i = 0; i < 15; i++) step(g, p, true, MX + 80, MY + 50);         // press and hold: the bulb squeezed
  shot(g, "dissolve_6_squeeze"); step(g, p); run(g, p, 0.7f); shot(g, "dissolve_7_one_third");   // let go: a third sucked up
  if (g.scr != S_ENDING) printf("FAIL one squeeze left the ending\n");
  for (int i = 0; i < 4; i++) step(g, p, true, MX + 80, MY + 50); step(g, p); run(g, p, 0.7f); shot(g, "dissolve_8_two_thirds");
  for (int i = 0; i < 4; i++) step(g, p, true, MX + 80, MY + 50); step(g, p); run(g, p, 1.2f); shot(g, "dissolve_9_last");
  run(g, p, 1.2f); shot(g, "dissolve_10_rising");                            // the dropper lifts away, full of goo
  run(g, p, 1.0f); shot(g, "dissolve_11_name_page");
  if (g.scr != S_SETUP_NAME) printf("FAIL after the dissolve: screen %d\n", (int)g.scr);
  // games, directly
  tap(g, p, 92, 182); tapDot(g, p, 20, 14); run(g, p, SQUEEZE_SECS + 0.2f); g.simulate(13 * 3600, false); g.scr = S_MAIN;
  g.pet.d.hunger = 90; g.pet.d.fun = 50; g.pet.sleeping = false;
  g.debugAct(5); run(g, p, 0.3f);
  // a finger sliding back and forth across the LCD
  for (int i = 0; i < 30 * 8 && g.scr == S_DODGE; i++) { int x = MX + 20 + (int)(120 * (0.5f + 0.5f * sinf(i * 0.05f))); step(g, p, true, x, MY + 80); if (i == 150) shot(g, "dodge_run"); }
  step(g, p);
  for (int i = 0; i < 30 * 15 && g.scr == S_DODGE; i++) step(g, p);   // hands off: they get hit
  shot(g, "dodge_result");
  if (g.scr != S_DODGE) printf("FAIL dodge left early\n");
  tapDot(g, p, 20, 10); run(g, p, 0.3f);
  g.pet.d.age = CHILD_END + 60; g.pet.d.pad0[0] = 0x05; run(g, p, 3);
  for (int i = 0; i < 60; i++) { step(g, p); if (i == 30) shot(g, "teen_walkframe"); }
  // litter boxes
  g.scr = S_MAIN; g.pet.sleeping = false; g.pet.d.pad1[1] = 0; g.pet.d.breed = SP_PLAIN; run(g, p, 3.5f);
  g.pet.d.droppings = 2; g.pet.d.floorMess = 0; run(g, p, 0.5f); shot(g, "box0_two");
  g.pet.d.droppings = 5; g.pet.d.floorMess = 2; run(g, p, 0.5f); shot(g, "box0_full_floor");
  tapIcon(g, p, IC_LITTER); run(g, p, 1.6f); shot(g, "litter_game_start");
  scrubSolve(g, p);
  run(g, p, 0.3f); shot(g, "litter_done");
  run(g, p, 2.0f);
  tap(g, p, 92, 110); run(g, p, 0.3f);
  g.pet.d.droppings = 0; g.pet.d.digestTimer = 1; g.pet.d.resent = 0;
  g.simulate(2, false); run(g, p, 0.4f); shot(g, "potty_walk"); run(g, p, 1.0f); shot(g, "potty_squat"); run(g, p, 0.8f); shot(g, "potty_done");
  run(g, p, 2);
  g.pet.d.digestTimer = 1; g.pet.d.resent = 90; g.pet.d.droppings = 2;
  g.simulate(2, false); run(g, p, 0.6f); shot(g, "potty_floor");
  // eating, every look: open mouth, then the chomp
  {
    struct L { float age; uint8_t pad; const char* nm; } looks[] = {
      {600, 0, "baby"}, {BABY_END + 60, 1, "round"}, {BABY_END + 60, 2, "mochi"}, {BABY_END + 60, 3, "cheeks"},
      {CHILD_END + 60, 3 | 1 << 2, "bean"}, {CHILD_END + 60, 1 | 2 << 2, "bun"}, {CHILD_END + 60, 1 | 3 << 2, "curly"},
      {TEEN_END + 60, 41, "puddle"}, {TEEN_END + 60, 37, "jelly"}, {TEEN_END + 60, 39, "dapper_adult"},
      {CHILD_END + 60, 5, "sprout"}, {TEEN_END + 60, 69, "dango"}, {TEEN_END + 60, 105, "puff"},
      {TEEN_END + 60, 42, "loaf"}};
    for (auto& l : looks) {
      g.scr = S_MAIN; g.pet.d.age = l.age; g.pet.d.pad0[0] = l.pad; g.pet.sleeping = false; g.pet.d.pad1[1] = 0; run(g, p, 2.5f);
      g.debugAct(0); run(g, p, 0.2f);
      char nm[40]; snprintf(nm, sizeof nm, "eat_%s_open", l.nm); shot(g, nm);
      run(g, p, 0.4f); snprintf(nm, sizeof nm, "eat_%s_chomp", l.nm); shot(g, nm);
      run(g, p, 2.5f);
    }
  }
  // sleeping with the lights off: the Zs over a second or so
  g.scr = S_MAIN; g.pet.d.age = BABY_END - 600; g.pet.d.pad0[0] = 0; g.pet.sleeping = true; g.pet.d.pad1[1] = 1; run(g, p, 3.0f); g.idleSec = 100; g.pet.sleeping = true; g.pet.d.pad1[1] = 1;
  for (int i = 0; i < 4; i++) { run(g, p, 0.6f); g.pet.sleeping = true; char nm[24]; snprintf(nm, sizeof nm, "zzz_dark_%d", i); shot(g, nm); }
  g.pet.d.pad1[1] = 0;
  for (int i = 0; i < 2; i++) { run(g, p, 0.6f); g.pet.sleeping = true; char nm[24]; snprintf(nm, sizeof nm, "zzz_light_%d", i); shot(g, nm); }
  printf("%d shots, screen %d\n", shotNo, (int)g.scr);
  return 0;
}

// Raise fellas with a simple "player" who checks in every few hours and answers the attention icon.
static int cmdLife() {
  // style: how often the player checks in; the last ones only check in the daytime (8am-10pm), like a real person
  const int checks[] = {600, 1800, 3600, 2 * 3600, 4 * 3600, 6 * 3600, 8 * 3600, 12 * 3600, 2 * 3600, 4 * 3600, 7 * 3600, 14 * 3600};
  const int NS = sizeof checks / sizeof checks[0], DAYONLY = 8;
  for (int style = 0; style < NS; style++) {
    SimPlat p;
    Game g(&p);
    g.begin();
    tap(g, p, 92, 195); tap(g, p, 92, 100); tap(g, p, 92, 182); tapDot(g, p, 20, 14); run(g, p, SQUEEZE_SECS + 0.2f);
    const int every = checks[style];
    // the first few minutes: you're there when they hatch (a feed, a game, the starter bug's medicine)
    for (int k = 0; k < 6; k++) { g.simulate(60, false); bool o; if (g.pet.d.hunger < 75 && g.pet.canFeed()) g.pet.feed(FOOD_KIBBLE, o); if (g.pet.d.fun < 70) g.pet.exercise(25, 1); while (g.pet.sick()) g.pet.vetVisit(); }
    int t = 0, m12 = -1, m18 = -1;
    int reasonMin[16] = {0};
    while (g.sv.hasBlob && t < 40 * 86400) {
      if (style >= DAYONLY) {   // a day person: checks at 8am and every few hours after, the last one by 10pm
        g.simulate(3600, false); t += 3600;
        if (getenv("DBGHOUR") && style == atoi(getenv("DBGHOUR")) && t < 2 * 86400) printf("    h %02d health %5.1f hunger %3.0f fun %3.0f energy %3.0f sick %d/%d/%d sleep %d mess %d/%d chonk %.0f\n", g.hour, g.pet.d.health, g.pet.d.hunger, g.pet.d.fun, 0.0f, g.pet.d.sick[0], g.pet.d.sick[1], g.pet.d.sick[2], g.pet.sleeping, g.pet.d.droppings, g.pet.d.floorMess, g.pet.d.chonk);
        if (!g.sv.hasBlob) break;
        if (g.hour < 8 || g.hour > 22 || (g.hour - 8) % (every / 3600)) continue;
      } else {
        if (style == 1 && t < 12 * 3600) { for (int q = 0; q < every; q += 60) { g.simulate(60, false); reasonMin[g.callReason()]++; } } else g.simulate(every, false);
        t += every;
        if (!g.sv.hasBlob) break;
      }
      if (getenv("DBGLIFE") && style == atoi(getenv("DBGLIFE")) && t < 3 * 86400) printf("  t %5.1fh %02d:00 age %5.1fh health %3.0f hunger %3.0f fun %3.0f sick %d/%d/%d sleep %d light %d\n", t / 3600.0f, g.hour, g.pet.d.age / 3600, g.pet.d.health, g.pet.d.hunger, g.pet.d.fun, g.pet.d.sick[0], g.pet.d.sick[1], g.pet.d.sick[2], g.pet.sleeping, g.pet.d.pad1[1] & 1);
      if (m12 < 0 && g.pet.d.age >= 12 * HOUR) m12 = g.pet.d.pad1[0];
      if (m18 < 0 && g.pet.d.age >= 18 * HOUR) m18 = g.pet.d.pad1[0];
      // answer the attention icon (and top their up a little), like a player who hears the beep on their next check
      if (g.pet.sleeping && g.pet.d.hunger < 40) g.pet.wakeGently();
      for (int k = 0; k < 3 && g.pet.d.hunger < 75 && g.pet.canFeed(); k++) { bool o; g.pet.feed(FOOD_KIBBLE, o); }
      for (int k = 0; k < 3 && g.pet.d.fun < 70; k++) g.pet.exercise(25, 1);
      if (g.pet.d.droppings || g.pet.d.floorMess) { g.pet.scoop(6); while (g.pet.d.floorMess) g.pet.cleanFloor(); }
      while (g.pet.sick()) g.pet.vetVisit();   // one dose per sickness
      if (g.pet.sleeping || g.hour >= 21 || g.hour < 8) g.pet.d.pad1[1] |= 1; else g.pet.d.pad1[1] &= ~1;   // lights off while they sleep (and at bedtime)
      if (g.callReason() != CR_NONE && style == 99 && t < 13 * 3600) printf("  t=%5.2fh call %d (hunger %.0f fun %.0f health %.0f mess %d/%d sleep %d)\n", t / 3600.0f, g.callReason(), g.pet.d.hunger, g.pet.d.fun, g.pet.d.health, g.pet.d.droppings, g.pet.d.floorMess, g.pet.sleeping);
      g.pet.d.pad0[0] = 37;   // looks are the player's pick; take the first ones
    }
    printf("checks every %4d min%s: %-12s after %4.1f days | mistakes @12h %3d @18h %3d | chonk %.0f\n",
           every / 60, style >= DAYONLY ? " (8am-10pm)" : "           ", g.sv.hasBlob ? "still alive" : endingName(g.sv.hall[0].ending), t / 86400.0f, m12, m18, g.pet.d.chonk);
    if (style == 1) { printf("  minutes per call reason in the first 12 h:"); for (int i = 1; i < 8; i++) printf(" %d", reasonMin[i]); printf("\n"); }
  }
  return 0;
}

// Settings + confirm flows (checks the screen after each tap).
static int cmdFlows() {
  SimPlat p;
  Game g(&p);
  g.begin();
  tap(g, p, 92, 195); tap(g, p, 92, 100); tap(g, p, 92, 182);   // clock OK, fill the dropper
  int bad = 0;
  auto expect = [&](Screen s, const char* what) { if (g.scr != s) { printf("FAIL %s: screen %d\n", what, (int)g.scr); bad++; } };
  expect(S_MAIN, "dropper");
  g.simulate(600, false); run(g, p, 0.5f);
  if (g.form() != F_DROPPER) { printf("FAIL the dropper didn't wait for a push\n"); bad++; }
  for (int i = 0; i < 45; i++) step(g, p, true, 92, 70);
  step(g, p); expect(S_SETTINGS, "long press");
  const int v0 = g.sv.volume; tap(g, p, 92, SETTINGS_Y0 + 12); if (g.sv.volume == v0) { printf("FAIL sound row\n"); bad++; }
  { // auto off: 30 Sec (the default) -> 1 Min -> Never -> 5 Sec -> 15 Sec -> 30 Sec
    const uint8_t a0 = g.sv.eq[2]; uint8_t seen[3];
    for (int i = 0; i < 3; i++) { tap(g, p, 92, SETTINGS_Y0 + 3 * SETTINGS_STEP + 12); seen[i] = g.sv.eq[2]; }
    tap(g, p, 92, SETTINGS_Y0 + 3 * SETTINGS_STEP + 12); tap(g, p, 92, SETTINGS_Y0 + 3 * SETTINGS_STEP + 12);   // ... -> 15 Sec -> back to 30 Sec
    if (a0 != 3 || seen[0] != 4 || seen[1] != 1 || seen[2] != 0 || g.sv.eq[2] != 3) { printf("FAIL auto off cycle %d -> %d %d %d\n", a0, seen[0], seen[1], seen[2]); bad++; }
  }
  tap(g, p, 92, SETTINGS_Y0 + 4 * SETTINGS_STEP + 12); expect(S_SETTINGS, "more");
  tap(g, p, 92, SETTINGS_Y0 + 0 * SETTINGS_STEP + 12); expect(S_FRIENDS, "old friends row");
  tap(g, p, 92, 209); expect(S_SETTINGS, "old friends back");
  tap(g, p, 92, SETTINGS_Y0 + 1 * SETTINGS_STEP + 12); expect(S_JOURNAL, "journal row");
  tap(g, p, 92, 209); expect(S_SETTINGS, "journal back");
  tap(g, p, 92, SETTINGS_Y0 + 2 * SETTINGS_STEP + 12); expect(S_SETUP_CLOCK, "clock row");
  tap(g, p, 40, 196); expect(S_SETTINGS, "clock cancel");
  tap(g, p, 92, SETTINGS_Y0 + 4 * SETTINGS_STEP + 12); expect(S_SETTINGS, "more (page 3)");
  tap(g, p, 92, SETTINGS_Y0 + 0 * SETTINGS_STEP + 12); expect(S_CONFIRM, "new blob row");
  tap(g, p, 50, 124); expect(S_SETTINGS, "new blob no");
  tap(g, p, 92, SETTINGS_Y0 + 1 * SETTINGS_STEP + 12); expect(S_CONFIRM, "erase everything row");
  tap(g, p, 50, 124); expect(S_SETTINGS, "erase everything no");
  if (!g.sv.hasBlob) { printf("FAIL erase NO still erased\n"); bad++; }
  { const bool t0 = g.showTouch; tap(g, p, 92, SETTINGS_Y0 + 2 * SETTINGS_STEP + 12); if (g.showTouch == t0) { printf("FAIL touch row (page 3)\n"); bad++; } tap(g, p, 92, SETTINGS_Y0 + 2 * SETTINGS_STEP + 12); }
  tap(g, p, 92, SETTINGS_Y0 + 4 * SETTINGS_STEP + 12); expect(S_SETTINGS, "page 3 back");
  tap(g, p, 92, SETTINGS_Y0 + 5 * SETTINGS_STEP + 12); expect(S_SETTINGS, "page 2 back");
  tap(g, p, 92, SETTINGS_Y0 + 5 * SETTINGS_STEP + 12); expect(S_MAIN, "back");
  // the screen: left untouched it goes off (Auto off); a tap or the button turns it on
  {
    g.sv.eq[2] = 0; run(g, p, 6.5f);                                        // Auto off: 5 Sec
    if (!g.displayOn()) { printf("FAIL the Auto off timer ran with the platform saying no\n"); bad++; }
    p.autoOff = true; run(g, p, 0.2f);
    if (g.displayOn()) { printf("FAIL untouched for 6 s: the screen should be off\n"); bad++; }
    tap(g, p, 92, 120); run(g, p, 0.2f);
    if (!g.displayOn()) { printf("FAIL a tap didn't turn the screen on\n"); bad++; }
    if (g.scr != S_MAIN) { printf("FAIL the waking tap also did something (screen %d)\n", (int)g.scr); bad++; }
    p.button = true; run(g, p, 0.3f);
    if (g.displayOn()) { printf("FAIL the button didn't turn the screen off\n"); bad++; }
    p.button = true; run(g, p, 0.3f);
    if (!g.displayOn()) { printf("FAIL the button didn't turn the screen on\n"); bad++; }
    g.sv.eq[2] = 2; run(g, p, 7.0f);                                        // Auto off: 15 Sec
    if (!g.displayOn()) { printf("FAIL Auto off 15 Sec: off after 7 s\n"); bad++; }
    run(g, p, 9.0f);
    if (g.displayOn()) { printf("FAIL Auto off 15 Sec: still on after 16 s\n"); bad++; }
    tap(g, p, 92, 120); g.sv.eq[2] = 1; run(g, p, 200.0f);                   // Never: stays on (dimmed), even past 3 minutes
    if (!g.displayOn()) { printf("FAIL Auto off Never: the screen went off\n"); bad++; }
    g.sv.eq[2] = 3; p.autoOff = false;
  }
  { // squeezed out at 11pm (their bedtime): awake through the landing and the first 2 minutes, then off to sleep
    SimPlat q; Game h(&q); h.begin();
    tap(h, q, 92, 195); tap(h, q, 92, 100); tap(h, q, 92, 182); q.setClock(23, 0); run(h, q, 1.5f);
    tapDot(h, q, 20, 14);
    bool slept = false;
    for (int i = 0; i < (int)((SQUEEZE_SECS + 0.3f) * 30); i++) { step(h, q); if (h.pet.sleeping) slept = true; }
    if (slept || h.scr != S_MAIN) { printf("FAIL a new blob fell asleep during its landing (screen %d)\n", (int)h.scr); bad++; }
    h.simulate(100, false); run(h, q, 0.3f);
    if (h.pet.sleeping) { printf("FAIL a new blob fell asleep within 2 minutes\n"); bad++; }
    h.simulate(NEWBORN_AWAKE - 100 - 60, false); run(h, q, 0.3f);
    if (h.pet.sleeping && h.pet.nap <= 0) { printf("FAIL a new blob at 11pm went to bed within its first 30 minutes (the test nap aside)\n"); bad++; }
    h.simulate(200, false); run(h, q, 0.3f);
    if (!h.pet.sleeping) { printf("FAIL a new blob at 11pm didn't go to sleep after its first 30 minutes\n"); bad++; }
  }
  { // a napping fella holds it until they wake; after a trip to the box they walk back, they don't stay beside it
    SimPlat q; Game h(&q); h.begin();
    tap(h, q, 92, 195); tap(h, q, 92, 100); tap(h, q, 92, 182); q.setClock(12, 0); run(h, q, 1.5f); tapDot(h, q, 20, 14); run(h, q, SQUEEZE_SECS + 0.5f);
    h.endNewborn(); h.simulate(200, false); run(h, q, 0.5f);   // past the first 30 minutes
    PetData& d = h.pet.d; d.pad2 = 0xE0FF; d.nextSick = 1e9f; d.hunger = 90; d.fun = 90; d.droppings = 0; d.floorMess = 0;
    d.digestTimer = 1e6f; run(h, q, 4.0f); h.pet.nap = 600; tapIcon(h, q, IC_LIGHT); h.simulate(5, false); run(h, q, 0.3f);   // a nap (with the light off)
    if (!h.pet.sleeping) { printf("FAIL no nap with the light off (scr %d light %d energy %.0f exploring %d age %.0f)\n", (int)h.scr, (int)h.pet.d.pad1[1] & 1, 0.0f, (int)d.exploring, d.age); bad++; }
    d.digestTimer = 20; h.simulate(300, false);
    if (d.droppings || d.floorMess) { printf("FAIL pooped while napping\n"); bad++; }
    tapIcon(h, q, IC_LIGHT); h.pet.nap = 0; h.pet.wakeGently(); h.simulate(40, false); run(h, q, 0.5f);
    if (!d.droppings) { printf("FAIL no poop after waking\n"); bad++; }
    if (h.anim() != AN_POTTY) { printf("FAIL no trip to the box after waking (anim %d)\n", h.anim()); bad++; }
    d.bubblesOnFloor = 0; d.giftOnFloor = 0;   // (a crowded floor leaves no room to wander)
    if (!h.displayOn()) { tap(h, q, 92, 120); run(h, q, 0.3f); }   // (the screen may have gone off: they only wander while it is on)
    const float before = h.blobX(); run(h, q, 3.0f); const float out = h.blobX(); float lowest = out, highest = out;
    for (int i = 0; i < 25 * 30; i++) { h.idleSec = 0; step(h, q); if (h.blobX() < lowest) lowest = h.blobX(); if (h.blobX() > highest) highest = h.blobX(); }   // (they wander at random: they just must not stay put)
    if (h.pet.sleeping || (lowest >= out && highest <= out)) { printf("FAIL they stayed by the box (x %.0f -> %.0f -> %.0f scr %d an %d fm %d drop %d disp %d form %d tx %.0f)\n", before, out, h.blobX(), (int)h.scr, h.anim(), d.floorMess, d.droppings, (int)h.displayOn(), (int)h.form(), 0.0f); bad++; }
  }
  tapIcon(g, p, IC_CALL); expect(S_MAIN, "call");
  tapIcon(g, p, IC_METER); expect(S_METER, "meter");
  tapIcon(g, p, IC_FEED); expect(S_MAIN, "feed with the dropper -> main");
  tapDot(g, p, 20, 14); expect(S_ANIM, "push the dropper");
  run(g, p, SQUEEZE_SECS + 0.2f); expect(S_MAIN, "the blob is out");
  if (g.form() != F_BABY) { printf("FAIL no Lil Blob after the squeeze\n"); bad++; }
  g.pet.d.hunger = 50; tapIcon(g, p, IC_FEED); expect(S_ANIM, "feed: a meal straight away (no menu)");
  if (g.anim() != AN_EAT_MEAL) { printf("FAIL feed isn't a meal\n"); bad++; }
  run(g, p, g.eatSecs() + 0.5f); expect(S_MAIN, "the meal eaten");
  tapIcon(g, p, IC_LIGHT); if (!(g.pet.d.pad1[1] & 1) || g.scr != S_MAIN) { printf("FAIL light off\n"); bad++; }
  tapIcon(g, p, IC_LIGHT); if (g.pet.d.pad1[1] & 1) { printf("FAIL light on\n"); bad++; }
  // litter: menu, scoop, shop
  g.pet.d.droppings = 3; g.pet.d.floorMess = 1; g.pet.sleeping = false;
  tapIcon(g, p, IC_LITTER); expect(S_SCRUB, "clean: the scrub opens");
  { const float need = g.sc_.need, secs = scrubSolve(g, p); if (secs < need / 130.0f || secs > need / 80.0f) { printf("FAIL scrubbing took %.1f s for %.0f dots of rubbing\n", secs, need); bad++; } }
  run(g, p, 2.5f); expect(S_MAIN, "scrubbed clean -> home");
  if (g.pet.d.droppings || g.pet.d.floorMess) { printf("FAIL scrub left mess\n"); bad++; }
  g.pet.d.droppings = 2; tapIcon(g, p, IC_LITTER); expect(S_SCRUB, "scrub again");
  tapIcon(g, p, IC_LITTER); expect(S_MAIN, "the soap icon again leaves");
  if (g.pet.d.droppings != 2) { printf("FAIL leaving the scrub early cleaned\n"); bad++; }
  g.pet.d.droppings = 0;
  // POOP UP is in the Play menu now
  tapIcon(g, p, IC_PLAY); expect(S_MENU, "play menu");
  tapDot(g, p, 30, 20); expect(S_POP, "POP from the Play menu");
  popPlay(g, p);
  run(g, p, 2.6f); expect(S_POP, "POP: the score");
  if (g.pop_.phase != 4) { printf("FAIL POP: no result card (phase %d)\n", g.pop_.phase); bad++; }
  tapDot(g, p, 20, 14); expect(S_MAIN, "POP: tap the score away");
  tapIcon(g, p, IC_LITTER); expect(S_MAIN, "nothing to scoop");
  // a poop while you're in a menu still plays the litter box animation when you come back
  g.pet.d.digestTimer = 2.5f; g.pet.d.resent = 0;
  tapIcon(g, p, IC_METER); g.simulate(3, false); run(g, p, 0.3f); expect(S_METER, "meter");
  tapIcon(g, p, IC_PLAY); run(g, p, 0.3f); expect(S_MENU, "play menu"); tapIcon(g, p, IC_PLAY); run(g, p, 0.2f);
  expect(S_ANIM, "potty shown after menu");
  run(g, p, 3.5f);
  if (!g.nameBuf()[0]) { printf("FAIL no starting name\n"); bad++; }
  { // every start gets a random name: 12 fresh devices shouldn't all agree
    int same = 0;
    for (int i = 0; i < 12; i++) { SimPlat q; q.seed = 1000u + i * 7919u; Game h(&q); h.begin(); if (!strcmp(h.nameBuf(), g.nameBuf())) same++; }
    if (same >= 12) { printf("FAIL the starting name isn't random (%s every time)\n", g.nameBuf()); bad++; }
  }
  // persistence: save, reload into a fresh game
  g.saveNow();
  Game g2(&p); g2.begin();
  if (!g2.sv.hasBlob || strcmp(g2.pet.d.name, g.pet.d.name)){ printf("FAIL reload\n"); bad++; }
  // a save made in the old HARD mode opens in the one remaining mode, straight to the fella
  g.sv.mode = (Mode)1; g.saveNow();   // 1 = the retired hard mode
  Game g3(&p); g3.begin();
  if (g3.sv.mode != MODE_EASY || g3.scr != S_MAIN) { printf("FAIL old hard save: mode %d screen %d\n", g3.sv.mode, (int)g3.scr); bad++; }
  printf("flows: %s\n", bad ? "FAILED" : "all ok");
  return bad ? 1 : 0;
}

// Random taps, holds and drags all over the screen, with random time jumps. Checks invariants every frame.
static int cmdFuzz(int seeds) {
  int bad = 0;
  for (int seed = 1; seed <= seeds; seed++) {
    SimPlat p;
    Game g(&p);
    g.begin();
    uint32_t r = (uint32_t)seed * 2654435761u;
    auto rnd = [&](int n) { r ^= r << 13; r ^= r >> 17; r ^= r << 5; return (int)(r % (uint32_t)n); };
    for (int it = 0; it < 6000; it++) {
      const int k = rnd(100);
      const int x = rnd(W), y = rnd(H);
      if (k < 55) tap(g, p, x, y);
      else if (k < 60) { for (int i = 0; i < 45; i++) step(g, p, true, x, y); step(g, p); }
      else if (k < 65) { int x2 = rnd(W), y2 = rnd(H); for (int i = 0; i <= 10; i++) step(g, p, true, x + (x2 - x) * i / 10, y + (y2 - y) * i / 10); step(g, p); }
      else if (k < 80) run(g, p, 0.5f + rnd(30) / 10.0f);
      else if (k < 88) g.simulate((float)(60 * (1 + rnd(120))), false);
      else if (k < 90) run(g, p, 0.2f);
      else if (k < 92) { p.ms += 1000u * (uint32_t)rnd(400); step(g, p); }   // long pause (screen goes off)
      else run(g, p, 0.1f);
      // invariants
      bool ok = true;
      if (g.scr >= S_COUNT) { printf("seed %d: bad screen %d\n", seed, g.scr); ok = false; }
      for (int i = 0; i < W * H; i++) if (fb.px[i] > 3) { printf("seed %d: bad shade\n", seed); ok = false; break; }
      if (g.pet.sickStack() > SICK_MAX) { printf("seed %d: sick stack %d\n", seed, g.pet.sickStack()); ok = false; }
      if (g.sv.hasBlob && (g.pet.d.hunger < 0 || g.pet.d.hunger > 100 || g.pet.d.chonk > 130)) { printf("seed %d: stats out of range\n", seed); ok = false; }
      if (!ok) { bad++; break; }
    }
    printf("seed %d: screen %d, age %.1f h, hasBlob %d, sick %u, fellas %u\n", seed, g.scr, g.pet.d.age / 3600, g.sv.hasBlob, (unsigned)g.sv.sickCount, g.sv.lifeBlobs);
  }
  printf("fuzz: %s\n", bad ? "FAILED" : "all ok");
  return bad ? 1 : 0;
}

// Random taps, holds and time jumps for a long while (run under ASan/UBSan): looks for crashes and stuck screens.
// A bot that plays Dodge well (it sees every drop) to check that ~27-30 s is the ceiling.
namespace lf {
struct DodgeBot {
  static int run(int seed) {
    SimPlat p; p.ms = 1000 + seed * 7919;
    Game g(&p); g.begin();
    tap(g, p, 92, 195); tap(g, p, 92, 100); tap(g, p, 92, 182); tapDot(g, p, 20, 14); run2(g, p, SQUEEZE_SECS + 0.2f); g.simulate(70, false); run2(g, p, 3);
    g.pet.d.hunger = 100; g.pet.rng = 1234567u + seed * 99991u;
    g.debugAct(5); step(g, p);
    const int gs = g.gameSprite();
    const int w = spriteW(gs), cy = MH - 1 - spriteH(gs);
    for (int f = 0; f < 30 * 60 && g.dg_.phase == 1; f++) {
      float bestScore = -1e9; int bestX = (int)g.dg_.x;
      for (int x = 0; x <= MW - w; x++) {
        // roll the future forward 2 s: they run to x at full speed; first time any drop would hit their
        float safe = 2.0f;
        for (float tt = 0; tt < 2.0f; tt += 1.0f / 30) {
          float cxp = g.dg_.x + (x > g.dg_.x ? 1 : -1) * 34.0f * tt;
          if ((x >= g.dg_.x && cxp > x) || (x <= g.dg_.x && cxp < x)) cxp = (float)x;
          const int cxi = (int)cxp;
          bool hit = false;
          for (auto& it : g.dg_.it) if (it.on) { const float y = it.y + it.v * tt; if (y + it.h - 1 >= cy + 2 && y < MH - 1 && it.x + it.w > cxi + 1 && it.x < cxi + w - 1) hit = true; }
          if (hit) { safe = tt; break; }
        }
        const float sc = safe - fabsf(x - (MW - w) / 2.0f) * 0.002f;
        if (sc > bestScore) { bestScore = sc; bestX = x; }
      }
      step(g, p, true, MX + (bestX + w / 2) * DOT + 1, MY + 60);
    }
    return (int)(g.dg_.phase == 1 ? 99 : g.dg_.score * 10);
  }
  static void run2(Game& g, SimPlat& p, float s) { int n = (int)(s * 30); for (int i = 0; i < n; i++) step(g, p); }
};
}  // namespace lf

// First-hour lessons: answer each request with its icon; the Lil Blob only grows up after all 8.
static int cmdLessons() {
  SimPlat p; Game g(&p); g.begin();
  tap(g, p, 92, 195); tap(g, p, 92, 100); tap(g, p, 92, 182); tapDot(g, p, 20, 14); run(g, p, SQUEEZE_SECS + 0.2f);
  int bad = 0, answered = 0;
  for (int min = 0; min < 120 && answered < 8; min++) {
    g.simulate(60, false); g.idleSec = 0; run(g, p, 1.5f);   // the player picks the device up and looks
    if (g.scr == S_ANIM) run(g, p, 3.5f);
    if (min == 95 && g.scr == S_EVOLVE && (g.pet.d.pad2 & 0xBF) != 0xBF) { printf("FAIL picker opened before lessons were done\n"); bad++; }
    if (min == 0) printf("  clock %02d:%02d\n", g.hour, g.minute);
    // medicine and the light are taught by the timed sickness and the sleepy nap
    while (g.pet.sick()) { printf("  min %3d: sick -> medicine\n", min); g.debugAct(-1); tapIcon(g, p, IC_MEDS); run(g, p, 3.0f); }
    if (g.pet.sleeping && !(g.pet.d.pad1[1] & 1)) { printf("  min %3d: sleepy -> light off\n", min); tapIcon(g, p, IC_LIGHT); run(g, p, 0.5f); }
    const int want = g.tutWant();
    if (want < 0) continue;
    if (g.scr != S_MAIN) { g.debugAct(-1); }
    printf("  min %3d: wants icon %d (reason %d)\n", min, want, (int)g.callReason());
    tapIcon(g, p, want); run(g, p, 0.3f);
    if (g.scr == S_MENU) { tapDot(g, p, 10, 4); run(g, p, 0.5f); }
    if (g.scr == S_POP) { popPlay(g, p); run(g, p, 2.6f); }
    if (g.scr == S_SCRUB) { scrubSolve(g, p); run(g, p, 2.5f); }
    if (g.scr == S_DODGE || g.scr == S_METER) { tapIcon(g, p, IC_METER); g.debugAct(-1); }
    run(g, p, 3.5f);
    answered++;
  }
  g.simulate(120, false); run(g, p, 1.0f);
  printf("lessons: answered %d, all used %d, coins %u, screen %d\n", answered, (g.pet.d.pad2 & 0xBF) == 0xBF, (unsigned)g.sv.sickCount, (int)g.scr);
  if ((g.pet.d.pad2 & 0xBF) != 0xBF) { printf("FAIL lessons incomplete\n"); bad++; }
  if (g.pet.d.age > BABY_END && g.scr != S_EVOLVE) { g.simulate(5, false); run(g, p, 0.5f); }
  {   // ignore the lessons: still a Lil Blob at 2 hours, no picker
    SimPlat p2; Game h(&p2); h.begin(); tap(h, p2, 92, 195); tap(h, p2, 92, 100); tap(h, p2, 92, 182); tapDot(h, p2, 20, 14); run(h, p2, SQUEEZE_SECS + 0.2f);
    for (int m = 0; m < 130; m++) { h.simulate(60, false); h.pet.d.hunger = 80; h.pet.d.fun = 80; h.pet.d.health = 90; }
    run(h, p2, 1.0f);
    if (h.scr == S_EVOLVE || h.form() != F_BABY) { printf("FAIL grew up without the lessons (screen %d form %d)\n", (int)h.scr, (int)h.form()); bad++; }
  }
  printf("lessons: %s\n", bad ? "FAILED" : "ok");
  return bad;
}

// Training + sleep visuals, from a clean start.
static int cmdTrain(const char* dir) {
  outDir = dir;
  SimPlat p; Game g(&p); g.begin();
  tap(g, p, 92, 195); tap(g, p, 92, 100); tap(g, p, 92, 182); tapDot(g, p, 20, 14); run(g, p, SQUEEZE_SECS + 0.2f);
  g.pet.d.pad2 = 0xC0FF; g.simulate(70, false); run(g, p, 4);
  g.pet.d.hunger = g.pet.d.fun = g.pet.d.health = 90; g.pet.d.droppings = 0;
  // asleep, lights off
  g.idleSec = 100; run(g, p, 2.0f);
  printf("sleeping %d\n", (int)g.pet.sleeping);
  tapIcon(g, p, IC_LIGHT); run(g, p, 0.5f);
  printf("sleeping %d light off %d screen %d\n", (int)g.pet.sleeping, g.pet.d.pad1[1] & 1, (int)g.scr);
  for (int i = 0; i < 4; i++) { run(g, p, 0.5f); char nm[24]; snprintf(nm, sizeof nm, "t_dark_%d", i); shot(g, nm); }
  return 0;
}


// touch: taps that land a little off, drift as the finger lifts, or are over between two frames
static int cmdTouch() {
  int fails = 0;
  auto check = [&](bool ok, const char* what) { printf("  %-44s %s\n", what, ok ? "ok" : "FAIL"); if (!ok) fails++; };
  SimPlat p; p.ms = 5000;
  Game g(&p); g.begin();
  tap(g, p, 92, 195); tap(g, p, 92, 100); tap(g, p, 92, 182); tapDot(g, p, 20, 14); run(g, p, SQUEEZE_SECS + 0.2f); g.simulate(70, false); run(g, p, 3);
  g.pet.d.hunger = 100; g.pet.d.giftOnFloor = GIFT_FLOWER;
  run(g, p, 1);
  // a gift drawn at dots (0..7, 23..27); tap 4 dots above it
  const int g0 = g.pet.d.giftCount;
  tapDot(g, p, 4, MH - 9); run(g, p, 0.3f);
  check(g.pet.d.giftCount == g0 + 1 && g.pet.d.giftOnFloor == 0, "gift tapped 4 dots off");
  // a quick tap that drifts 30 px while lifting still feeds them
  g.pet.d.hunger = 50;
  step(g, p, true, ICON_X[0] + 8, ICON_TOP_Y + 8); step(g, p, true, ICON_X[0] + 8 + 30, ICON_TOP_Y + 8); step(g, p); step(g, p);
  check(g.scr == S_ANIM && g.anim() == AN_EAT_MEAL, "quick tap with 30 px drift");
  run(g, p, g.eatSecs() + 0.5f);
  g.debugAct(-1); run(g, p, 0.3f);
  run(g, p, 0.5f);
  // POP: a finger down on the LCD takes aim, lifting it fires
  g.debugAct(13); run(g, p, POP_CARD_SECS + 0.2f);
  check(g.scr == S_POP && g.pop_.phase == 1, "POP open, the card gone");
  step(g, p, true, 40, MY + 60); step(g, p, true, 60, MY + 60);
  check(g.pop_.aiming && g.pop_.phase == 1 && g.pop_.ang < 0, "POP: a finger on the LCD aims (left of the middle: to the left)");
  step(g, p, true, 150, MY + 150);
  check(g.pop_.aiming && g.pop_.ang > 0.5f, "POP: the aim follows the finger, even off the LCD");
  step(g, p);
  check(g.pop_.phase == 2, "POP: lifting the finger fires");
  g.debugAct(-1); run(g, p, 0.5f);

  return fails ? 1 : (printf("touch: all ok\n"), 0);
}

// litter: Bubble Pop Rush
// a new fella doesn't sleep in its first 30 minutes (even at night); after that, normal bedtime
static int cmdNewborn() {
  int fails = 0;
  auto check = [&](bool ok, const char* what) { printf("  %-56s %s\n", what, ok ? "ok" : "FAIL"); if (!ok) fails++; };
  SimPlat p; p.ms = 4000; Game g(&p); g.begin();
  tap(g, p, 92, 195); tap(g, p, 92, 100); tap(g, p, 92, 182); p.setClock(23, 0); run(g, p, 1.5f);   // bedtime
  tapDot(g, p, 20, 14); run(g, p, SQUEEZE_SECS + 0.2f);
  bool slept = false;
  for (int m = 0; m < 29; m++) { g.simulate(60, false); run(g, p, 0.1f); if (g.pet.sleeping && g.pet.nap <= 0) slept = true; }
  check(!slept, "no bedtime in the first 29 minutes, at night (naps aside)");
  for (int m = 0; m < 4; m++) { g.simulate(60, false); run(g, p, 0.1f); }
  g.simulate(60, false); run(g, p, 0.5f);
  check(g.pet.sleeping, "asleep once 30 minutes are up (it is still night)");
  return fails ? (printf("newborn: %d FAILED\n", fails), 1) : (printf("newborn: all ok\n"), 0);
}

// the name page: type your own, delete, random, then Let's goo!
static int cmdName() {
  int fails = 0;
  auto check = [&](bool ok, const char* what) { printf("  %-56s %s\n", what, ok ? "ok" : "FAIL"); if (!ok) fails++; };
  SimPlat p; p.ms = 4000;
  Game g(&p); g.begin();
  tap(g, p, 92, 195); run(g, p, 0.3f); tap(g, p, 92, 100); run(g, p, 0.3f);   // the clock's OK
  check(g.scr == S_SETUP_NAME, "name page after the clock");
  const int kw = 22, kh = 20, x0 = (W - (7 * 22 + 6 * 2)) / 2, y0 = 38;
  auto key = [&](int i) { tap(g, p, x0 + (i % 7) * (kw + 2) + kw / 2, y0 + (i / 7) * (kh + 2) + kh / 2); run(g, p, 0.1f); };
  const char first[12] = {0}; (void)first;
  check(g.nameBuf()[0] != 0, "starts with a random name");
  key('Z' - 'A');
  check(!strcmp(g.nameBuf(), "Z"), "the first key replaces the random name");
  key('E' - 'A'); key('D' - 'A');
  check(!strcmp(g.nameBuf(), "Zed"), "capital first letter, then small letters");
  key(27); key('B' - 'A'); key('O' - 'A'); key('B' - 'A');
  check(!strcmp(g.nameBuf(), "Zed Bob"), "a space starts a new capital");
  key(26); key(26); key(26); key(26);
  check(!strcmp(g.nameBuf(), "Zed"), "DEL removes letters");
  for (int i = 0; i < 14; i++) key('A' - 'A' + i);
  check(strlen(g.nameBuf()) == 10, "names stop at 10 letters");
  for (int i = 0; i < 12; i++) key(26);
  check(g.nameBuf()[0] == 0, "DEL can clear it all");
  tap(g, p, 92, 182); run(g, p, 0.3f);
  check(g.scr == S_SETUP_NAME, "Let's goo! does nothing with no name");
  { char seen[8][12]; int n = 0, diff = 0;
    for (int i = 0; i < 8; i++) { tap(g, p, 92, 144); run(g, p, 0.1f); strcpy(seen[n++], g.nameBuf()); if (i && strcmp(seen[i], seen[i - 1])) diff++; }
    check(diff >= 5 && g.nameBuf()[0], "Random name gives a different name each tap"); }
  key('Q' - 'A');
  check(!strcmp(g.nameBuf(), "Q"), "typing after a random name starts fresh");
  key('U' - 'A'); key(27);
  tap(g, p, 92, 182); run(g, p, 0.5f);
  check(g.scr == S_MAIN && !strcmp(g.pet.d.name, "Qu"), "Let's goo! starts the blob with that name (trailing space dropped)");
  return fails ? (printf("name: %d FAILED\n", fails), 1) : (printf("name: all ok\n"), 0);
}

// growth: a new stage reached while asleep waits for them to wake up
static int cmdGrowth() {
  int fails = 0;
  auto check = [&](bool ok, const char* what) { printf("  %-52s %s\n", what, ok ? "ok" : "FAIL"); if (!ok) fails++; };
  const float ages[3] = {BABY_END + 30, CHILD_END + 30, TEEN_END + 30};
  for (int st = 0; st < 3; st++) {
    SimPlat p; p.ms = 4000 + st * 31; Game g(&p); g.begin();
    tap(g, p, 92, 195); tap(g, p, 92, 100); tap(g, p, 92, 182); tapDot(g, p, 20, 14); run(g, p, SQUEEZE_SECS + 0.2f); g.simulate(70, false); g.endNewborn(); run(g, p, 3);
    g.pet.d.pad2 = 0; g.pet.d.pad0[0] = st == 0 ? 0 : st == 1 ? 1 : 0x05;
    g.pet.d.hunger = g.pet.d.fun = 90;
    // asleep with the light off when the stage arrives
    g.pet.d.pad2 |= 0x2000; g.pet.d.nextSick = 1e9f;
    p.setClock(23, 0); run(g, p, 1.5f);   // night: they are asleep (the clock is read once a second)
    g.pet.sleeping = true; g.pet.d.pad1[1] = 1; g.pet.d.age = ages[st];
    bool opened = false;
    for (int i = 0; i < 90; i++) { g.pet.sleeping = true; step(g, p); if (g.scr == S_EVOLVE || g.anim() == AN_LEVELUP) opened = true; }
    char b[64]; snprintf(b, sizeof b, "stage %d: no picker while asleep, light off", st + 1); check(!opened, b);
    // asleep with the light on (a nap)
    p.setClock(13, 0); g.pet.nap = 600; run(g, p, 1.5f);   // daytime nap: worn out
    g.pet.d.pad1[1] = 0; g.pet.sleeping = true; g.pet.nap = 600;
    int asleepFrames = 0;
    for (int i = 0; i < 90; i++) { g.idleSec = 100; step(g, p); if (g.pet.sleeping) asleepFrames++; if ((g.scr == S_EVOLVE || g.anim() == AN_LEVELUP) && g.pet.sleeping) opened = true; }
    if (asleepFrames < 80) printf("    (nap only lasted %d frames)\n", asleepFrames);
    snprintf(b, sizeof b, "stage %d: no picker during a nap", st + 1); check(!opened, b);
    // wakes up: now it opens
    g.pet.sleeping = false; g.idleSec = 0;
    // first the level-up: they pulse small-big three times with sounds, and only then the picker
    const int boing0 = p.soundCount[SND_BOING], coin0 = p.soundCount[SND_COIN], win0 = p.soundCount[SND_WIN];
    int frames = 0, lvl = 0, minLit = 9999, maxLit = 0; bool pickerEarly = false;
    for (int i = 0; i < 150 && g.scr != S_EVOLVE; i++) {
      g.idleSec = 0; step(g, p); frames++;
      if (g.anim() == AN_LEVELUP) { lvl++; if (lvl < 60) { int n = 0; for (int k = 0; k < MW * MH; k++) if (g.dots()[k]) n++; if (n < minLit) minLit = n; if (n > maxLit) maxLit = n; } }
      if (g.scr == S_EVOLVE && lvl < 60) pickerEarly = true;
    }
    snprintf(b, sizeof b, "stage %d: picker opens once awake", st + 1); check(g.scr == S_EVOLVE, b);
    snprintf(b, sizeof b, "stage %d: the level-up plays first (%.1f s)", st + 1, lvl / 30.0f); check(!pickerEarly && lvl / 30.0f > LEVELUP_SECS - 0.2f && lvl / 30.0f < LEVELUP_SECS + 0.2f, b);
    snprintf(b, sizeof b, "stage %d: they swell and shrink (%d to %d dots lit)", st + 1, minLit, maxLit); check(maxLit > minLit + 8, b);
    snprintf(b, sizeof b, "stage %d: level-up sounds (boings, a sparkle, then the fanfare)", st + 1);
    check(p.soundCount[SND_BOING] - boing0 >= 2 && p.soundCount[SND_COIN] - coin0 >= 1 && p.soundCount[SND_WIN] - win0 >= 1, b);
  }
  return fails ? (printf("growth: %d FAILED\n", fails), 1) : (printf("growth: all ok\n"), 0);
}

#include "report.inc"

// The squeeze out of the dropper: when does the first HELLO! ask for attention?   ./sim hello
static int cmdHello() {
  SimPlat p; p.ms = 321; Game g(&p); g.begin();
  tap(g, p, 92, 195); tap(g, p, 92, 100); tap(g, p, 92, 182); tapDot(g, p, 20, 14);
  float t = 0; bool got = false;
  for (int i = 0; i < 30 * 12 && !got; i++) { step(g, p); t += 1 / 30.0f; if (g.callReason() == CR_HELLO) got = true; }
  printf("  HELLO! %.1f s after the push (the drop lands at 5.0 s)\n", t);
  const bool ok = got && t >= 5.8f && t <= 7.1f;
  printf("hello: %s\n", ok ? "ok" : "FAIL");
  return ok ? 0 : 1;
}

// Frames of the absorbing eat animation: ./sim eatshots <dir> [child pick 1-3]
static int cmdEatShots(const char* dir, int child) {
  outDir = dir;
  SimPlat p; p.ms = 99; Game g(&p); g.begin();
  tap(g, p, 92, 195); tap(g, p, 92, 100); tap(g, p, 92, 182); tapDot(g, p, 20, 14); run(g, p, SQUEEZE_SECS + 0.2f); g.simulate(70, false); run(g, p, 3);
  PetData& d = g.pet.d; d.pad2 = 0xE0FF; d.nextSick = 1e9f; d.pad0[0] = (uint8_t)child; d.age = BABY_END + 60; d.hunger = 20; 
  g.scr = S_MAIN; g.pet.sleeping = false; run(g, p, 1);
  g.debugAct(0);
  for (int i = 0; i < 12 * 30; i++) { step(g, p); if (i % 12 == 0) shot(g, "eat"); }
  return 0;
}

// The evolution tree: every path through the picker, tapped for real.   ./sim tree

// RUN oracle: can any plan (a jump at frame t1, maybe a double jump at t2) survive the next 1.4 s with the things already on screen?
static bool runSurvives(Game& g, int t1, int t2, int frames) {
  Game::Run saved = g.rn_;
  bool ok = true;
  for (int f = 0; f < frames && ok; f++) { g.runStep(1.0f / 30, f == t1 || f == t2, true); if (g.rn_.phase != 1) ok = false; }
  g.rn_ = saved;
  return ok;
}
static bool runThreat(Game& g) {   // anything closer than ~75 dots that matters?
  for (int k = 0; k < 6; k++) if (g.rn_.ob[k].on && g.rn_.ob[k].kind != 6 && g.rn_.ob[k].x < 75) return true;
  return false;
}
// policy 0 = jump only when forced (latest), 1 = jump as soon as a plan allows, 2 = random among the allowed. Returns frames survived; viableFails counts frames with NO surviving plan.
static int runKinds[8];
static int runProof(Game& g, int policy, bool boostAlways, int maxFrames, int& viableFails, int& presses, uint32_t seed) {
  const int H = 42;
  viableFails = 0; presses = 0;
  uint32_t rs = seed * 2654435761u + 1;
  Game::Run& r = g.rn_;
  memset(&r, 0, sizeof r); r.next = 40; r.phase = 1;
  int f = 0;
  for (; f < maxFrames && r.phase == 1; f++) {
    if (boostAlways) r.boost = 5;
    bool press = false;
    if (runThreat(g)) {
      bool any = runSurvives(g, -1, -1, H);
      if (!any) for (int a = 0; a < H && !any; a++) { if (runSurvives(g, a, -1, H)) any = true; else for (int b = a + 1; b < H && !any; b++) any = runSurvives(g, a, b, H); }
      if (!any) viableFails++;
      if (!runSurvives(g, -1, -1, H)) {                       // standing still dies: something has to be done
        bool jumpNow = false, dblNow = false;
        if (r.y <= 0.01f) {
          // a jump now works if some follow-up (none or a double) survives
          jumpNow = runSurvives(g, 0, -1, H); for (int b = 1; b < H && !jumpNow; b++) jumpNow = runSurvives(g, 0, b, H);
          bool later = false;                                  // does waiting a frame still leave a plan?
          for (int a = 1; a < 3 && !later; a++) { later = runSurvives(g, a, -1, H); for (int b = a + 1; b < H && !later; b++) later = runSurvives(g, a, b, H); }
          if (jumpNow && (policy == 1 || !later || (policy == 2 && (rs = rs * 1664525u + 1013904223u, (rs >> 16) % 5 == 0)))) press = true;
        } else if (!r.air2) {
          dblNow = runSurvives(g, 0, -1, H);
          bool later = runSurvives(g, 1, -1, H) || runSurvives(g, 2, -1, H);
          if (dblNow && (policy == 1 || !later || (policy == 2 && (rs = rs * 1664525u + 1013904223u, (rs >> 16) % 5 == 0)))) press = true;
        }
      }
    }
    if (press) presses++;
    bool was[6]; for (int k = 0; k < 6; k++) was[k] = r.ob[k].on;
    g.runStep(1.0f / 30, press, false);
    for (int k = 0; k < 6; k++) if (r.ob[k].on && !was[k]) runKinds[r.ob[k].kind]++;
    if (getenv("GAMES_SHOTS") && seed == 107) { static int shots[8]; for (int k = 0; k < 6; k++) if (r.ob[k].on && r.ob[k].kind >= 3 && r.ob[k].x < 22 && r.ob[k].x > 14 && shots[r.ob[k].kind] < 1) { shots[r.ob[k].kind]++; outDir = getenv("GAMES_SHOTS"); char nm[16]; snprintf(nm, sizeof nm, "runk%d", r.ob[k].kind); g.frame(fb, false, 0, 0); shot(g, nm); } }
  }
  return f;
}

// ORBIT CATCH bot: hop to where the next star is, away from the next rock
static void orbitBot(Game& g, SimPlat& p) {
  auto& o = g.orb_;
  int want = o.lane; float best = 9;
  for (int k = 0; k < 14; k++) if (o.it[k].on) { const float d = o.it[k].a - o.th; if (d > 0.12f && d < best && d < 1.1f) { best = d; want = o.it[k].kind == 0 ? o.it[k].lane : 1 - o.it[k].lane; } }
  for (int k = 0; k < 14; k++) if (o.it[k].on && o.it[k].kind == 1) { const float d = o.it[k].a - o.th; if (d > -0.3f && d < 0.3f) want = 1 - o.it[k].lane; }
  static int lastHits = 0; static char snap[1024]; static int pre = 0;
  if (o.hits != lastHits) { if (o.hits > lastHits) printf("   HIT t=%.2f lane %d want %d prev: %s\n", o.t, o.lane, want, snap); lastHits = o.hits; }
  { char* q = snap; q += sprintf(q, "[lane %d th %.2f]", o.lane, o.th); for (int k = 0; k < 14; k++) if (o.it[k].on && o.it[k].a - o.th < 1.3f) q += sprintf(q, " (k%d l%d d%.2f)", o.it[k].kind, o.it[k].lane, o.it[k].a - o.th); (void)pre; }
  if (want != o.lane) { step(g, p, true, 46, 100); step(g, p); } else step(g, p);
}
// RUN and ORBIT CATCH: the new games work for every look
static int cmdNewGames() {
  int fails = 0;
  auto check = [&](bool ok, const char* what) { printf("  %-60s %s\n", what, ok ? "ok" : "FAIL"); if (!ok) fails++; };
  SimPlat p; p.ms = 999; Game g(&p); g.begin();
  tap(g, p, 92, 195); tap(g, p, 92, 100); tap(g, p, 92, 182); tapDot(g, p, 20, 14); run(g, p, SQUEEZE_SECS + 0.2f); g.simulate(70, false); run(g, p, 3);
  PetData& d = g.pet.d;
  const float ages[4] = {BABY_END - 100, CHILD_END - 100, TEEN_END - 100, TEEN_END + 100};
  int bestAll = 0;
  for (int st = 0; st < 4; st++) {
    d.pad0[0] = st == 0 ? 0 : st == 1 ? Game::packLooks(1, 0, 0) : st == 2 ? Game::packLooks(1, 1, 0) : Game::packLooks(1, 1, 1);
    d.age = ages[st]; d.pad2 = 0xE0FF; d.nextSick = 1e9f; d.hunger = d.fun = 90; d.health = 100;
    g.scr = S_MAIN; g.pet.sleeping = false; run(g, p, 0.5f);
    // RUN: hands off, they run into the first thing
    g.debugAct(11); run(g, p, 0.3f);
    check(g.scr == S_RUN, "RUN opens");
    for (int i = 0; i < 30 * 10 && g.rn_.phase < 3; i++) step(g, p);
    check(g.rn_.phase == 3 && g.rn_.score < 40, "RUN: not jumping ends the run");
    // RUN: a bot that jumps just before things reach it
    g.debugAct(-1); run(g, p, 0.3f); g.debugAct(11); run(g, p, 0.3f);
    int jumps = 0, vf = 0;
    for (int pol = 0; pol < 3; pol++) {
      const int fr = runProof(g, pol, false, 30 * 90, vf, jumps, 100 + st * 7 + pol);
      printf("  look %d (stage %d) policy %d: ran %d frames, %d presses, %d impossible frames, %.0f m\n", (int)g.form(), st, pol, fr, jumps, vf, g.rn_.dist / 4);
      check(vf == 0, "RUN: no impossible situations (some plan always survives)");
      check(g.rn_.phase == 1, "RUN: the oracle survives 90 s");
      check(g.rn_.dist / 4 >= 300 && g.rn_.wallDone, "RUN: 300 m reached, and a double jump over a tall wall was made (4 hearts possible)");
      bestAll = bestAll > (int)g.rn_.dist ? bestAll : (int)g.rn_.dist;
    }
    printf("  spawned so far: block %d crate %d mat %d hole %d bird %d pillar %d bolt %d\n", runKinds[0], runKinds[1], runKinds[2], runKinds[3], runKinds[4], runKinds[5], runKinds[6]);
    { const int fr = runProof(g, 2, true, 30 * 60, vf, jumps, 555 + st); (void)fr;
      check(vf == 0 && g.rn_.phase == 1, "RUN: boosted the whole time, still always a way through"); }
    g.debugAct(-1); run(g, p, 0.3f);
    // ORBIT CATCH
    g.debugAct(12); run(g, p, 0.3f);
    check(g.scr == S_ORBIT, "ORBIT opens");
    for (int i = 0; i < 30 * 40 && g.orb_.phase < 3; i++) {
      if (getenv("GAMES_SHOTS") && st == 3 && (i == 200 || i == 260 || i % 9 == 0)) { outDir = getenv("GAMES_SHOTS"); char nm[24]; snprintf(nm, sizeof nm, "orbit_%04d", i); shot(g, nm); }
      orbitBot(g, p);
    }
    printf("  look %d: orbit bot caught %d stars, %d hits\n", (int)g.form(), g.orb_.stars, g.orb_.hits);
    check(g.orb_.phase == 3 && g.orb_.hits == 0 && g.orb_.stars >= 20, "ORBIT: hopping to the stars and away from rocks works");
    if (getenv("GAMES_SHOTS") && st == 3) { outDir = getenv("GAMES_SHOTS"); shot(g, "orbit_result"); }
    g.debugAct(-1); run(g, p, 0.3f); g.debugAct(12); run(g, p, 0.3f);
    run(g, p, 40);   // hands off: it ends by itself
    check(g.orb_.phase == 3, "ORBIT: it ends by itself");
    if (st == 3) {   // twenty different skies: a bot that hops in time never gets hit
      int bad = 0;
      for (int sd = 0; sd < 20; sd++) {
        g.debugAct(-1); run(g, p, 0.2f); g.pet.rng = 7919u * (sd + 3); g.debugAct(12); run(g, p, 0.2f);
        for (int i = 0; i < 30 * 40 && g.orb_.phase < 3; i++) orbitBot(g, p);
        if (g.orb_.hits) bad++;
      }
      check(bad == 0, "ORBIT: 20 skies, no unavoidable rock");
    }
    g.debugAct(-1); run(g, p, 0.3f);
  }
  (void)bestAll;
  return fails ? (printf("games: %d FAILED\n", fails), 1) : (printf("games: all ok\n"), 0);
}

// Big Blorbus: needs an old fella and a Lil Fella that has a Big form; never eats
static int cmdBlorbus() {
  int fails = 0, eligible = 0, others = 0;
  auto check = [&](bool ok, const char* what) { if (!ok) { printf("  FAIL %s\n", what); fails++; } };
  SimPlat p; p.ms = 4242; Game g(&p); g.begin();
  tap(g, p, 92, 195); tap(g, p, 92, 100); tap(g, p, 92, 182); tapDot(g, p, 20, 14); run(g, p, SQUEEZE_SECS + 0.2f); g.simulate(70, false); run(g, p, 3);
  PetData& d = g.pet.d;
  bool seenBig[F_COUNT] = {};
  auto ready = [&](int c, int t, int a) {
    d.pad0[0] = Game::packLooks(c + 1, t + 1, a + 1); d.pad1[1] &= ~(7 << 2); d.pad0[1] = 0;
    d.pad2 = 0xE0FF; d.nextSick = 1e9f; d.age = AGE_BLORBUS + 100; d.hunger = d.fun = 90; d.health = 100;
    g.scr = S_MAIN; g.pet.sleeping = false; d.pad1[1] &= ~1;
  };
  for (int c = 0; c < 3; c++) {
    d.pad0[0] = Game::packLooks(c + 1, 0, 0);
    for (int t = 0; t < g.evoCount(2); t++) {
      d.pad0[0] = Game::packLooks(c + 1, t + 1, 0);
      for (int a = 0; a < g.evoCount(3); a++) {
        ready(c, t, a);
        const Form fin = g.evoOption(3, a).form;
        const Form big = Game::bigOf(fin, c + 1);
        check(g.form() == fin, "before the pick they are still their Lil Fella");
        if (big == F_COUNT) { others++; check(g.pendingEvolution() == 0, "a Lil Fella without a Big form is never offered one"); continue; }
        eligible++;
        check(g.pendingEvolution() == 4, "old enough: Big Blorbus is offered");
        d.pad0[1] = 1; check(g.pendingEvolution() == 0, "not offered while locked"); d.pad0[1] = 0;
        d.age = AGE_BLORBUS - 100; check(g.pendingEvolution() == 0, "not offered before the age");
        d.age = AGE_BLORBUS + 100;
        for (int i = 0; i < 200 && g.scr != S_EVOLVE; i++) { g.idleSec = 0; g.pet.sleeping = false; step(g, p); }
        check(g.scr == S_EVOLVE, "the picker opens");
        run(g, p, 0.6f);
        check(g.evoCount(4) == 2 && g.evoOption(4, 0).form == big && g.evoOption(4, 1).form == F_COUNT, "two choices: the Big form or stay");
        tapDot(g, p, MW / 2, 20); run(g, p, 0.4f); run(g, p, 2.6f); g.scr = S_MAIN;
        check(g.form() == big, "picking it makes them Big");
        check(stageOfForm(g.form()) == 4 && !strcmp(g.lifeStageName(), "BIG BLORBUS"), "stage name");
        check(Game::lookName(big)[0] != 0, "has a name");
        check(g.pendingEvolution() == 0, "no second offer");
        seenBig[big] = true;
        // living as a Big Blorbus: wanders without trouble, never hungry, no feeding
        int idle, alt, sl; g.blobSprite(big, idle, alt, sl);
        const int w = spriteW(idle);
        float lo = 999, hi = -999, hxMax = 0; bool shotA = false;
        d.hunger = 5;
        for (int i = 0; i < 90 * 30; i++) { g.idleSec = 0; g.pet.sleeping = false; step(g, p); lo = lo < g.blobX() ? lo : g.blobX(); hi = hi > g.blobX() ? hi : g.blobX(); hxMax = hxMax > fabsf(g.ballHx_) ? hxMax : fabsf(g.ballHx_); if (g.scr != S_MAIN) { g.scr = S_MAIN; } d.droppings = 0; d.floorMess = 0;
          if (getenv("BLORBUS_SHOTS")) { outDir = getenv("BLORBUS_SHOTS"); if (big == F_BB_BLOBBERT && (g.blobX() < -1 || g.blobX() > MW - w + 1) && !shotA) { shotA = true; shot(g, "blobbert_squish"); } if (big == F_BB_BALLBOI && fabsf(g.ballHx_) > 3.2f && !shotA) { shotA = true; shot(g, "ballboi_slam"); } if (big == F_BB_ALIEN && i == 900 && !shotA) { shotA = true; shot(g, "alien"); } } }
        check(d.hunger >= 99, "never hungry");
        if (getenv("BLORBUS_SHOTS")) { outDir = getenv("BLORBUS_SHOTS"); g.debugAct(6); for (int pg = 0; pg < 4; pg++) { run(g, p, 0.3f); shot(g, "meter"); tapDot(g, p, 20, 14); } g.debugAct(-1); run(g, p, 0.3f); }
        check(lo >= -3 && hi <= MW - w + 3, "stays near the screen");
        if (big == F_BB_BLOBBERT) check(lo < 0 || hi > MW - w, "the too-big blob reaches the sides (and squishes)");
        if (big == F_BB_BALLBOI) check(hxMax > 2.5f, "the hamster slides around inside the ball");
        g.scr = S_MAIN; run(g, p, 0.3f);
        tapIcon(g, p, IC_FEED); run(g, p, 0.3f);
        check(g.scr == S_ORBIT, "the Feed icon is Orbit Catch for a Big Blorbus");
        for (int i = 0; i < 30 * 40 && g.orb_.phase < 3; i++) orbitBot(g, p);
        check(g.orb_.phase == 3 && g.orb_.stars >= 20, "Orbit Catch can be played well");
        check(g.pet.d.hunger >= 99, "Orbit Catch doesn't change hunger");
        g.scr = S_MAIN; run(g, p, 0.3f);
      }
    }
  }
  // turning it down
  { ready(0, 0, 0); for (int i = 0; i < 200 && g.scr != S_EVOLVE; i++) { g.idleSec = 0; step(g, p); }
    run(g, p, 0.6f); tapDot(g, p, MW - 2, 14); run(g, p, 0.1f); tapDot(g, p, MW / 2, 20); run(g, p, 3.0f); g.scr = S_MAIN;
    check(g.evoChoice(4) == 7 && g.form() == g.evoOption(3, 0).form && g.pendingEvolution() == 0, "STAY keeps the Lil Fella and is not offered again"); }
  int distinct = 0; for (int i = 0; i < F_COUNT; i++) distinct += seenBig[i];
  printf("  %d paths can become Big Blorbus, %d cannot, %d Big forms reached\n", eligible, others, distinct);
  check(distinct == 3, "all three Big Blorbus are reachable");
  d.pad0[0] = Game::packLooks(1, 4, 3);   // Blobbert > Slosh > Ballboi: no Big Ballboi on that line
  check(Game::bigOf(F_AD_JELLY, 2) == F_COUNT && Game::bigOf(F_AD_PUFF, 3) == F_COUNT && Game::bigOf(F_AD_PUFF, 1) == F_BB_BLOBBERT, "Big Blobbert is for the Blobbert line only");
  check(g.evoOption(3, 2).form == F_AD_BALLBOI && Game::bigOf(F_AD_BALLBOI, 1) == F_COUNT && Game::bigOf(F_AD_BALLBOI, 3) == F_BB_BALLBOI, "Big Ballboi is for the Cheeks line only");
  return fails ? (printf("blorbus: %d FAILED\n", fails), 1) : (printf("blorbus: all ok\n"), 0);
}

static int cmdTree() {
  int fails = 0;
  auto check = [&](bool ok, const char* what) { if (!ok) { printf("  FAIL %s\n", what); fails++; } };
  SimPlat p; p.ms = 777; Game g(&p); g.begin();
  tap(g, p, 92, 195); tap(g, p, 92, 100); tap(g, p, 92, 182); tapDot(g, p, 20, 14); run(g, p, SQUEEZE_SECS + 0.2f); g.simulate(70, false); run(g, p, 3);
  PetData& d = g.pet.d;
  int paths = 0, shots = 0;
  bool seen[F_COUNT] = {}, shotSeen[F_COUNT] = {};
  for (int c = 0; c < 3; c++) {
    d.pad0[0] = Game::packLooks(c + 1, 0, 0);
    const int nt = g.evoCount(2);
    for (int t = 0; t < nt; t++) {
      d.pad0[0] = Game::packLooks(c + 1, t + 1, 0);
      const int na = g.evoCount(3);
      check(na >= 1 && na <= 5, "a final list holds 1-5 looks");
      for (int a = 0; a < na; a++) {
        d.pad0[0] = 0; d.pad2 = 0xE0FF; d.nextSick = 1e9f; d.age = BABY_END + 30; d.hunger = d.fun = 90; d.health = 100;
        g.scr = S_MAIN; g.pet.sleeping = false; d.pad1[1] &= ~1;
        const float ages[3] = {BABY_END + 30, CHILD_END + 30, TEEN_END + 30};
        const int sel[3] = {c, t, a};
        char nm[96];
        for (int st = 1; st <= 3; st++) {
          d.age = ages[st - 1];
          for (int i = 0; i < 200 && g.scr != S_EVOLVE; i++) { g.idleSec = 0; g.pet.sleeping = false; step(g, p); }
          snprintf(nm, sizeof nm, "path %d/%d/%d: picker opens at stage %d", c, t, a, st); check(g.scr == S_EVOLVE, nm);
          if (g.scr != S_EVOLVE) break;
          run(g, p, 0.6f);
          if (st == 1 && c == 0 && t == 0 && a == 0) { run(g, p, 0.1f); }
          for (int k = 0; k < sel[st - 1]; k++) { tapDot(g, p, MW - 2, 14); run(g, p, 0.1f); }
          tapDot(g, p, MW / 2, 20); run(g, p, 0.4f);
          snprintf(nm, sizeof nm, "path %d/%d/%d: pick at stage %d goes to the grow animation", c, t, a, st); check(g.scr == S_ANIM, nm);
          run(g, p, 2.6f); g.scr = S_MAIN;
        }
        snprintf(nm, sizeof nm, "path %d/%d/%d: picks stored", c, t, a);
        check(d.pad0[0] == Game::packLooks(c + 1, t + 1, a + 1), nm);
        const Form fin = g.form();
        check(fin == g.evoOption(3, a).form, "final form is the one picked");
        check(!strcmp(g.lifeStageName(), "LIL FELLA"), "final form is a Lil Fella");
        check(Game::lookName(fin)[0] != 0, "final form has a name");
        seen[fin] = true;
        if (getenv("TREE_SHOTS") && shots < 40 && !shotSeen[fin]) { shotSeen[fin] = true; outDir = getenv("TREE_SHOTS"); g.scr = S_MAIN; run(g, p, 1.0f); g.debugAct(0); run(g, p, 0.25f); shot(g, "final"); shots++; }
        paths++;
      }
    }
  }
  int distinct = 0; for (int i = 0; i < F_COUNT; i++) distinct += seen[i];
  printf("  %d paths, %d distinct final forms reached\n", paths, distinct);
  check(distinct >= 11, "every final form is reachable");
  // the Blunkus forms: every Blunkus is reachable and a Lil Blunkus
  for (int c = 1; c <= 3; c++) {
    d.pad0[0] = Game::packLooks(c, 0, 0);
    for (int t = 0; t < g.evoCount(2); t++) {
      d.pad0[0] = Game::packLooks(c, t + 1, 0); d.age = CHILD_END + 30;
      check(stageOfForm(g.evoOption(2, t).form) == 2, "a Lil Blunkus option is a Lil Blunkus");
    }
  }
  // saves from before the tree are re-packed, and only once
  d.pad1[1] = 0; d.pad0[0] = 1 | 1 << 2 | 4 << 4;   // old layout: Blobbert, Bean, Sprout (adult)
  g.migrateLooks();
  check((d.pad1[1] & 2) != 0, "migration marks the save");
  check((d.pad0[0] & 3) == 1 && ((d.pad0[0] >> 2) & 7) >= 1 && ((d.pad0[0] >> 2) & 7) <= 4, "migrated Lil Blunkus pick is valid");
  const uint8_t once = d.pad0[0]; g.migrateLooks(); check(d.pad0[0] == once, "migration runs once");
  d.pad1[1] = 0; d.pad0[0] = 3 | 2 << 2 | 7 << 4;   // old: Cheeks, Bun, Loaf
  g.migrateLooks();
  check(g.form() >= F_BABY, "migrated fella has a look");
  return fails ? (printf("tree: %d FAILED\n", fails), 1) : (printf("tree: all ok\n"), 0);
}


// Every way a life can end, how long neglect takes, and the friends log.   ./sim endings
static int cmdEndings() {
  int bad = 0;
  auto fail = [&](const char* what) { printf("FAIL %s\n", what); bad++; };
  struct Case { const char* nm; int want; };
  const Case cases[] = {{"too chonky", END_DISSOLVED}, {"untreated sickness", END_SICK}, {"zero happiness", END_SAD},
                        {"recycled", END_RECYCLED}, {"ran away", END_RAN_AWAY}};
  for (const Case& c : cases) {
    SimPlat p; Game g(&p); g.begin();
    tap(g, p, 92, 195); tap(g, p, 92, 100); tap(g, p, 92, 182); tapDot(g, p, 20, 14); run(g, p, SQUEEZE_SECS + 0.5f);
    PetData& d = g.pet.d;
    d.pad2 = 0xE0FF; d.age = 2 * DAY; d.nextSick = 1e9f; d.pad0[0] = 1 | 1 << 2;   // a Lil Blunkus (looks picked), lessons done, no scheduled sickness
    float awakeH = 0, totalH = 0;
    for (int m = 0; m < 30 * 24 * 6 && g.sv.hasBlob; m++) {   // 10-minute steps, a caring player except for one thing
      if (c.want != END_SAD) d.fun = 80;
      if (c.want == END_SAD) { d.fun = 0; d.resent = 0; }
      d.hunger = 80; d.floorMess = 0; d.droppings = 0;
      if (c.want == END_SICK) { if (m == 0) g.pet.getSick(SICK_LIGHT); }
      else while (g.pet.sick()) g.pet.vetVisit();
      if (c.want == END_DISSOLVED) d.chonk += 1;
      if (c.want == END_RECYCLED) { d.resent = 0; d.chonk = 50; }
      if (c.want == END_RAN_AWAY) { d.resent += 2; d.health = 100; }
      if (c.want != END_RAN_AWAY && c.want != END_SAD) d.resent = 0;
      if (g.pet.sleeping) d.pad1[1] |= 1; else d.pad1[1] &= (uint8_t)~1u;   // lights out while they sleep
      if (!g.pet.sleeping) awakeH += 1 / 6.0f;
      totalH += 1 / 6.0f;
      g.simulate(600, false);
    }
    run(g, p, 0.3f);
    const int got = g.sv.hall[0].ending;
    printf("  %-20s -> %-12s after %5.1f h (%5.1f h awake)\n", c.nm, g.sv.hasBlob ? "still alive" : endingName(got), totalH, awakeH);
    if (g.sv.hasBlob || got != c.want) fail(c.nm);
    if (g.scr != S_ENDING) fail("ending screen");
    if (c.want == END_SAD && (awakeH < 11.5f || awakeH > 13.5f)) fail("sadness should take ~12 awake hours");
    if (c.want == END_SICK && (awakeH < 28 || awakeH > 32)) fail("an untreated sickness should take ~30 awake hours");
    if (!(g.sv.hall[0].flags & HE_HOURS) || g.sv.hall[0].look == 0) fail("log entry");
    tapDot(g, p, 20, 14); run(g, p, 0.2f);                                   // a tap while they're still melting does nothing
    for (int k = 0; k < 45; k++) step(g, p, true, MX + 80, MY + 50);         // a long press during an ending: Settings
    step(g, p);
    if (g.scr != S_SETTINGS) fail("Settings during an ending");
    tap(g, p, 92, SETTINGS_Y0 + 5 * SETTINGS_STEP + 12); run(g, p, 0.2f);    // Back: the ending carries on where it was
    if (g.scr != S_ENDING) fail("Back from Settings returns to the ending");
    run(g, p, ENDING_WAIT_AT + 30); tapDot(g, p, 20, 14); run(g, p, 0.2f);   // the dropper waits as long as it takes (that tap was the first squeeze)
    if (g.scr != S_ENDING) fail("one squeeze left the ending");
    for (int i = 0; i < ENDING_PULLS - 2; i++) { for (int k = 0; k < 4; k++) step(g, p, true, MX + 80, MY + 50); step(g, p); run(g, p, 0.8f); }
    run(g, p, 10);
    if (g.scr != S_ENDING) fail("two squeezes aren't enough");
    for (int k = 0; k < 4; k++) step(g, p, true, MX + 80, MY + 50); step(g, p); run(g, p, 1.0f);   // the third
    // a finger back on the glass while the dropper rises: the name page waits for it to lift, and that press types nothing
    for (int k = 0; k < 30 * 4; k++) step(g, p, true, MX + 80, MY + 50);
    if (g.scr != S_ENDING) fail("the name page opened under a held finger");
    step(g, p);
    if (g.scr != S_SETUP_NAME) fail("name page after the third squeeze");
    char nm[12]; strcpy(nm, g.nameBuf()); run(g, p, 0.3f);
    if (!nm[0] || strcmp(nm, g.nameBuf())) fail("a press from the ending carried over to the name page");
    // every kind of press on the name page works after an ending: Random name, a key, Let's goo!
    tap(g, p, 92, 144); run(g, p, 0.1f);
    if (!strcmp(nm, g.nameBuf())) fail("Random name after an ending");
    { const int kx0 = (W - (7 * 22 + 6 * 2)) / 2, i = 'Z' - 'A'; tap(g, p, kx0 + (i % 7) * 24 + 11, 38 + (i / 7) * 22 + 10); run(g, p, 0.1f); }
    if (strcmp(g.nameBuf(), "Z")) fail("a key after an ending");
    tap(g, p, 92, 182); run(g, p, 0.5f);
    if (g.scr != S_MAIN || g.form() != F_DROPPER || strcmp(g.pet.d.name, "Z")) fail("Let's goo! after an ending: the opening scene");
    tapDot(g, p, 20, 14); run(g, p, 0.3f);                                    // one tap squeezes the dropper
    if (g.form() != F_BABY || g.anim() != AN_SQUEEZE) fail("one tap starts the new blob");
    for (int k = 0; k < 45; k++) step(g, p, true, MX + 80, MY + 50);          // a long press while the drop falls: Settings
    step(g, p);
    if (g.scr != S_SETTINGS) fail("Settings during the fall");
    tap(g, p, 92, SETTINGS_Y0 + 5 * SETTINGS_STEP + 12); run(g, p, 0.2f);     // Back: the fall carries on
    if (g.scr != S_ANIM || g.anim() != AN_SQUEEZE) fail("Back from Settings returns to the fall");
  }
  { // medicine stops a sickness; away never ends a life
    SimPlat p; Game g(&p); g.begin();
    tap(g, p, 92, 195); tap(g, p, 92, 100); tap(g, p, 92, 182); tapDot(g, p, 20, 14); run(g, p, SQUEEZE_SECS + 0.5f);
    PetData& d = g.pet.d; d.pad2 = 0xE0FF; d.age = 2 * DAY; d.nextSick = 1e9f; d.sick[3] |= SF_BUG_DONE; d.pad0[0] = 1 | 1 << 2;
    g.pet.getSick(SICK_LIGHT); g.pet.getSick(SICK_HUNGER);
    for (int h = 0; h < 30 && g.sv.hasBlob; h++) { d.hunger = d.fun = 80; d.droppings = d.floorMess = 0; if (h == 3 || h == 4) g.pet.vetVisit(); g.pet.d.pad1[1] |= g.pet.sleeping ? 1 : 0; g.simulate(3600, false); }
    if (!g.sv.hasBlob || g.pet.sick() || d.health < 60) fail("medicine should cure (one dose each)");
    d.fun = 0; d.hunger = 0; g.simulate(3 * DAY, true);
    if (!g.sv.hasBlob) fail("away (offline) ended a life");
    g.scr = S_MAIN; d.hunger = d.fun = 80; d.health = 100; d.resent = 0; d.sadness = 0;
    // the log: Settings -> New blob is remembered too, newest first, at most HALL_MAX
    for (int i = 0; i < 10; i++) {
      g.debugAct(7); run(g, p, 0.2f);   // Settings page 1
      tap(g, p, 92, SETTINGS_Y0 + 4 * SETTINGS_STEP + 12); tap(g, p, 92, SETTINGS_Y0 + 4 * SETTINGS_STEP + 12);   // More, More
      tap(g, p, 92, SETTINGS_Y0 + 0 * SETTINGS_STEP + 12);   // page 3: New blob
      tap(g, p, 130, 124);   // YES
      if (g.scr != S_SETUP_NAME) { fail("new blob from settings"); break; }
      tap(g, p, 92, 182); tapDot(g, p, 20, 14); run(g, p, SQUEEZE_SECS + 0.5f);
      for (int h = 0; h <= i; h++) { d.hunger = d.fun = 80; d.health = 100; d.droppings = d.floorMess = 0; g.simulate(3600, false); }
    }
    if (g.sv.hallCount != HALL_MAX || g.sv.hall[0].ending != END_RESET || g.sv.hall[0].age != 9 || g.sv.hall[7].age != 2) fail("friends log after 10 new blobs");
    printf("  log: %d entries, newest %s (%s, %d h)\n", g.sv.hallCount, g.sv.hall[0].name, endingName(g.sv.hall[0].ending), g.sv.hall[0].age);
    // Settings → Erase everything: like a brand new device
    g.sv.yeetBest = 42; g.timeScale = 60;
    g.debugAct(7); run(g, p, 0.2f);
    tap(g, p, 92, SETTINGS_Y0 + 4 * SETTINGS_STEP + 12); tap(g, p, 92, SETTINGS_Y0 + 4 * SETTINGS_STEP + 12);
    tap(g, p, 92, SETTINGS_Y0 + 1 * SETTINGS_STEP + 12);
    if (g.scr != S_CONFIRM) fail("erase row");
    tap(g, p, 130, 124);   // YES
    if (g.scr != S_SETUP_CLOCK || g.sv.hasBlob || g.sv.hallCount || g.sv.yeetBest || g.sv.lifeBlobs || g.timeScale != 1) fail("erase everything");
    SimPlat p2; p2.store = p.store; Game g2(&p2); g2.begin();   // what a restart finds
    if (g2.scr != S_SETUP_CLOCK || g2.sv.hasBlob || g2.sv.hallCount) fail("erase survives a restart");
    tap(g, p, 92, 195); run(g, p, 0.2f); tap(g, p, 92, 100); run(g, p, 0.2f);   // clock OK, then the title
    if (g.scr != S_SETUP_NAME) fail("after erase: clock OK -> name page");
    tap(g, p, 92, 182); tapDot(g, p, 20, 14); run(g, p, SQUEEZE_SECS + 0.5f);
    if (!g.sv.hasBlob || g.form() != F_BABY) fail("a new blob after erasing");
    g.saveNow(); { SimPlat p3; p3.store = p.store; Game g3(&p3); g3.begin();   // a restart keeps that new blob
      if (!g3.sv.hasBlob || g3.scr != S_MAIN || strcmp(g3.pet.d.name, g.pet.d.name)) fail("restart after erase + new blob"); }
  }
  printf("endings: %s\n", bad ? "FAILED" : "all ok");
  return bad ? 1 : 0;
}


// Every look eats with its own routine (eatfx), meal and snack, start to finish; Big Blorbus earns a snack
// from a good Orbit Catch round.   ./sim eat
static int cmdEat() {
  int fails = 0;
  auto check = [&](bool ok, const char* what) { if (!ok) { printf("  FAIL %s\n", what); fails++; } };
  SimPlat p; p.ms = 777; Game g(&p); g.begin();
  tap(g, p, 92, 195); tap(g, p, 92, 100); tap(g, p, 92, 182); tapDot(g, p, 20, 14); run(g, p, SQUEEZE_SECS + 0.2f); g.simulate(70, false); g.endNewborn(); run(g, p, 3);
  PetData& d = g.pet.d;
  bool seen[F_COUNT] = {};
  int marshyWalls[2] = {0, 0};
  auto calm = [&]() { d.pad2 = 0xE0FF; d.nextSick = 1e9f; d.hunger = 30; d.fun = 90; d.health = 100; d.droppings = 0; d.floorMess = 0; d.chonk = 30; d.sadness = 0; d.resent = 0;   // 100+ meals in a row: keep them from bursting
                      g.scr = S_MAIN; g.pet.sleeping = false; d.pad1[1] &= ~1; d.digestTimer = 1e6f; };
  auto eat = [&](int which) {
    const Form f = g.form();
    char b[96];
    calm(); step(g, p); if (g.scr != S_MAIN) { run(g, p, 2.4f); calm(); step(g, p); }   // a new look first plays its grow-up flash
    calm(); g.debugAct(which);
    const float len = g.eatSecs();
    const int r = g.eatRoutine();
    snprintf(b, sizeof b, "%s: has its own routine", Game::lookName(f)); check(f == F_BABY ? r == 0 : r != 0, b);
    if (f == F_AD_MARSHY) marshyWalls[g.eatVar_ & 1]++;
    int frames = 0, lit = 0, empty = 0;
    while (g.scr == S_ANIM && frames < 30 * 20) {
      step(g, p); frames++;
      int n = 0; for (int i = 0; i < MW * MH; i++) if (g.dots()[i]) n++;
      if (n) lit++; else empty++;
    }
    snprintf(b, sizeof b, "%s %s: ends back on the main screen", Game::lookName(f), which ? "snack" : "meal"); check(g.scr == S_MAIN, b);
    snprintf(b, sizeof b, "%s %s: runs its full length (%.1f s, took %.1f s)", Game::lookName(f), which ? "snack" : "meal", len, frames / 30.0f);
    check(frames / 30.0f > len - 0.2f && frames / 30.0f < len + 0.4f, b);
    snprintf(b, sizeof b, "%s %s: something on screen every frame (%d blank)", Game::lookName(f), which ? "snack" : "meal", empty); check(empty == 0, b);
    seen[f] = true;
  };
  for (int c = 0; c < 3; c++) {
    d.pad0[0] = Game::packLooks(c + 1, 0, 0); d.age = BABY_END + 60; eat(0); eat(1);
    for (int t = 0; t < g.evoCount(2); t++) {
      d.pad0[0] = Game::packLooks(c + 1, t + 1, 0); d.age = CHILD_END + 60; eat(0); eat(1);
      for (int a = 0; a < g.evoCount(3); a++) {
        d.pad0[0] = Game::packLooks(c + 1, t + 1, a + 1); d.pad1[1] &= ~(7 << 2); d.age = TEEN_END + 60; eat(0); eat(1);
        const Form big = Game::bigOf(g.form(), c + 1);
        if (big != F_COUNT) { d.age = AGE_BLORBUS + 100; g.evoSetChoice(4, 1); if (g.form() == big) { eat(0); eat(1); } d.pad1[1] &= ~(7 << 2); }
      }
    }
  }
  d.pad0[0] = 0; d.age = DROP_AGE + 10; eat(0);   // Lil Blob: still the absorb
  for (int f = F_BABY; f < F_COUNT; f++) { char b[64]; snprintf(b, sizeof b, "%s was fed", Game::lookName(f)); check(seen[f], b); }
  check(marshyWalls[0] > 0 && marshyWalls[1] > 0, "Marshy: both walls get picked");
  // Big Blorbus: Feed is Orbit Catch; a good round (3+ hearts) ends in a snack, a poor one doesn't
  for (int good = 0; good < 2; good++) {
    d.pad0[0] = Game::packLooks(1, 1, 1); d.age = AGE_BLORBUS + 100; d.pad1[1] &= ~(7 << 2);
    for (int c = 0; c < 3; c++) for (int t = 0; t < 4; t++) for (int a = 0; a < 4; a++) {
      d.pad0[0] = Game::packLooks(c + 1, t + 1, a + 1);
      if (t < g.evoCount(2) && a < g.evoCount(3) && Game::bigOf(g.lilFellaForm(), c + 1) != F_COUNT) goto found;
    }
    found:
    d.age = AGE_BLORBUS + 100; g.evoSetChoice(4, 1); calm(); step(g, p); if (g.scr != S_MAIN) run(g, p, 2.4f); calm();
    check(stageOfForm(g.form()) == 4, "orbit test: a Big Blorbus");
    memset(&g.orb_, 0, sizeof g.orb_); g.scr = S_ORBIT; run(g, p, 0.2f);
    g.orb_.phase = 3; g.orb_.t = 0; g.orb_.stars = good ? 20 : 6; run(g, p, 1.0f);
    check(g.orb_.t > 0.6f, "the Orbit Catch result screen's clock runs (so a tap can leave it)");
    tapDot(g, p, 20, 14); run(g, p, 0.1f);
    check(good ? g.scr == S_ANIM : g.scr == S_MAIN, good ? "a good Orbit Catch round: a snack" : "a poor round: straight back home");
    if (good) { run(g, p, g.eatSecs() + 0.5f); check(g.scr == S_MAIN, "after the snack: home"); }
  }
  return fails ? (printf("eat: %d FAILED\n", fails), 1) : (printf("eat: all ok (every look, meal and snack)\n"), 0);
}

// How much work a frame is: ./sim bench  (frames per second of pure game + drawing, per screen)
static int cmdBench() {
  SimPlat p; p.ms = 777; Game g(&p); g.begin();
  tap(g, p, 92, 195); tap(g, p, 92, 100); tap(g, p, 92, 182); tapDot(g, p, 20, 14); run(g, p, SQUEEZE_SECS + 0.2f); g.simulate(70, false); g.endNewborn(); run(g, p, 3);
  PetData& d = g.pet.d;
  d.pad2 = 0xE0FF; d.nextSick = 1e9f; d.pad0[0] = Game::packLooks(1, 1, 1); d.age = TEEN_END + 60; run(g, p, 3);
  auto timeIt = [&](const char* name, int frames, bool keep(Game&)) {
    const clock_t c0 = clock(); int n = 0;
    for (int i = 0; i < frames; i++) { d.hunger = d.fun = 90; d.health = 100; d.chonk = 30; if (!keep(g)) break; step(g, p); n++; }
    const double us = (double)(clock() - c0) / CLOCKS_PER_SEC * 1e6 / (n ? n : 1);
    printf("  %-22s %7.1f us per frame (%d frames)\n", name, us, n);
  };
  g.scr = S_MAIN; timeIt("home screen", 60000, [](Game& gg) { gg.scr = S_MAIN; gg.idleSec = 0; return true; });
  g.debugAct(0); timeIt("eating routine", 200, [](Game& gg) { return gg.scr == S_ANIM; });
  g.scr = S_MAIN; g.debugAct(2); timeIt("clean game", 600, [](Game& gg) { return gg.scr == S_SCRUB; });
  g.scr = S_MAIN; g.debugAct(5); timeIt("dodge", 300, [](Game& gg) { return gg.scr == S_DODGE; });
  return 0;
}

// The Journal: every look raised is marked, by stage; Lil Blob from the start; old saves are filled in.   ./sim journal
namespace lf { struct JournalTest { static int all(); }; }
int lf::JournalTest::all() {
  int fails = 0;
  auto check = [&](bool ok, const char* what) { printf("  %-62s %s\n", what, ok ? "ok" : "FAIL"); if (!ok) fails++; };
  SimPlat p; p.ms = 4242; Game g(&p); g.begin();
  int tot = 0;
  check(g.journalCount(-1, &tot) == 1 && tot == 26 && g.seenLook(F_BABY), "a brand new device: Lil Blob only, 26 looks in all");
  check(g.journalCount(0, &tot) == 1 && tot == 1 && g.journalCount(1, &tot) == 0 && tot == 3, "by stage: Lil Blob 1/1, Lil Blub 0/3");
  { int all = 0, sum = 0; g.journalCount(-1, &all); for (int s = 0; s < 5; s++) { int t = 0; g.journalCount(s, &t); sum += t; } check(sum == all, "the stages add up to the whole"); }
  tap(g, p, 92, 195); tap(g, p, 92, 100); tap(g, p, 92, 182); tapDot(g, p, 20, 14); run(g, p, SQUEEZE_SECS + 0.2f); g.simulate(70, false); g.endNewborn(); run(g, p, 3);
  PetData& d = g.pet.d;
  d.pad2 = 0xE0FF; d.nextSick = 1e9f;
  // grow up one path: each look is marked when they become it
  d.pad0[0] = Game::packLooks(2, 0, 0); d.age = BABY_END + 60; run(g, p, 2.6f); g.scr = S_MAIN;
  check(g.seenLook(F_S0B) && !g.seenLook(F_S0) && !g.seenLook(F_S0C), "becoming Mochi marks Mochi (and only Mochi)");
  d.pad0[0] = Game::packLooks(2, 3, 0); d.age = CHILD_END + 60; run(g, p, 2.6f); g.scr = S_MAIN;
  check(g.seenLook(F_TN_TOASTY), "then Toasty");
  d.pad0[0] = Game::packLooks(2, 3, 1); d.age = TEEN_END + 60; run(g, p, 2.6f); g.scr = S_MAIN;
  check(g.seenLook(F_AD_LOAF) && g.journalCount(-1, nullptr) == 4, "then Loaf: 4 of 26");
  d.age = AGE_BLORBUS + 100; g.evoSetChoice(4, 1); run(g, p, 2.6f); g.scr = S_MAIN;
  check(g.seenLook(F_BB_ALIEN) && g.journalCount(4, nullptr) == 1, "and Big Blorbus: the Alien Cat");
  // it outlives the fella: a new blob keeps it
  const uint32_t j = g.sv.journal;
  g.logFriend(END_RESET); g.sv.hasBlob = 0; g.newPet(); run(g, p, 0.5f);
  check(g.sv.journal == j, "a new blob keeps the Journal");
  // the page: opens, a tap names a look, Back
  g.scr = S_SETTINGS; g.setPage_ = 1; run(g, p, 0.2f);
  tap(g, p, 92, SETTINGS_Y0 + 1 * SETTINGS_STEP + 12); run(g, p, 0.1f);
  check(g.scr == S_JOURNAL, "Settings page 2: Journal opens it");
  tap(g, p, 92, 18 + 10 + 9); run(g, p, 0.1f);   // the Lil Blob cell
  check(g.journalSel_ == F_BABY, "tapping a look picks it (its name shows)");
  tap(g, p, 92, 209); run(g, p, 0.1f);
  check(g.scr == S_SETTINGS, "Back");
  // saved and loaded
  g.autosave(true);
  { SimPlat p2 = p; Game h(&p2); h.begin(); check(h.sv.journal == j, "it's in the save"); }
  // an old save without a Journal: filled in from the fella's path and the old friends
  g.sv.journal = 0; d.pad0[0] = Game::packLooks(1, 2, 1); d.age = TEEN_END + 60; d.pad1[1] &= ~(7 << 2);
  g.sv.hall[0].look = F_AD_NIMBUS + 1; if (g.sv.hallCount < 1) g.sv.hallCount = 1;
  g.autosave(true);
  { SimPlat p2 = p; Game h(&p2); h.begin();
    check(h.seenLook(F_S0) && h.seenLook(F_TN_BUN) && h.seenLook(F_AD_PUDDLE) && h.seenLook(F_AD_NIMBUS) && h.seenLook(F_BABY), "an old save: the current path and the old friends are filled in");
    check(!h.seenLook(F_S0C) && !h.seenLook(F_BB_BLOBBERT), "an old save: nothing else"); }
  // Erase everything wipes it (back to Lil Blob only)
  g.eraseAll();
  check(g.journalCount(-1, nullptr) == 1, "Erase everything: back to Lil Blob only");
  return fails ? (printf("journal: %d FAILED\n", fails), 1) : (printf("journal: all ok\n"), 0);
}


// ---------------------------------------------------------------------------------------------
// v0.07: sickness, the 30 s game timeout, the scrub, YEET
// ---------------------------------------------------------------------------------------------
namespace lf {
static constexpr int FLOOR_ROW = MH - 1;
struct NewTests {
  int fails = 0;
  void check(bool ok, const char* what) { printf("  %-62s %s\n", what, ok ? "ok" : "FAIL"); if (!ok) fails++; }
  static void hatch(Game& g, SimPlat& p) { tap(g, p, 92, 195); tap(g, p, 92, 100); tap(g, p, 92, 182); tapDot(g, p, 20, 14); run(g, p, SQUEEZE_SECS + 0.2f); }
  static void grown(Game& g, SimPlat& p, float age, uint8_t looks) {
    hatch(g, p); g.simulate(70, false); run(g, p, 3);
    PetData& d = g.pet.d; d.pad2 = 0xE0FF; d.sick[3] |= SF_BUG_DONE; d.age = age; d.pad0[0] = looks;
    d.hunger = d.fun = 80; d.health = 100; g.pet.sleeping = false; p.setClock(12, 0);
    for (int i = 0; i < 300 && (g.scr == S_ANIM || g.scr == S_EVOLVE); i++) { if (g.scr == S_EVOLVE) { tapDot(g, p, MW / 2, 20); } step(g, p); }
    g.scr = S_MAIN; g.lastForm_ = g.form();
  }
  // make them a given look (searching the ages and picks that lead to it)
  static bool setLook(Game& g, Form f) {
    PetData& d = g.pet.d;
    const float ages[5] = {600, BABY_END + 60, CHILD_END + 60, TEEN_END + 60, AGE_BLORBUS + 60};
    for (int st = 0; st < 5; st++) for (int c = 0; c < 4; c++) for (int t = 0; t < 8; t++) for (int a = 0; a < 8; a++) {
      if ((st < 3) != (a == 0) || (st < 2) != (t == 0) || (st < 1) != (c == 0)) continue;   // (every stage so far picked)
      d.age = ages[st]; d.pad0[0] = Game::packLooks(c, t, a); g.evoSetChoice(4, st == 4 ? 1 : 0); d.pad0[1] = 0;
      if (g.form() == f) { g.lastForm_ = f; return true; }
    }
    return false;
  }
  // one YEET from start to stop, pulled back (px, py); returns metres (-1: never stopped)
  static int yeetOnce(Game& g, SimPlat& p, float px, float py, float* secs = nullptr, bool* sane = nullptr) {
    g.startYeet(); g.yt_.phase = 1; g.yeetLaunch(px, py);
    int f = 0;
    for (; f < 30 * 40 && g.yt_.phase == 2; f++) {
      g.yeetStep(1 / 30.0f);
      if (sane && (g.yt_.y > 27.5f || std::isnan(g.yt_.x) || std::isnan(g.yt_.y) || g.yt_.x < 0)) *sane = false;
    }
    if (secs) *secs = f / 30.0f;
    return g.yt_.phase == 3 ? g.yt_.meters : -1;
  }
  void sickness() {
    printf("sickness\n");
    { // the starter bug: once, inside the first 5 minutes; it doesn't count, and leaving it isn't a care mistake
      int early = 0, counted = 0, mistakes = 0, never = 0, twice = 0;
      for (int seed = 0; seed < 20; seed++) {
        SimPlat p; p.ms = 3000 + seed * 7919; Game g(&p); g.begin(); hatch(g, p);
        int firstAt = -1, n = 0, prev = 0;
        for (int s = 0; s < 40 * 60; s++) {
          g.pet.d.hunger = g.pet.d.fun = 80; g.pet.d.droppings = g.pet.d.floorMess = 0;
          g.simulate(1, false);
          const int now = g.pet.d.sick[SICK_BUG];
          if (now > prev) { n++; if (firstAt < 0) firstAt = (int)g.pet.d.age; }
          prev = now;
        }
        if (firstAt < 0) never++; else if (firstAt <= 300) early++;
        if (n > 1) twice++;
        if (g.sv.sickCount) counted++;
        mistakes += g.pet.d.pad1[0];
      }
      char b[120]; snprintf(b, sizeof b, "starter bug inside 5 min in %d/20 lives (never %d, twice %d)", early, never, twice); check(early == 20 && !twice, b);
      check(!counted, "the starter bug isn't counted as a sickness");
      check(!mistakes, "leaving the starter bug for 40 min isn't a care mistake");
    }
    { // well looked after: never sick again (10 days, fed, light off at night)
      SimPlat p; Game g(&p); g.begin(); grown(g, p, CHILD_END + 60, Game::packLooks(1, 1, 0));
      int sick = 0;
      for (int h = 0; h < 240; h++) {
        for (int m = 0; m < 6; m++) { PetData& d = g.pet.d; d.hunger = 80; d.fun = 80; d.droppings = d.floorMess = 0; if (g.pet.sleeping) d.pad1[1] |= 1; else d.pad1[1] &= (uint8_t)~1u; g.simulate(600, false); }
        if (g.pet.sick()) { sick++; while (g.pet.sick()) g.pet.vetVisit(); }
      }
      char b[80]; snprintf(b, sizeof b, "10 days of good care: sick %d times (no random sickness)", sick); check(sick == 0, b);
    }
    { // hunger: hitting 0 makes them sick, once until they're fed again
      SimPlat p; Game g(&p); g.begin(); grown(g, p, CHILD_END + 60, Game::packLooks(1, 1, 0));
      PetData& d = g.pet.d; const int c0 = g.sv.sickCount;
      d.hunger = 2; g.simulate(600, false);
      check(d.sick[SICK_HUNGER] == 1 && g.sv.sickCount == c0 + 1, "hunger hits 0: sick (and it counts)");
      check(d.health <= 100 - SICK_HIT + 1, "a sickness knocks health down");
      d.hunger = 0; g.simulate(3600, false);
      check(d.sick[SICK_HUNGER] == 1, "still at 0: no second sickness");
      d.hunger = 40; g.simulate(5, false); d.hunger = 1; g.simulate(600, false);
      check(d.sick[SICK_HUNGER] == 2, "fed, then hungry to 0 again: it stacks");
      check(g.callReason() == CR_SICK || g.callReason() == CR_HUNGRY, "they call for help");
      // medicine: one dose each
      d.hunger = 80; d.digestTimer = 1e6f; g.scr = S_MAIN; g.pet.sleeping = false; run(g, p, 0.3f);   // (no potty trip between the doses)
      tapIcon(g, p, IC_CALL); run(g, p, 0.2f);
      check(!strcmp(g.msg_, "SICK X2"), "the call says SICK X2");
      tapIcon(g, p, IC_MEDS); run(g, p, 2.5f);
      check(g.pet.sickStack() == 1 && !strcmp(g.msg_, "1 MORE"), "one dose: one sickness left (1 MORE)");
      tapIcon(g, p, IC_MEDS); run(g, p, 2.5f);
      check(!g.pet.sick() && !strcmp(g.msg_, "BETTER"), "second dose: better");
      tapIcon(g, p, IC_MEDS); run(g, p, 0.2f);
      check(g.anim() == AN_REFUSE, "not sick: medicine refused");
      run(g, p, 2);
    }
    // the light: on for more than 15% of the night's sleep -> sick on waking
    const float fracs[4] = {0.0f, 0.10f, 0.20f, 1.0f};
    for (float fr : fracs) {
      SimPlat p; Game g(&p); g.begin(); grown(g, p, CHILD_END + 60, Game::packLooks(1, 1, 0));
      p.setClock(21, 0); g.updateClock(); run(g, p, 0.2f);
      PetData& d = g.pet.d;
      int asleepMin = 0, lightMin = 0; bool wokeSick = false;
      for (int m = 0; m < 16 * 60; m++) {
        d.hunger = d.fun = 80; d.droppings = d.floorMess = 0;
        const bool on = (asleepMin < fr * 600.0f);   // the light left on for the first part of the night
        if (g.pet.sleeping) { asleepMin++; if (on) { d.pad1[1] &= (uint8_t)~1u; lightMin++; } else d.pad1[1] |= 1; }
        g.simulate(60, false);
        if (d.sick[SICK_LIGHT]) { wokeSick = !g.pet.sleeping; break; }
      }
      char b[100]; snprintf(b, sizeof b, "light on %3.0f%% of the night (%d of %d min): %s", 100.0f * lightMin / (asleepMin ? asleepMin : 1), lightMin, asleepMin, d.sick[SICK_LIGHT] ? "sick on waking" : "fine");
      check((fr > 0.15f) == (d.sick[SICK_LIGHT] > 0) && (!d.sick[SICK_LIGHT] || wokeSick), b);
    }
    { // every look shows it: a speckled body and the icon
      int bad = 0;
      for (int f = F_BABY; f < F_COUNT; f++) {
        SimPlat p; Game g(&p); g.begin(); grown(g, p, CHILD_END + 60, Game::packLooks(1, 1, 0));
        if (!setLook(g, (Form)f)) { printf("    look %d: can't be set\n", f); bad++; continue; }
        g.scr = S_MAIN; g.pet.sleeping = false; g.pet.d.exploring = 0; run(g, p, 2.5f); g.scr = S_MAIN;
        auto mids = [&]() { int n = 0; for (int i = 0; i < MW * MH; i++) n += g.dots()[i] == SH_MID; return n; };
        uint8_t before[MW * MH];
        g.m_.clear(); g.drawBlob(4, 2, false, 0, false); memcpy(before, g.dots(), sizeof before);
        g.pet.getSick(SICK_LIGHT);
        g.anim_ = 0.5f;   // (not mid-shiver)
        g.m_.clear(); g.drawBlob(4, 2, false, 0, false);
        int diff = 0; for (int i = 0; i < MW * MH; i++) diff += before[i] != g.dots()[i];
        (void)mids;
        if (diff < 10) { bad++; printf("    look %d: sick look barely differs (%d dots)\n", f, diff); }
        { // and on the real home screen (frames, as played)
          g.pet.d.pad2 = 0xE0FF; g.evoSetChoice(4, stageOfForm((Form)f) == 4 ? 1 : 7);
          memset(g.pet.d.sick, 0, 3); g.scr = S_MAIN; g.an_ = AN_NONE; run(g, p, 3.0f);
          int scrOk = g.scr == S_MAIN && g.an_ == AN_NONE && g.form() == f;
          uint8_t a0[MW * MH]; g.anim_ = 0.6f; step(g, p); memcpy(a0, g.dots(), sizeof a0); const float bx = g.blobX();
          g.pet.getSick(SICK_LIGHT); g.anim_ = 0.6f - 0.033f; g.blobX_ = g.blobTx_ = bx; g.stepT_ = 99; step(g, p);
          int d2 = 0; for (int i = 0; i < MW * MH; i++) d2 += a0[i] != g.dots()[i];
          if (!scrOk || d2 < 10) { bad++; printf("    look %d on the home screen: scr %d anim %d form %d, %d dots differ\n", f, (int)g.scr, (int)g.an_, (int)g.form(), d2); }
        }
      }
      check(!bad, "every look gets the sick speckle");
    }
  }
  void meds() {
    printf("medicine\n");
    SimPlat p; Game g(&p); g.begin(); grown(g, p, CHILD_END + 60, Game::packLooks(1, 1, 0));
    g.pet.getSick(SICK_LIGHT); g.pet.getSick(SICK_HUNGER); g.pet.d.hunger = 80;
    const float h0 = g.pet.d.health;
    tapIcon(g, p, IC_MEDS); step(g, p);
    check(g.anim() == AN_MEDS && g.pet.sickStack() == 1 && g.pet.d.health > h0, "the dose works the moment it's given");
    // try everything to cut it short: every icon, the LCD, a long press, the web page's Home button, a new look
    bool held = true; int k = 0;
    while (g.scr == S_ANIM && g.anT_ < g.anLen_ && k < 200) {
      const int what = k % 12;
      if (what < 8) tapIcon(g, p, what);
      else if (what == 8) tapDot(g, p, 20, 14);
      else if (what == 9) { for (int i = 0; i < 40; i++) step(g, p, true, MX + 80, MY + 50); step(g, p); }
      else if (what == 10) g.debugAct(-1);
      else { g.lastForm_ = F_BABY; step(g, p); }
      if (g.scr == S_ANIM && g.anT_ < g.anLen_ && g.anim() != AN_MEDS) held = false;
      if (g.scr != S_ANIM && g.anT_ < g.anLen_ - 0.05f) held = false;
      k++;
    }
    check(held, "nothing cuts the medicine short (icons, taps, holds, Home, growing)");
    run(g, p, 0.3f);
    check(!strcmp(g.msg_, "1 MORE"), "then it says 1 MORE");
  }
  void yeetClip() {
    printf("yeet: never drawn into the ground\n");
    int bad = 0, frames = 0;
    for (int f = F_BABY; f < F_COUNT; f++) {
      SimPlat p; Game g(&p); g.begin(); grown(g, p, CHILD_END + 60, Game::packLooks(1, 1, 0));
      if (!setLook(g, (Form)f)) continue;
      g.scr = S_MAIN; g.an_ = AN_NONE;
      int worst = -99;
      for (int i = 0; i < 12; i++) {
        g.pet.rng = 777u + i * 31u + f;
        g.startYeet(); g.yt_.phase = 1; run(g, p, 0.1f);
        const float a = (5 + i * 7) * 3.14159f / 180.0f;
        g.yeetLaunch(-cosf(a) * Game::YEET_PULL, sinf(a) * Game::YEET_PULL); g.yt_.gyScr = -1;
        for (int t = 0; t < 30 * 30 && g.yt_.phase < 4; t++) {
          step(g, p); frames++;
          if (g.yt_.gyScr >= 0 && g.yt_.gyScr < MH && g.yt_.drawBot - (g.yt_.gyScr - 1) > worst) { worst = g.yt_.drawBot - (g.yt_.gyScr - 1); if (worst > 0 && getenv("DBGC")) printf("      phase %d rot %d y %.2f camY %.2f ground %d landT %.2f t %.2f\n", g.yt_.phase, g.yt_.rot, g.yt_.y, g.yt_.camY, g.yt_.onGround, g.yt_.landT, g.yt_.t); }
        }
      }
      if (worst > 0) { bad++; printf("    look %2d: drawn %d row(s) into the ground\n", f, worst); }
    }
    char b[80]; snprintf(b, sizeof b, "every look, 12 flights each (%d frames): never below the ground line", frames); check(!bad, b);
  }
  void lock() {
    printf("LOCK\n");
    SimPlat p; Game g(&p); g.begin(); grown(g, p, CHILD_END + 60, Game::packLooks(1, 1, 0));
    PetData& d = g.pet.d; d.digestTimer = 1e6f;
    check(!g.locked(), "a new fella starts unlocked");
    tapIcon(g, p, IC_LOCK); run(g, p, 0.3f);
    check(g.scr == S_LOCK, "the LOCK icon asks first");
    tapDot(g, p, 30, 23); run(g, p, 0.2f);
    check(g.scr == S_MAIN && !g.locked(), "NO: nothing changes");
    tapIcon(g, p, IC_LOCK); run(g, p, 0.3f); tapIcon(g, p, IC_LOCK); run(g, p, 0.2f);
    check(g.scr == S_MAIN && !g.locked(), "the LOCK icon again: backs out");
    tapIcon(g, p, IC_LOCK); run(g, p, 16.0f);
    check(g.scr == S_MAIN && !g.locked(), "left alone 15 s: the question goes away");
    tapIcon(g, p, IC_LOCK); run(g, p, 0.3f); tapDot(g, p, 10, 23); step(g, p);
    check(g.locked() && g.anim() == AN_LOCK, "YES: locked, the padlock snaps shut");
    run(g, p, 4.5f);
    check(g.scr == S_MAIN, "back home after the padlock");
    { SimPlat q; q.store = p.store; Game h(&q); h.begin(); check(h.locked(), "the lock is saved"); }
    // time stands still: no ageing, no growing
    const Form f0 = g.form(); const float age0 = d.age;
    for (int i = 0; i < 40; i++) { d.hunger = d.fun = 90; d.health = 100; g.simulate(3600, false); }
    check(d.age == age0 && g.form() == f0 && g.pendingEvolution() == 0, "40 hours on: not a minute older, same look, no picker");
    g.simulate(40 * 3600.0f, true);
    check(d.age == age0, "away for 40 hours: still no older");
    d.age = AGE_BLORBUS + 100; d.pad0[0] = Game::packLooks(1, 1, 1);
    check(g.pendingEvolution() == 0, "old enough for Big Blorbus but locked: not offered");
    d.age = age0; d.pad0[0] = Game::packLooks(1, 1, 0); g.lastForm_ = g.form();
    // needs carry on
    d.hunger = 60; d.fun = 60; const int sick0 = g.pet.sickStack(); d.digestTimer = 60; d.droppings = 0; d.floorMess = 0;
    for (int i = 0; i < 30; i++) g.simulate(3600, false);
    check(d.hunger < 5 && d.fun < 30, "they still get hungry and bored");
    check(g.pet.sickStack() > sick0, "an empty tummy still makes them sick");
    check(d.droppings + d.floorMess > 0, "they still poop");
    g.pet.sleeping = false; p.setClock(12, 0); run(g, p, 1.0f);
    check(g.callReason() != CR_NONE, "they still call for help");
    // nothing ends them
    bool ended = false;
    for (int i = 0; i < 24 * 20 && !ended; i++) { g.simulate(3600, false); for (int k = 0; k < 5; k++) step(g, p); if (g.scr == S_ENDING || g.pet.ended) ended = true; }
    check(!ended && g.sv.hasBlob, "20 days of total neglect: still here");
    check(d.health >= 1, "health never runs out");
    d.health = 0; d.resent = 100; d.sadness = SAD_LIMIT + 5; d.chonk = CHONK_LIMIT + 5; d.age = AGE_RETIRE + 100;
    g.simulate(60, false); run(g, p, 1.0f);
    check(!g.pet.ended && g.scr != S_ENDING, "every ending at once (and past the last day): none happens");
    // medicine and food still work
    d.age = age0; d.chonk = 40; g.setScreen(S_MAIN); g.pet.sleeping = false; run(g, p, 0.5f);
    { const int st = g.pet.sickStack(); if (st) { tapIcon(g, p, IC_MEDS); step(g, p); check(g.pet.sickStack() == st - 1, "medicine still cures"); run(g, p, 4.0f); } }
    // unlock: life goes on
    g.setScreen(S_MAIN); tapIcon(g, p, IC_LOCK); run(g, p, 0.3f);
    check(g.scr == S_LOCK, "locked: the icon asks about unlocking");
    tapDot(g, p, 10, 23); step(g, p);
    check(!g.locked() && g.anim() == AN_LOCK, "YES: unlocked");
    check(d.health >= 30 && d.resent <= 80 && d.sadness == 0, "unlocking a neglected fella leaves room to put things right");
    run(g, p, 4.5f);
    check(!g.pet.ended && g.scr == S_MAIN, "and they are still here after the padlock opens");
    d.hunger = d.fun = 90; d.health = 100; d.resent = 0; d.sadness = 0;
    const float a1 = d.age; g.simulate(3600, false);
    check(d.age > a1 + 3000, "unlocked: they age again");
    d.age = AGE_BLORBUS + 100; d.pad0[0] = Game::packLooks(1, 1, 1);
    check(g.pendingEvolution() == (Game::bigOf(g.lilFellaForm(), 1) != F_COUNT ? 4 : 0), "unlocked and old enough: Big Blorbus is offered with no training");
    d.health = 0; g.pet.sleeping = false; g.simulate(60, false); run(g, p, 0.5f);
    check(g.pet.ended || g.scr == S_ENDING, "unlocked: endings are back");
    // an old save's training bar is not a lock
    { SimPlat q; Game h(&q); h.begin(); grown(h, q, CHILD_END + 60, Game::packLooks(1, 1, 0));
      h.pet.d.pad0[1] = 1; h.sv.fixes &= ~FIX_LOCK; h.autosave(true);
      SimPlat r; r.store = q.store; Game k(&r); k.begin();
      check(!k.locked(), "a v0.07 save with 1 training point: loads unlocked"); }
    // lessons: 7 icons, LOCK is not one of them
    { SimPlat q; Game h(&q); h.begin(); hatch(h, q); h.pet.d.pad2 = (h.pet.d.pad2 & ~0xFF) | 0xBF;
      check(h.lessonsDone(), "all 7 lessons done without ever touching LOCK"); }
  }
  void sounds() {
    printf("sounds\n");
    // every sound renders: finite, audible, not clipping hard, the length the table says, at both rates
    { int bad = 0;
      for (int sr : {16000, 32000}) for (int s = 0; s < SND_COUNT; s++) {
        SynthVoice v; v.start((Sound)s, sr, 99u + s);
        int n = 0; double pk = 0, e = 0; bool nan = false;
        while (!v.done() && n < sr * 4) { const float x = v.next(); if (!(x == x)) nan = true; if (fabsf(x) > pk) pk = fabsf(x); e += x * x; n++; }
        const double ms = n * 1000.0 / sr, rms = sqrt(e / (n ? n : 1));
        if (nan || pk < 0.15 || pk > 1.6 || rms < 0.03 || fabs(ms - synthMillis((Sound)s)) > 3 || ms > (s >= SND_TITLE1 ? 2600 : 1700)) { bad++; printf("    sound %d at %d Hz: %.0f ms, peak %.2f, rms %.3f%s\n", s, sr, ms, pk, rms, nan ? " NaN" : ""); }
      }
      check(!bad, "all 48 sounds render cleanly at 16 kHz (device) and 32 kHz (web)"); }
    SimPlat p; Game g(&p); g.begin(); grown(g, p, CHILD_END + 60, Game::packLooks(1, 1, 0));
    PetData& d = g.pet.d; d.digestTimer = 1e6f; d.droppings = 0; d.floorMess = 0; d.giftOnFloor = 0; g.sv.volume = 2;
    auto snap = [&](int* c) { memcpy(c, p.soundCount, sizeof p.soundCount); };
    auto total = [&](const int* c) { int n = 0; for (int i = 0; i < SND_COUNT; i++) n += p.soundCount[i] - c[i]; return n; };
    auto got = [&](const int* c, Sound s) { return p.soundCount[s] - c[s]; };
    int c[SND_COUNT];
    g.blobX_ = g.blobTx_ = 0; run(g, p, 0.3f);
    snap(c); tapDot(g, p, 34, 3); run(g, p, 0.2f);
    check(got(c, SND_TAP) == 1 && total(c) == 1, "a tap on an empty spot: one tap bloop");
    snap(c); tapIcon(g, p, IC_PLAY); run(g, p, 0.2f);
    check(got(c, SND_MENU) == 1 && total(c) == 1, "opening the Play menu: the menu sound alone (no tap under it)");
    snap(c); tapIcon(g, p, IC_PLAY); run(g, p, 0.2f);
    check(got(c, SND_BACK) == 1 && total(c) == 1, "backing out: the back sound");
    snap(c); tapIcon(g, p, IC_METER); run(g, p, 0.2f); tapDot(g, p, 20, 14); run(g, p, 0.2f);
    check(got(c, SND_MENU) == 1 && got(c, SND_PAGE) == 1 && total(c) == 2, "the Meter: opens, then a page flip");
    g.setScreen(S_MAIN); run(g, p, 0.2f);
    snap(c); tapIcon(g, p, IC_LIGHT); run(g, p, 0.2f); tapIcon(g, p, IC_LIGHT); run(g, p, 0.2f);
    check(got(c, SND_LIGHT_OFF) == 1 && got(c, SND_LIGHT_ON) == 1, "the light: one sound off, another on");
    g.pet.sleeping = false; run(g, p, 0.5f); g.setScreen(S_MAIN);
    snap(c); d.hunger = 100; tapIcon(g, p, IC_FEED); run(g, p, 2.0f);
    check(got(c, SND_NOPE) == 1, "full up: the refusal sound");
    g.setScreen(S_MAIN); d.hunger = 30; snap(c); tapIcon(g, p, IC_FEED); run(g, p, 12.0f);
    check(got(c, SND_SCOOP) == 1 && got(c, SND_EAT) + got(c, SND_GULP) >= 1, "a meal: served, then eating sounds");
    g.setScreen(S_MAIN); snap(c); tapIcon(g, p, IC_MEDS); run(g, p, 2.0f);
    check(got(c, SND_NOPE) == 1, "medicine when well: the refusal sound");
    g.setScreen(S_MAIN); g.pet.getSick(SICK_HUNGER); d.hunger = 80; snap(c); tapIcon(g, p, IC_MEDS); run(g, p, 4.0f);
    check(got(c, SND_MEDS) == 1 && got(c, SND_SPARKLE) == 1 && got(c, SND_WIN) == 1, "medicine when sick: the dose, the sparkle, better");
    g.setScreen(S_MAIN); run(g, p, 3.0f); g.petCd_ = 0; g.blobX_ = g.blobTx_ = 10; run(g, p, 0.1f);
    snap(c); tapDot(g, p, (int)g.blobX_ + 3, FLOOR_ROW - 2); run(g, p, 0.3f);
    check(got(c, SND_PET) == 1 && total(c) == 1, "petting them: the purr-squish");
    run(g, p, 3.0f); g.setScreen(S_MAIN);
    snap(c); tapIcon(g, p, IC_LOCK); run(g, p, 0.3f); tapDot(g, p, 10, 23); run(g, p, 3.0f);
    check(got(c, SND_MENU) == 1 && got(c, SND_LOCK) == 1, "locking: the question, then the padlock clunk");
    run(g, p, 2.0f); snap(c); tapIcon(g, p, IC_LOCK); run(g, p, 0.3f); tapDot(g, p, 10, 23); run(g, p, 3.0f);
    check(got(c, SND_UNLOCK) == 1, "unlocking: the padlock springs open");
    run(g, p, 2.0f); g.setScreen(S_MAIN);
    snap(c); tapIcon(g, p, IC_LITTER); run(g, p, 0.3f);
    check(got(c, SND_PAGE) == 1, "nothing to clean: a little all-clean sound");
    run(g, p, 2.0f); d.droppings = 2; run(g, p, 0.3f); snap(c); tapIcon(g, p, IC_LITTER); run(g, p, 0.2f); scrubSolve(g, p); run(g, p, 2.6f);
    check(got(c, SND_SCRUB) >= 3 && got(c, SND_BUBBLE) == 2 && got(c, SND_SPARKLE) == 1, "scrubbing: wet rubs, a pop per poop, the sparkle at the end");
    g.setScreen(S_MAIN); g.pottyPending_ = 1; d.droppings = 1; snap(c); run(g, p, 4.0f);
    check(got(c, SND_PLOP) == 1 && got(c, SND_SCRATCH) == 1, "going potty: a squat squish, then the plop");
    d.droppings = 0; g.setScreen(S_MAIN); run(g, p, 0.3f);
    snap(c); g.startYeet(); g.yt_.phase = 1; run(g, p, 0.1f);
    g.yeetLaunch(-0.8f * Game::YEET_PULL, 0.5f * Game::YEET_PULL);
    for (int t = 0; t < 30 * 30 && g.yt_.phase < 4; t++) step(g, p);
    if (getenv("DBGS")) printf("   whoosh %d splat %d bounce %d phase %d\n", got(c, SND_WHOOSH), got(c, SND_SPLAT), got(c, SND_BOUNCE), g.yt_.phase);
    check(got(c, SND_WHOOSH) == 1 && got(c, SND_SPLAT) == 1, "YEET: a whoosh off the sling, one splat where they land");
    // volume off: silence
    g.setScreen(S_MAIN); g.sv.volume = 0; snap(c); tapIcon(g, p, IC_PLAY); run(g, p, 0.3f); tapDot(g, p, 10, 4); run(g, p, 1.0f);
    check(total(c) == 0, "sound off: nothing plays");
  }
  void box() {
    printf("litter box on the home screen\n");
    SimPlat p; Game g(&p); g.begin(); grown(g, p, CHILD_END + 60, Game::packLooks(1, 1, 0));
    PetData& d = g.pet.d; d.digestTimer = 1e6f; d.droppings = 0; d.floorMess = 0;
    auto wall = [&]() { return g.dots()[(FLOOR_ROW - 2) * MW + BOX_X + BOX_W - 1] == SH_INK && g.dots()[FLOOR_ROW * MW + BOX_X + 5] == SH_INK; };
    g.blobX_ = g.blobTx_ = 0; run(g, p, 0.3f);
    check(!wall(), "no poop waiting: no box on screen");
    d.droppings = 2; run(g, p, 0.3f);
    check(wall(), "poop waiting: the box is out (bottom right)");
    tapDot(g, p, BOX_X + 5, FLOOR_ROW - 2); run(g, p, 0.2f);
    check(g.scr == S_SCRUB, "tapping the box opens the scrub");
    scrubSolve(g, p); run(g, p, 2.6f);
    if (!(g.scr == S_MAIN && !d.droppings && !wall())) printf("   (scr %d drop %d wall %d an %d)\n", (int)g.scr, (int)d.droppings, (int)wall(), (int)g.an_);
    check(g.scr == S_MAIN && !d.droppings && !wall(), "scrubbed clean: the box is put away");
    // using it: the box comes out, they walk over, and step back off it
    g.pottyPending_ = 1; d.droppings = 1; step(g, p);
    bool sawBoxEarly = false, overBox = false;
    for (int i = 0; i < 30 * 4 && g.scr == S_ANIM; i++) { step(g, p); if (g.anT_ < 0.4f && wall()) sawBoxEarly = true; if (g.anT_ > 1.0f && g.anT_ < 2.0f) overBox = true; }
    check(g.scr == S_MAIN && sawBoxEarly && overBox, "going potty: the box is out while they use it");
    int bad = 0;
    for (int f = F_BABY; f < F_COUNT; f++) {
      if (f == F_BB_BLOBBERT || f == F_BB_BALLBOI) continue;   // (the two giants nearly fill the screen: they lean on the box)
      if (!setLook(g, (Form)f)) continue;
      g.scr = S_MAIN; g.an_ = AN_NONE; g.pet.sleeping = false; d.exploring = 0; d.droppings = 3; d.floorMess = 0; d.hunger = d.fun = 80;
      run(g, p, 3.0f); g.scr = S_MAIN; g.an_ = AN_NONE;
      int idle, alt, sl; g.blobSprite((Form)f, idle, alt, sl);
      for (int i = 0; i < 30 * 25; i++) { step(g, p); d.droppings = 3; if (g.scr == S_MAIN && (int)g.blobX() + spriteW(idle) > BOX_X) { bad++; printf("    look %d stands on the box (x %d w %d)\n", f, (int)g.blobX(), spriteW(idle)); break; } }
    }
    check(!bad, "no look ever stands on the box");
  }
  void timeout() {
    printf("games left alone\n");
    const int acts[] = {5, 11, 12, 13, 14, 2};
    const char* nm[] = {"dodge", "run", "orbit", "poop up", "yeet", "scrub"};
    for (int k = 0; k < 6; k++) {
      SimPlat p; Game g(&p); g.begin(); grown(g, p, CHILD_END + 60, Game::packLooks(1, 1, 0));
      g.pet.d.droppings = 3;
      g.pet.d.digestTimer = 1e6f;   /* (no potty trip in the middle of the wait) */ g.debugAct(acts[k]); run(g, p, 0.3f);
      const Screen s0 = g.scr;
      run(g, p, 20); for (int i = 0; i < 45; i++) step(g, p, true, 92, 4); step(g, p);   // a touch (held: not a tap) at 20 s keeps it
      run(g, p, 25);
      const bool kept = g.scr == s0;
      run(g, p, 6);
      if (getenv("DBG")) printf("    s0 %d kept %d now %d\n", (int)s0, kept, (int)g.scr);
      char b[80]; snprintf(b, sizeof b, "%-8s: home after 30 s untouched (kept while touched)", nm[k]);
      check(s0 != S_MAIN && kept && g.scr == S_MAIN, b);
    }
  }
  void scrub() {
    printf("scrub\n");
    float minMul = 99, maxMul = 0, tFew = 0, tMid = 0, tMany = 0; int shapesSeen = 0, bad = 0, nFew = 0, nMid = 0, nMany = 0;
    for (int seed = 0; seed < 40; seed++) {
      const int mess = 1 + seed % (BOX_MAX + 4);
      SimPlat p; p.ms = 1000 + seed * 7717; Game g(&p); g.begin(); grown(g, p, CHILD_END + 60, Game::packLooks(1, 1, 0));
      g.pet.rng = 4243u * (seed + 1);
      g.pet.d.droppings = (uint8_t)(mess > BOX_MAX ? BOX_MAX : mess); g.pet.d.floorMess = (uint8_t)(mess > BOX_MAX ? mess - BOX_MAX : 0); g.pet.d.digestTimer = 1e6f;
      tapIcon(g, p, IC_LITTER); run(g, p, 0.1f);
      const float need = g.sc_.need, base = g.sc_.base, mul = need / base;
      for (int i = 0; i < g.sc_.np; i++) { shapesSeen |= 1 << g.sc_.poo[i].shape; const auto& q = g.sc_.poo[i]; if (q.x < 0 || q.y < 0 || q.x >= MW || q.y >= MH) bad++; }
      if (mul < minMul) minMul = mul;
      if (mul > maxMul) maxMul = mul;
      const float easy = base / 108.0f;   // all easy, scrubbing briskly
      const float t = scrubSolve(g, p);
      run(g, p, 2.5f);
      if (g.sc_.np != mess) bad++;   // every poop they've done is there to scrub
      (mess <= 2 ? tFew : mess >= 10 ? tMany : tMid) += t; (mess <= 2 ? nFew : mess >= 10 ? nMany : nMid)++;
      if (seed < BOX_MAX + 4) printf("    %2d mess: %.1fx the rubbing (all easy: %.1f s) -> clean in %.1f s\n", mess, mul, easy, t);
      if (easy < 1.7f || easy > 17.5f || mul < 1.0f || mul > 2.0f || t > easy * mul * 1.35f + 0.5f || g.scr != S_MAIN || g.pet.d.droppings || g.pet.d.floorMess) bad++;
    }
    check(!bad, "40 boards (1 to 16 poops): all there, cleaned, all easy takes 2 s for one and 17 s for 16, never more than 2x that");
    { char c[110]; snprintf(c, sizeof c, "more poop is more scrubbing: %.1f s for 1-2, %.1f s for 3-9, %.1f s for 10 and up", tFew / nFew, tMid / nMid, tMany / nMany);
      check(tMany / nMany > 3 * tFew / nFew && tMid / nMid > tFew / nFew && tMany / nMany < 35, c); }
    char b[90]; snprintf(b, sizeof b, "it varies: %.1fx to %.1fx the rubbing, %d poop shapes seen", minMul, maxMul, __builtin_popcount(shapesSeen));
    check(maxMul - minMul > 0.3f && __builtin_popcount(shapesSeen) == 4, b);
  }
  void yeet() {
    printf("yeet\n");
    { // the whole flow, by touch
      SimPlat p; Game g(&p); g.begin(); grown(g, p, TEEN_END + 60, 37);
      g.sv.yeetBest = 0; g.pet.d.fun = 30; 
      tapIcon(g, p, IC_PLAY); run(g, p, 0.2f); tapDot(g, p, 10, 20); run(g, p, 0.2f);
      check(g.scr == S_YEET && g.yt_.phase == 0, "YEET from the Play menu: the card first");
      bool clear = true; for (int i = 0; i < 10; i++) if (g.yt_.ob[i].on && g.yt_.ob[i].x < MW) clear = false;
      run(g, p, 0.5f); tapDot(g, p, 20, 14); run(g, p, 0.2f);
      check(g.yt_.phase == 1, "a tap: the sling");
      // press, drag back and down-left ~20 mm, let go
      step(g, p, true, MX + 100, MY + 50);
      for (int i = 1; i <= 15; i++) step(g, p, true, MX + 100 - i * 8, MY + 50 + i * 5);
      check(g.yt_.pullX < -6.0f, "dragging back pulls the band");
      step(g, p); run(g, p, 0.1f);
      check(g.yt_.phase == 2, "let go: they fly");
      for (int i = 0; i < 10; i++) if (g.yt_.ob[i].on && g.yt_.ob[i].x < MW) clear = false;
      check(clear, "nothing out there is on screen before the launch");
      for (int i = 0; i < 30 * 40 && g.yt_.phase < 4; i++) step(g, p);
      check(g.yt_.phase == 4, "they land, then the score");
      check(g.sv.yeetBest == g.yt_.meters && g.yt_.meters > 0, "the best is saved");
      check(g.pet.d.fun > 30 || g.yt_.hearts == 0, "fun goes up");
      run(g, p, 0.7f); tapDot(g, p, 20, 14); run(g, p, 0.2f);
      check(g.scr == S_MAIN, "tap the score away");
      // a tiny pull just snaps back
      g.startYeet(); run(g, p, 0.4f); tapDot(g, p, 20, 14); run(g, p, 0.3f);
      step(g, p, true, MX + 100, MY + 50); step(g, p, true, MX + 96, MY + 50); step(g, p); run(g, p, 0.2f);
      check(g.yt_.phase == 1, "a tiny pull doesn't launch");
    }
    // the physics by stage: plenty of shots each, at random angles
    const struct { const char* nm; float age; uint8_t looks; } st[5] = {{"Lil Blob", 600, 0}, {"Lil Blub", BABY_END + 60, 1}, {"Lil Blunkus", CHILD_END + 60, Game::packLooks(1, 1, 0)}, {"Lil Fella", TEEN_END + 60, 37}, {"Big Blorbus", AGE_BLORBUS + 60, 37}};
    for (int s = 0; s < 5; s++) {
      SimPlat p; Game g(&p); g.begin(); grown(g, p, st[s].age, st[s].looks);
      if (s == 4) { g.evoSetChoice(4, 1); }
      if (stageOfForm(g.form()) != s) { printf("    (stage %d: got look %d, stage %d)\n", s, (int)g.form(), stageOfForm(g.form())); }
      std::vector<int> ms; float worstT = 0; bool sane = true; int stuck = 0;
      for (int i = 0; i < 300; i++) {
        g.pet.rng = 1234567u + i * 2654435761u;
        const float a = (10 + (i * 37) % 60) * 3.14159f / 180.0f;   // 10..70 degrees up
        float secs = 0;
        const int m = yeetOnce(g, p, -cosf(a) * Game::YEET_PULL, sinf(a) * Game::YEET_PULL, &secs, &sane);
        if (m < 0) stuck++; else ms.push_back(m);
        if (secs > 15.0f && getenv("DBGY")) { printf("      long: %.1f s, x %.0f y %.0f vx %.1f vy %.1f ground %d mud %d | near:", secs, g.yt_.x, g.yt_.y, g.yt_.vx, g.yt_.vy, g.yt_.onGround, g.yt_.inMud);
          for (int o = 0; o < 10; o++) if (g.yt_.ob[o].on && fabsf(g.yt_.ob[o].x - g.yt_.x) < 20) printf(" k%d@%.0f,%.0f", g.yt_.ob[o].kind, g.yt_.ob[o].x - g.yt_.x, g.yt_.ob[o].y); printf("\n"); }
        if (secs > worstT) worstT = secs;
      }
      std::sort(ms.begin(), ms.end());
      const int n = (int)ms.size();
      int h[5] = {0}; for (int m : ms) h[m >= 150 ? 4 : m >= 100 ? 3 : m >= 60 ? 2 : m >= 30 ? 1 : 0]++;
      printf("    %-11s (look %2d): min %3d  25%% %3d  median %3d  75%% %3d  max %3d m | hearts 0-4: %d %d %d %d %d | longest %.1f s\n", st[s].nm, (int)g.form(),
             n ? ms[0] : 0, n ? ms[n / 4] : 0, n ? ms[n / 2] : 0, n ? ms[3 * n / 4] : 0, n ? ms[n - 1] : 0, h[0], h[1], h[2], h[3], h[4], worstT);
      char b[90]; snprintf(b, sizeof b, "%s: every shot lands (in 30 s), never through the floor", st[s].nm);
      check(!stuck && sane && worstT <= 31.0f, b);
    }
  }
};
}  // namespace lf
static int cmdNew() {
  lf::NewTests t;
  t.sickness(); t.box(); t.meds(); t.timeout(); t.scrub(); t.yeet(); t.yeetClip(); t.lock(); t.sounds();
  return t.fails ? (printf("new: %d FAILED\n", t.fails), 1) : (printf("new: all ok\n"), 0);
}

namespace lf { struct NewShots { static int run_(const char* dir);
  static void run(Game& g, SimPlat& p, float sec) { ::run(g, p, sec); } }; }
static int cmdNewShots(const char* dir) { return lf::NewShots::run_(dir); }
#include "v010.inc"
int lf::NewShots::run_(const char* dir) {
  outDir = dir;
  { // play menu, sick looks
    SimPlat p; Game g(&p); g.begin(); NewTests::grown(g, p, TEEN_END + 60, 37);
    tapIcon(g, p, IC_PLAY); run(g, p, 0.3f); shot(g, "play_menu");
    step(g, p, true, MX + 10 * DOT, MY + 20 * DOT); shot(g, "play_menu_press_yeet"); step(g, p); g.debugAct(-1); run(g, p, 0.3f);
    for (int f = F_BABY; f < F_COUNT; f++) {
      if (!NewTests::setLook(g, (Form)f)) continue;
      memset(g.pet.d.sick, 0, 3); g.pet.d.sick[SICK_HUNGER] = f % 3 == 0 ? 1 : 0; g.pet.d.sick[SICK_LIGHT] = 1; if (f % 4 == 0) g.pet.d.sick[SICK_BUG] = 1;
      g.scr = S_MAIN; g.an_ = AN_NONE; g.pet.sleeping = false; g.pet.d.exploring = 0; g.pet.d.giftOnFloor = 0; run(g, p, 2.6f); g.scr = S_MAIN; g.an_ = AN_NONE; g.blobX_ = g.blobTx_ = 6;
      g.anim_ = 0.6f; g.m_.clear(); g.drawScene(true); char nm[32]; snprintf(nm, sizeof nm, "sick_%02d", f);
      // render via a frame so the icons show
      g.anim_ = 0.6f; step(g, p); shot(g, nm);
    }
    memset(g.pet.d.sick, 0, 3);
    g.debugAct(6); for (int k = 0; k < 5; k++) tapDot(g, p, 20, 14); run(g, p, 0.3f); shot(g, "meter_last");
  }
  for (int v = 0; v < 6; v++) { // scrub boards: they differ
    SimPlat p; p.ms = 300 + v * 977; Game g(&p); g.begin(); NewTests::grown(g, p, TEEN_END + 60, 37);
    g.pet.rng = 99u + v * 7919u;
    g.pet.d.droppings = (uint8_t)(2 + v % 5); g.pet.d.floorMess = (uint8_t)(v % 4); g.pet.d.digestTimer = 1e6f; run(g, p, 0.5f);
    tapIcon(g, p, IC_LITTER); run(g, p, 0.6f); char nm[24]; snprintf(nm, sizeof nm, "scrub_board%d", v); shot(g, nm);
  }
  for (int n = 0; n < 4; n++) { // the litter box on the home screen
    SimPlat p; Game g(&p); g.begin(); NewTests::grown(g, p, n == 3 ? TEEN_END + 60 : CHILD_END + 60, n == 3 ? 37 : Game::packLooks(1, 1, 0));
    static const int cnt[4] = {0, 1, 4, 6};
    g.pet.d.droppings = (uint8_t)cnt[n]; g.pet.d.floorMess = n == 2 ? 2 : 0; g.pet.d.digestTimer = 1e6f; run(g, p, 3.0f); g.scr = S_MAIN;
    run(g, p, 0.5f); char nm[24]; snprintf(nm, sizeof nm, "homebox_%d", cnt[n]); shot(g, nm);
    if (n == 1) { g.pottyPending_ = 1; step(g, p); for (int i = 0; i < 80; i++) { step(g, p); if (i == 8 || i == 35 || i == 72) { snprintf(nm, sizeof nm, "homebox_potty%02d", i); shot(g, nm); } } }
  }
  { // scrub
    SimPlat p; Game g(&p); g.begin(); NewTests::grown(g, p, TEEN_END + 60, 37);
    g.pet.d.droppings = 4; g.pet.d.floorMess = 2; g.pet.d.digestTimer = 1e6f; run(g, p, 0.5f);
    tapIcon(g, p, IC_LITTER); run(g, p, 0.6f); shot(g, "scrub_0");
    for (int i = 0; i < 40; i++) { const int k = i % 16, x = MX + 20 + (k < 8 ? k : 16 - k) * 16; step(g, p, true, x, MY + 70 + (i % 5) * 4); }
    shot(g, "scrub_1");
    for (int i = 0; i < 60; i++) { const int k = i % 16, x = MX + 20 + (k < 8 ? k : 16 - k) * 16; step(g, p, true, x, MY + 50 + (i % 7) * 6); }
    shot(g, "scrub_2");
    scrubSolve(g, p); shot(g, "scrub_done"); run(g, p, 0.3f); shot(g, "scrub_done2"); run(g, p, 0.6f); shot(g, "scrub_clean");
  }
  { // poop up
    SimPlat p; Game g(&p); g.begin(); NewTests::grown(g, p, TEEN_END + 60, 37);
    g.debugAct(13); run(g, p, 0.5f); shot(g, "pop_card"); run(g, p, POP_CARD_SECS); shot(g, "pop_play");
    popPlay(g, p); run(g, p, 0.4f); shot(g, "pop_done"); run(g, p, 2.0f); shot(g, "pop_score");
  }
  { // yeet: a Lil Fella and a Lil Blob
    for (int who = 0; who < 3; who++) {
      SimPlat p; Game g(&p); g.begin();
      if (who == 0) NewTests::grown(g, p, TEEN_END + 60, 37); else if (who == 1) NewTests::grown(g, p, 600, 0); else NewTests::grown(g, p, CHILD_END + 60, Game::packLooks(1, 3, 0));
      g.sv.yeetBest = 57; g.pet.rng = 4242 + who;
      g.debugAct(14); run(g, p, 0.4f); char nm[40];
      snprintf(nm, sizeof nm, "yeet%d_card", who); shot(g, nm);
      tapDot(g, p, 20, 14); run(g, p, 0.5f); snprintf(nm, sizeof nm, "yeet%d_sling", who); shot(g, nm);
      step(g, p, true, MX + 100, MY + 50);
      for (int i = 1; i <= 15; i++) step(g, p, true, MX + 100 - i * 9, MY + 50 + i * 7);
      snprintf(nm, sizeof nm, "yeet%d_aim", who); shot(g, nm);
      step(g, p);
      for (int i = 0; i < 30 * 30 && g.yt_.phase < 4; i++) { step(g, p); if (i % 12 == 6 && i < 30 * 8) { snprintf(nm, sizeof nm, "yeet%d_fly%03d", who, i); shot(g, nm); } if (g.yt_.phase == 3 && g.yt_.landT < 0.04f) { snprintf(nm, sizeof nm, "yeet%d_land", who); shot(g, nm); } }
      run(g, p, 0.4f); snprintf(nm, sizeof nm, "yeet%d_score", who); shot(g, nm);
    }
  }
  return 0;
}

int main(int argc, char** argv) {
  if (argc >= 3 && !strcmp(argv[1], "newshots")) return cmdNewShots(argv[2]);
  if (argc >= 3 && !strcmp(argv[1], "babyeat")) return lf::V010::babyEat(argv[2]);
  if (argc >= 3 && !strcmp(argv[1], "popshots")) return lf::V010::popShots(argv[2]);
  if (argc >= 2 && !strcmp(argv[1], "v010")) return lf::V010::checks();
  if (argc >= 2 && !strcmp(argv[1], "blub")) return lf::V010::blub();
  if (argc >= 3 && !strcmp(argv[1], "wikishots")) return lf::V010::wikiShots(argv[2]);
  if (argc >= 2 && !strcmp(argv[1], "new")) return cmdNew();
  if (argc >= 3 && !strcmp(argv[1], "train")) return cmdTrain(argv[2]);
  if (argc >= 2 && !strcmp(argv[1], "lessons")) return cmdLessons();
  if (argc >= 2 && !strcmp(argv[1], "dodgebot")) { for (int s = 0; s < 12; s++) printf("%.1f ", DodgeBot::run(s) / 10.0f); printf("\n"); return 0; }
  if (argc >= 2 && !strcmp(argv[1], "fuzz")) return cmdFuzz(argc >= 3 ? atoi(argv[2]) : 8);
  if (argc >= 2 && !strcmp(argv[1], "flows")) return cmdFlows();
  if (argc >= 2 && !strcmp(argv[1], "endings")) return cmdEndings();
  if (argc >= 2 && !strcmp(argv[1], "touch")) return cmdTouch();
  if (argc >= 2 && !strcmp(argv[1], "pop")) return lf::V010::pop();
  if (argc >= 2 && !strcmp(argv[1], "growth")) return cmdGrowth();
  if (argc >= 2 && !strcmp(argv[1], "eat")) return cmdEat();
  if (argc >= 2 && !strcmp(argv[1], "journal")) return JournalTest::all();
  if (argc >= 2 && !strcmp(argv[1], "bench")) return cmdBench();
  if (argc >= 2 && !strcmp(argv[1], "tree")) return cmdTree();
  if (argc >= 2 && !strcmp(argv[1], "blorbus")) return cmdBlorbus();
  if (argc >= 2 && !strcmp(argv[1], "newgames")) return cmdNewGames();
  if (argc >= 2 && !strcmp(argv[1], "name")) return cmdName();
  if (argc >= 2 && !strcmp(argv[1], "newborn")) return cmdNewborn();
  if (argc >= 2 && !strcmp(argv[1], "hello")) return cmdHello();
  if (argc >= 3 && !strcmp(argv[1], "eatshots")) return cmdEatShots(argv[2], argc >= 4 ? atoi(argv[3]) : 1);
  if (argc >= 2 && !strcmp(argv[1], "touchmap")) return cmdTouchMap();
  if (argc >= 3 && !strcmp(argv[1], "sessions")) return cmdSessions(argv[2], argc >= 4 ? atoi(argv[3]) : 60);
  if (argc >= 4 && !strcmp(argv[1], "trace")) return cmdTrace(atoi(argv[2]), atoi(argv[3]));
  if (argc >= 3 && !strcmp(argv[1], "games")) return cmdGames(argv[2], argc >= 4 ? atoi(argv[3]) : 80);
  if (argc >= 2 && !strcmp(argv[1], "shots")) { if (argc >= 3) outDir = argv[2]; return cmdShots(); }
  if (argc >= 2 && !strcmp(argv[1], "life")) return cmdLife();
  printf("usage: sim shots <dir> | life | flows | touch | litter | growth\n");
  return 1;
}
