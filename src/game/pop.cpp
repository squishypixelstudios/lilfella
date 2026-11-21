// POP (Play menu): a bubble shooter after the arcade classic Puzzle Bobble (1994).
//
// Bubbles hang from the ceiling in staggered rows. A launcher at the bottom fires one bubble at a time: slide a
// finger left and right to aim (a dotted line shows the way, bounces off the side walls included), let go to fire.
// The bubble sticks where it lands. Three or more of the same kind touching pop, and anything left hanging with no
// way back to the ceiling drops off with them. Every few shots the ceiling comes down a row. Clear the field to
// win; a bubble pushed below the line ends the game.
//
// The LCD has no colours, so the five kinds are told apart by their shape: an X, a triangle, a grey diamond,
// an empty ring, and a bubble that is black all through.
//
// POP has the whole screen. The icons are put away and its LCD is POP_MH rows tall (the usual one is 28); a small
// BACK button, top right, leaves. A finger anywhere else aims, so there is nothing under it to press by mistake.
//
// The field: 8 bubbles across (5 dots each: the whole 40-dot LCD), the odd rows 7 across and half a bubble along.
// POP_SAFE_ROWS rows fit above the line.
#include "game.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

namespace lf {

namespace {
constexpr int CELL = 5;                       // a bubble is 5 x 5 dots
constexpr int OY = (POP_MH - MH) / 2;         // the how-to card and the result were laid out for 28 rows: they sit in the middle of the taller LCD
constexpr int BACK_X = 126, BACK_Y = 8, BACK_W = 38, BACK_H = 15;   // the BACK button (logical px), clear of the screen's rounded corner
constexpr float LAUNCH_X = 17.0f, LAUNCH_Y = (float)(POP_MH - CELL);   // the launcher's bubble (its top left corner, like every bubble here)
constexpr float SPEED = 46.0f;                // dots a second in flight
constexpr float POP_STEP = 0.5f;              // dots a shot moves per physics step: the flight and the dotted guide/popTrace use the same one, so the guide is exact
constexpr float FALL_GRAV = 90.0f;            // dots a second squared: how fast popped-off bubbles drop
constexpr float AIM_RANGE_PX = 74.0f;         // finger distance from the middle (logical px) for the full aim angle
constexpr float OVER_SECS = 1.6f;             // how long the end of a game is shown before the result card
constexpr float RESULT_TAP_SECS = 0.6f;       // the result card ignores taps this long (so the last one does not skip it)
constexpr float HIT = 4.2f;                   // centres closer than this: it has hit (a little under a bubble's width, so
                                              // a shot can squeeze through a one-bubble gap)
// '#' black, 'o' grey. A bubble is only 5 dots across (about 3 mm), too small for a mark inside a ring to read:
// tried that way, the X and the triangle were both a grey smudge. So the X and the triangle are drawn as the mark
// itself, the full size of the bubble; the diamond is grey with a black edge (it was a block in a round grey skin,
// which read as a third, faded circle beside the ring and the black one); the empty one is a plain ring and the
// black one is dark right through. Any two differ in at least 8 of their 25 dots.
const char* const ART[PK_COUNT][CELL] = {
  {".....", ".....", ".....", ".....", "....."},   // PK_NONE: an empty cell (bubble() never draws it; kept so rows match the PK_ values)
  {"#...#", ".#.#.", "..#..", ".#.#.", "#...#"},   // X
  {"..#..", "..#..", ".###.", ".###.", "#####"},   // triangle
  {"..#..", ".#o#.", "#ooo#", ".#o#.", "..#.."},   // diamond
  {".###.", "#...#", "#...#", "#...#", ".###."},   // empty
  {".###.", "#####", "#####", "#####", ".###."},   // black
};
void bubble(Matrix& m, int x, int y, int kind) {
  if (kind <= PK_NONE || kind >= PK_COUNT) return;
  for (int r = 0; r < CELL; r++) for (int c = 0; c < CELL; c++) {
    const char ch = ART[kind][r][c];
    if (ch != '.') m.set(x + c, y + r, ch == '#' ? SH_INK : SH_MID);
  }
}
}  // namespace

// The up-to-six bubbles that touch (r, c). Even rows sit at x = 5c, odd rows at x = 5c + 2.
int Game::popNeighbours(int r, int c, int nr[6], int nc[6]) {
  int n = 0;
  auto add = [&](int rr, int cc) { if (rr >= 0 && rr < POP_ROWS && cc >= 0 && cc < popCols(rr)) { nr[n] = rr; nc[n] = cc; n++; } };
  add(r, c - 1); add(r, c + 1);
  for (int dr = -1; dr <= 1; dr += 2) {
    if (r & 1) { add(r + dr, c); add(r + dr, c + 1); }
    else { add(r + dr, c - 1); add(r + dr, c); }
  }
  return n;
}

int Game::popCount() const {
  int n = 0;
  for (int r = 0; r < POP_ROWS; r++) for (int c = 0; c < popCols(r); c++) if (pop_.g[r][c]) n++;
  return n;
}

// A kind for the launcher: one that is still on the field (so no shot is ever useless), any kind if it's empty.
uint8_t Game::popPick() {
  bool have[PK_COUNT] = {false};
  int kinds = 0;
  for (int r = 0; r < POP_ROWS; r++) for (int c = 0; c < popCols(r); c++) { const uint8_t k = pop_.g[r][c]; if (k && k < PK_COUNT && !have[k]) { have[k] = true; kinds++; } }
  if (!kinds) return (uint8_t)(1 + pet.rnd() % (PK_COUNT - 1));
  int pick = (int)(pet.rnd() % (uint32_t)kinds);
  for (int k = 1; k < PK_COUNT; k++) if (have[k] && pick-- == 0) return (uint8_t)k;
  return PK_X;
}

// Where a finger at logical x aims: straight up from the middle of the screen, further over toward the edges.
float Game::popAngleAt(int px) {
  float k = (px - W / 2) / AIM_RANGE_PX;
  if (k < -1) k = -1;
  if (k > 1) k = 1;
  return k * POP_MAX_ANG;
}

// One small step of a bubble in flight (top left corner x, y; velocity vx, vy; `dist` dots). True when it has hit the
// ceiling or another bubble: (r, c) is then the free spot it settles into (r = -1 if there is none, which a full
// field can do).
bool Game::popAdvance(float& x, float& y, float& vx, float& vy, float dist, int& r, int& c, bool* bounced) const {
  const Pop& P = pop_;
  const float sp = sqrtf(vx * vx + vy * vy);
  if (sp < 0.001f) return false;
  x += vx / sp * dist; y += vy / sp * dist;
  const float xmax = (float)(MW - CELL);
  if (x < 0) { x = -x; vx = -vx; if (bounced) *bounced = true; }
  if (x > xmax) { x = 2 * xmax - x; vx = -vx; if (bounced) *bounced = true; }
  bool hit = y <= (float)popCellY(0);
  for (int rr = 0; rr < POP_ROWS && !hit; rr++) for (int cc = 0; cc < popCols(rr); cc++) {
    if (!P.g[rr][cc]) continue;
    const float ddx = x - popCellX(rr, cc), ddy = y - popCellY(rr);
    if (ddx * ddx + ddy * ddy < HIT * HIT) { hit = true; break; }
  }
  if (!hit) return false;
  // the nearest free spot that hangs from something: the ceiling, or a bubble it would touch
  float best = 1e9f; r = -1; c = -1;
  for (int rr = 0; rr < POP_ROWS; rr++) {
    if (rr + P.drops > POP_SAFE_ROWS) break;   // (one row below the line can be landed in: that ends the game)
    for (int cc = 0; cc < popCols(rr); cc++) {
      if (P.g[rr][cc]) continue;
      bool held = rr == 0;
      int nr[6], nc[6];
      const int n = popNeighbours(rr, cc, nr, nc);
      for (int i = 0; i < n && !held; i++) if (P.g[nr[i]][nc[i]]) held = true;
      if (!held) continue;
      const float ddx = x - popCellX(rr, cc), ddy = y - popCellY(rr), d2 = ddx * ddx + ddy * ddy;
      if (d2 < best) { best = d2; r = rr; c = cc; }
    }
  }
  return true;
}

// Where a shot at this angle ends up (the sim and the tests use it; the dotted guide traces the same path in screenPop). False if it has nowhere to land.
bool Game::popTrace(float ang, int& r, int& c, int* bounces) const {
  float x = LAUNCH_X, y = LAUNCH_Y, vx = sinf(ang) * SPEED, vy = -cosf(ang) * SPEED;
  if (bounces) *bounces = 0;
  for (int i = 0; i < 2000; i++) {
    bool b = false;
    if (popAdvance(x, y, vx, vy, POP_STEP, r, c, &b)) return r >= 0;
    if (b && bounces) (*bounces)++;
  }
  return false;
}

void Game::startPop() {
  Pop& P = pop_;
  memset(&P, 0, sizeof P);
  // the field: POP_START_ROWS full rows and a few more hanging under them. Each bubble often takes the kind of one
  // it touches, so the field comes in clumps (as the arcade's levels do) and a good shot clears several at once.
  auto fill = [&](int r, int c) {
    int nr[6], nc[6];
    const int n = popNeighbours(r, c, nr, nc);
    uint8_t near[6]; int m = 0;
    for (int i = 0; i < n; i++) if (P.g[nr[i]][nc[i]]) near[m++] = P.g[nr[i]][nc[i]];
    P.g[r][c] = m && pet.rnd() % 100 < 55 ? near[pet.rnd() % (uint32_t)m] : (uint8_t)(1 + pet.rnd() % (PK_COUNT - 1));
  };
  for (int r = 0; r < POP_START_ROWS; r++) for (int c = 0; c < popCols(r); c++) fill(r, c);
  for (int k = 0; k < POP_START_EXTRA; k++) {
    const int c = (int)(pet.rnd() % (uint32_t)popCols(POP_START_ROWS));
    if (!P.g[POP_START_ROWS][c]) fill(POP_START_ROWS, c);
  }
  P.start = popCount();
  P.cur = popPick(); P.next = popPick();
  P.card = POP_CARD_SECS;
  P.phase = 1;
  selIcon_ = IC_PLAY;
  setScreen(S_POP);
}

void Game::popFire(float ang) {
  Pop& P = pop_;
  if (P.phase != 1) return;
  if (ang < -POP_MAX_ANG) ang = -POP_MAX_ANG;
  if (ang > POP_MAX_ANG) ang = POP_MAX_ANG;
  P.ang = ang;
  P.bx = LAUNCH_X; P.by = LAUNCH_Y; P.vx = sinf(ang) * SPEED; P.vy = -cosf(ang) * SPEED;
  P.phase = 2; P.carry = 0;
  sound(SND_BLOW);
}

// The game is over: the hearts, and what the fella gets out of it.
void Game::popEnd(bool won) {
  Pop& P = pop_;
  P.won = won; P.phase = 3; P.overT = 0;
  // cleared before the ceiling moved: 4. Cleared after one drop: 3. Cleared at all: 2. Lost, but at least as many popped as there were at the start: 1.
  P.hearts = won ? (P.drops == 0 ? 4 : P.drops == 1 ? 3 : 2) : (P.popped >= P.start ? 1 : 0);
  pet.exercise(4.0f + 5.0f * P.hearts, 0.3f + 0.2f * P.hearts);
  pet.train(ACC_CLEAN, 0.1f * P.hearts);
  sv.lifeGames++;
  sound(won ? SND_WIN : SND_LOSE);
}

// The bubble in flight has landed at (r, c).
void Game::popLand(int r, int c) {
  Pop& P = pop_;
  P.shots++; P.sinceDrop++;
  P.phase = 1;
  auto fxAdd = [&](int rr, int cc, bool fall, int order) {
    for (auto& f : P.fx) if (!f.on) {
      f.on = true; f.x = (float)popCellX(rr, cc); f.y = (float)popCellY(rr); f.vy = 0; f.t = -0.03f * order;
      f.kind = P.g[rr][cc]; f.fall = fall ? 1 : 0;
      break;
    }
  };
  if (r < 0 || r >= POP_ROWS) { popEnd(false); return; }   // nowhere left to put it
  P.g[r][c] = P.cur;
  // the bubbles of its kind it touches, and theirs, and so on
  bool mark[POP_ROWS][POP_COLS] = {};
  int stR[POP_ROWS * POP_COLS], stC[POP_ROWS * POP_COLS], sp = 0, n = 0;
  int grpR[POP_ROWS * POP_COLS], grpC[POP_ROWS * POP_COLS];
  stR[sp] = r; stC[sp++] = c; mark[r][c] = true;
  while (sp) {
    const int rr = stR[--sp], cc = stC[sp];
    grpR[n] = rr; grpC[n++] = cc;
    int nr[6], nc[6];
    const int k = popNeighbours(rr, cc, nr, nc);
    for (int i = 0; i < k; i++) if (!mark[nr[i]][nc[i]] && P.g[nr[i]][nc[i]] == P.cur) { mark[nr[i]][nc[i]] = true; stR[sp] = nr[i]; stC[sp++] = nc[i]; }
  }
  if (n >= 3) {
    for (int i = 0; i < n; i++) { fxAdd(grpR[i], grpC[i], false, i); P.g[grpR[i]][grpC[i]] = PK_NONE; }
    P.popped += n;
    sound(SND_BUBBLE);
    // what still hangs from the ceiling (row 0, and everything touching it one way or another); the rest falls
    bool held[POP_ROWS][POP_COLS] = {};
    sp = 0;
    for (int cc = 0; cc < popCols(0); cc++) if (P.g[0][cc]) { held[0][cc] = true; stR[sp] = 0; stC[sp++] = cc; }
    while (sp) {
      const int rr = stR[--sp], cc = stC[sp];
      int nr[6], nc[6];
      const int k = popNeighbours(rr, cc, nr, nc);
      for (int i = 0; i < k; i++) if (!held[nr[i]][nc[i]] && P.g[nr[i]][nc[i]]) { held[nr[i]][nc[i]] = true; stR[sp] = nr[i]; stC[sp++] = nc[i]; }
    }
    int fell = 0;
    for (int rr = 0; rr < POP_ROWS; rr++) for (int cc = 0; cc < popCols(rr); cc++) if (P.g[rr][cc] && !held[rr][cc]) { fxAdd(rr, cc, true, fell); P.g[rr][cc] = PK_NONE; fell++; }
    P.popped += fell;
    if (fell) sound(SND_COIN);
  } else sound(SND_PLOP);
  if (!popCount()) { popEnd(true); return; }
  if (P.sinceDrop >= POP_DROP_EVERY) { P.sinceDrop = 0; P.drops++; P.shake = 0.35f; sound(SND_SCRATCH); }   // the ceiling comes down a row
  for (int rr = 0; rr < POP_ROWS; rr++) for (int cc = 0; cc < popCols(rr); cc++)
    if (P.g[rr][cc] && rr + P.drops >= POP_SAFE_ROWS) { popEnd(false); return; }                               // below the line
  P.cur = P.next; P.next = popPick();
  bool onField = false;   // (the kind in the launcher was just cleared off the field: swap it for one that's there)
  for (int rr = 0; rr < POP_ROWS; rr++) for (int cc = 0; cc < popCols(rr); cc++) if (P.g[rr][cc] == P.cur) onField = true;
  if (!onField) P.cur = popPick();
}

// The BACK button's touch area: generous, the whole top right corner above the LCD.
bool Game::popBackAt(int px, int py) { return px >= W / 2 + 12 && py < POP_MY - 4; }

// POP's screen furniture, in place of the icons: BACK, top right (it goes dark while a finger is on it).
void Game::drawPopBack(Frame& f) {
  const uint8_t sh = in.down && popBackAt(in.px, in.py) ? SH_INK : SH_MID;
  f.roundFrame(BACK_X, BACK_Y, BACK_W, BACK_H, sh);
  f.text(BACK_X + 8, BACK_Y + 4, "BACK", sh);
}

void Game::screenPop() {
  Pop& P = pop_;
  P.t += dt;
  // The icons are put away: the only button is BACK (a tap in the top right corner of the screen).
  if (in.tap && popBackAt(in.px, in.py)) { sound(SND_BACK); setScreen(S_MAIN); return; }
  char b[16];
  if (P.phase == 4) {   // the result, on a clear screen
    m_.textCenter(OY + 2, P.won ? "CLEAR!" : "STUCK!");
    m_.hearts(OY + 12, P.hearts);
    snprintf(b, sizeof b, "%d POPPED", P.popped);
    m_.smallCenter(OY + 21, b, SH_MID);
    P.overT += dt;
    if (P.overT > RESULT_TAP_SECS && in.tap) setScreen(S_MAIN);
    return;
  }
  if (P.card > 0) {   // the card: how to play, with the five kinds along the bottom (a tap starts it, or it starts by itself)
    m_.smallCenter(OY, "SLIDE = AIM");
    m_.smallCenter(OY + 6, "LET GO =");
    m_.smallCenter(OY + 12, "FIRE");
    m_.smallCenter(OY + 18, "MATCH 3");
    for (int k = 1; k < PK_COUNT; k++) bubble(m_, 2 + (k - 1) * 8, OY + 24, k);
    P.card -= dt;
    if (in.tap && P.card < POP_CARD_SECS - 0.3f) { P.card = 0; sound(SND_TAP); }
    return;
  }
  // ---- play ----
  if (P.phase == 1) {   // aiming: a finger down takes aim (it can slide anywhere), letting go fires
    if (in.pressed && !popBackAt(in.px, in.py)) P.aiming = true;   // (anywhere on the screen but the BACK button)
    if (P.aiming && in.down) P.ang = popAngleAt(in.x);
    if (P.aiming && !in.down) { P.aiming = false; popFire(P.ang); }
  } else if (P.phase == 2) {   // in flight
    // (in the same POP_STEP steps the dotted guide is traced in, so it lands exactly where the guide said)
    P.carry += SPEED * dt;
    while (P.carry >= POP_STEP && P.phase == 2) {
      P.carry -= POP_STEP;
      int r, c; bool bounced = false;
      const bool landed = popAdvance(P.bx, P.by, P.vx, P.vy, POP_STEP, r, c, &bounced);
      if (bounced) sound(SND_BOUNCE);
      if (landed) popLand(r, c);
    }
  } else if (P.phase == 3) {   // over: a moment to see how it ended
    P.overT += dt;
    if (P.overT > OVER_SECS) { P.phase = 4; P.overT = 0; return; }
  }
  if (P.shake > 0) P.shake -= dt;
  const int jx = P.shake > 0 && ((int)(P.t * 24) & 1) ? 1 : 0;   // the field shudders as the ceiling comes down
  // the ceiling, once it has started coming down: a hatched slab
  const int top = popCellY(0);
  for (int y = 0; y < top; y++) for (int x = 0; x < MW; x++) if (y == top - 1 || ((x + y) & 3) == 0) m_.set(x, y, y == top - 1 ? SH_INK : SH_MID);
  // the line: bubbles below it end the game
  const int lineY = POP_SAFE_ROWS * CELL + 1;
  for (int x = 0; x < MW; x += 3) m_.set(x, lineY, SH_MID);
  // the dotted guide while aiming: the way the shot will go, off the walls and all, up to where it lands
  if (P.phase == 1 && P.aiming) {
    float x = LAUNCH_X, y = LAUNCH_Y, vx = sinf(P.ang) * SPEED, vy = -cosf(P.ang) * SPEED;
    int r, c;
    for (int i = 1; i < 400; i++) {
      if (popAdvance(x, y, vx, vy, POP_STEP, r, c, nullptr)) break;
      if (i % 5 == 0 && y < LAUNCH_Y - 3.0f) m_.set((int)lroundf(x) + 2, (int)lroundf(y) + 2, SH_MID);
    }
  }
  // the bubbles
  const bool lostBlink = P.phase == 3 && !P.won && ((int)(P.overT * 6) & 1);
  for (int r = 0; r < POP_ROWS; r++) for (int c = 0; c < popCols(r); c++) {
    if (!P.g[r][c]) continue;
    if (lostBlink && r + P.drops >= POP_SAFE_ROWS) continue;   // the ones over the line blink
    bubble(m_, popCellX(r, c) + jx, popCellY(r), P.g[r][c]);
  }
  // popping and falling ones
  for (auto& f : P.fx) {
    if (!f.on) continue;
    f.t += dt;
    if (f.t < 0) { bubble(m_, (int)f.x, (int)f.y, f.kind); continue; }   // (its turn hasn't come: they go one after another)
    if (f.fall) {
      f.vy += FALL_GRAV * dt; f.y += f.vy * dt;
      if (f.y > POP_MH) { f.on = false; continue; }
      bubble(m_, (int)f.x, (int)lroundf(f.y), f.kind);
    } else {
      if (f.t > 0.28f) { f.on = false; continue; }
      const int rr = 1 + (int)(f.t * 14), cx = (int)f.x + 2, cy = (int)f.y + 2;   // a ring of drops flies out
      static const int8_t D[8][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {-1, 1}, {1, -1}, {-1, -1}};
      for (int k = 0; k < 8; k++) m_.set(cx + D[k][0] * rr, cy + D[k][1] * rr, k < 4 ? SH_INK : SH_MID);
    }
  }
  if (P.phase == 3 && P.won && ((int)(P.overT * 6) & 1)) { m_.sprite(6, 5, SPR_SPARK); m_.sprite(30, 9, SPR_SPARK); m_.sprite(18, 2, SPR_SPARK); m_.sprite(12, 14, SPR_SPARK); m_.sprite(27, 16, SPR_SPARK); }
  // the launcher: the bubble to fire (or the one in flight), a pointer the way it's aimed, the next one waiting at the left
  if (P.phase == 2) bubble(m_, (int)lroundf(P.bx), (int)lroundf(P.by), P.cur);
  else if (P.phase == 1) {
    bubble(m_, (int)LAUNCH_X, (int)LAUNCH_Y, P.cur);
    for (int k = 4; k <= 5; k++) m_.set((int)lroundf(LAUNCH_X + 2 + sinf(P.ang) * k), (int)lroundf(LAUNCH_Y + 2 - cosf(P.ang) * k), SH_INK);
  }
  if (P.phase <= 2) bubble(m_, 1, POP_MH - CELL, P.next);
  // shots left before the ceiling comes down: one pip each, bottom right (the last one blinks)
  {
    const int left = POP_DROP_EVERY - P.sinceDrop;
    for (int i = 0; i < left; i++) {
      if (left == 1 && ((int)(P.t * 4) & 1)) continue;
      m_.rect(MW - 2 - i * 2, POP_MH - 2, 1, 2, SH_INK);
    }
  }
}

}  // namespace lf
