// The Clean scrub (soap icon) and YEET (Play menu).
#include "game.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

namespace lf {

static constexpr int FLOOR = MH - 1;
static int clampi(int v, int a, int b) { return v < a ? a : (v > b ? b : v); }
static float clampf_(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }
static void yHearts(Matrix& m, int y, int n) { for (int i = 0; i < 4; i++) m.sprite(3 + i * 9, y, i < n ? SPR_HEART : SPR_HEART_E); }

// ---------------------------------------------------------------------------------------------
// CLEAN: scrub it. A long litter box fills the screen, with any mess on the floor at its left. Rub a finger back and
// forth: suds build up where you rub and the poop nearest your finger wears away. Every poop is different - its
// shape, where it lies, and how stuck-on it is: tough ones (crusty, then with flies) take up to twice the rubbing
// of an easy one. There's no way to fail. Every poop they've done since the last clean is there (up to BOX_MAX in the
// box, piled three deep, and 4 on the floor), and each one is more to scrub: about 2 s of brisk scrubbing for one easy
// poop and a second more for every other, so a box left to fill up is a 15 to 30 s job.
// ---------------------------------------------------------------------------------------------
namespace {
constexpr int BOX_X0 = 11;   // the box: from here to the right edge (the floor in front of it, at the left)
struct PooShape { uint8_t w, h; const char* rows[5]; };
const PooShape POO_SHAPES[4] = {
  {4, 3, {".XX.", "XXXX", "XXXX"}},                                // a nugget
  {5, 4, {"..X..", ".XXX.", "XX.XX", "XXXXX"}},                    // a swirl
  {7, 3, {".XXXXX.", "XX.X.XX", ".XXXXX."}},                       // a log
  {7, 5, {"...X...", "..XXX..", ".XX.XX.", "XXXXXXX", "XXXXXXX"}}, // a pile
};
uint32_t dotHash(int i, int x, int y) { uint32_t h = (uint32_t)(i * 73856093) ^ (uint32_t)(x * 19349663) ^ (uint32_t)(y * 83492791); h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15; return h; }
}  // namespace

void Game::startScrub() {
  Scrub& S = sc_;
  // A scrub that was left half done is picked up where it stopped: the same poops, as worn away as they were.
  // (Kept until it's finished, a new blob arrives or the device restarts. Poops done since then join them, fresh.)
  const Scrub old = S;
  const bool resume = old.np > 0 && !old.done;
  memset(&S, 0, sizeof S);
  S.box = pet.d.droppings > BOX_MAX ? BOX_MAX : pet.d.droppings;
  S.floor = pet.d.floorMess > 4 ? 4 : pet.d.floorMess;
  if (!S.box && !S.floor) { sound(SND_PAGE); say("ALL CLEAN"); setScreen(S_MAIN); return; }
  const int n = S.box + S.floor;
  S.base = 200.0f + 110.0f * (n - 1);   // the rubbing it takes when every poop is an easy one (dots, at most 4 a frame count): more poops, more rubbing
  // the poop that was the k-th in the box (or on the floor) last time, if there was one
  auto before = [&](int k, bool onFloor) -> decltype(&old.poo[0]) {
    if (!resume || k >= (onFloor ? old.floor : old.box)) return nullptr;
    const int oi = onFloor ? old.box + k : k;
    return oi < old.np ? &old.poo[oi] : nullptr;
  };
  const bool same = resume && old.box == S.box && old.floor == S.floor;   // nothing new since: everything exactly where it was
  // in the box: spread along it, then a second layer and a third on top (each a little narrower: a heap)
  const int perRow = S.box <= 3 ? S.box : S.box <= 8 ? (S.box + 1) / 2 : 4;
  for (int i = 0; i < S.box; i++) {
    auto& p = S.poo[S.np++];
    const auto* o = before(i, false);
    p.shape = o ? o->shape : (uint8_t)(pet.rnd() % 4);
    const PooShape& sh = POO_SHAPES[p.shape];
    const int row = i / perRow, col = i % perRow, left = S.box - row * perRow, cols = left < perRow ? left : perRow, inset = row * 3;
    const int span = MW - 2 - (BOX_X0 + 2) - 2 * inset, slot = span / (cols ? cols : 1);
    const int cx = BOX_X0 + 2 + inset + slot * col + slot / 2 + (int)(pet.rnd() % 3) - 1;
    p.x = (int8_t)clampi(cx - sh.w / 2, BOX_X0 + 1, MW - 1 - sh.w);
    p.y = (int8_t)(FLOOR - sh.h - row * 4 - (int)(pet.rnd() % 2));
    if (o && same) { p.x = o->x; p.y = o->y; }
  }
  // on the floor, in front of the box: the smaller shapes
  static const int8_t FL[4][2] = {{0, 23}, {5, 23}, {2, 18}, {6, 14}};
  for (int i = 0; i < S.floor; i++) {
    auto& p = S.poo[S.np++];
    const auto* o = before(i, true);
    p.shape = o ? o->shape : (uint8_t)(pet.rnd() % 2);
    p.x = FL[i][0]; p.y = (int8_t)(FL[i][1] + 3 - POO_SHAPES[p.shape].h);
  }
  // how stuck-on each one is: mostly easy, now and then a tough one
  static const uint8_t TOUGH[10] = {1, 1, 1, 1, 1, 2, 2, 3, 4, 5};   // (each step is a quarter more rubbing: 5 is twice an easy one)
  S.need = 0;
  for (int i = 0; i < S.np; i++) {
    auto& p = S.poo[i];
    const auto* o = before(i < S.box ? i : i - S.box, i >= S.box);
    p.tough = o ? o->tough : TOUGH[pet.rnd() % 10];
    p.hp0 = p.hp = S.base / n * (1.0f + 0.25f * (p.tough - 1));
    if (o) p.hp = o->hp0 > 0 && o->hp > 0 ? p.hp0 * o->hp / o->hp0 : 0;   // as worn down as it was (scrubbed away stays away)
    S.need += p.hp0;
  }
  if (resume) S.prog = old.prog;   // (so "SCRUB!" isn't flashed again)
  selIcon_ = IC_LITTER;
  setScreen(S_SCRUB);
}

void Game::screenScrub() {
  Scrub& S = sc_;
  S.t += dt;
  int ic;
  if (iconTapped(ic)) {   // leaving before it's clean: nothing's cleaned yet, but the scrubbing done so far is kept (startScrub)
    if (ic == IC_LITTER) { sound(SND_BACK); setScreen(S_MAIN); }
    else doIcon(ic);
    return;
  }
  if (!S.done) {
    // the box: a long tray with a lip, litter in the bottom
    for (int x = BOX_X0; x < MW; x++) m_.set(x, FLOOR, SH_INK);
    for (int y = 19; y < FLOOR; y++) { m_.set(BOX_X0, y, SH_INK); m_.set(MW - 1, y, SH_INK); }
    m_.set(BOX_X0 - 1, 19, SH_INK); m_.set(BOX_X0 + 1, 19, SH_INK); m_.set(MW - 2, 19, SH_INK);
    for (int x = BOX_X0 + 2; x < MW - 2; x += 3) m_.set(x, FLOOR - 1, SH_MID);
    // the poops: each wears away dot by dot as it's scrubbed (tough ones are crusty; the toughest have flies)
    for (int i = 0; i < S.np; i++) {
      const auto& p = S.poo[i];
      if (p.hp <= 0) continue;
      const float frac = p.hp / p.hp0;
      const PooShape& sh = POO_SHAPES[p.shape];
      for (int r = 0; r < sh.h; r++) for (int c = 0; c < sh.w; c++) {
        if (sh.rows[r][c] != 'X') continue;
        if ((dotHash(i, c, r) & 1023) / 1024.0f > frac + 0.1f) continue;   // worn away
        uint8_t shade = frac < 0.4f ? SH_MID : SH_INK;
        if (p.tough >= 3 && ((c + r) & 1) && r > 0 && c > 0 && c < sh.w - 1) shade = SH_MID;   // crusty
        m_.set(p.x + c, p.y + r, shade);
      }
      if (frac > 0.4f && ((int)(anim_ * 2.5f + i) & 1)) m_.sprite(p.x + sh.w / 2 - 1, p.y - 4, (i & 1) ? SPR_STINK1 : SPR_STINK2);   // the smell
      if (p.tough >= 4) {   // a fly or two buzzing round it
        for (int k = 0; k < p.tough - 3; k++) {
          const float a = anim_ * (7.0f + k * 2) + i + k * 3.0f;
          m_.set(p.x + sh.w / 2 + (int)lroundf(cosf(a) * (sh.w * 0.5f + 1)), p.y - 2 + (int)lroundf(sinf(a * 1.3f) * 2.0f), SH_INK);
        }
      }
    }
    // rubbing: every dot the finger travels counts against the poop nearest it; suds pile up where it goes
    int dx, dy;
    if (in.down && inMatrix(in.x, in.y, dx, dy)) {
      if (S.had) {
        const int d0 = abs(dx - S.lx) + abs(dy - S.ly);
        const int d = d0 > 4 ? 4 : d0;   // (a jump, like a second finger, doesn't count for more)
        S.prog += d;
        int best = -1; float bd = 1e9f;
        for (int i = 0; i < S.np; i++) {
          const auto& p = S.poo[i];
          if (p.hp <= 0) continue;
          const PooShape& sh = POO_SHAPES[p.shape];
          const float ex = p.x + sh.w * 0.5f - dx, ey = p.y + sh.h * 0.5f - dy, dd = ex * ex + ey * ey;
          if (dd < bd) { bd = dd; best = i; }
        }
        if (best >= 0) {
          S.poo[best].hp -= d;
          if (S.poo[best].hp <= 0) sound(SND_BUBBLE);   // that one's gone
        }
        for (int j = 0; j < d && j < 3; j++) {
          auto& u = S.suds[S.ns++ % 48];
          u.x = (int8_t)clampi(dx + (int)(pet.rnd() % 7) - 3, 0, MW - 3);
          u.y = (int8_t)clampi(dy + (int)(pet.rnd() % 7) - 3, 0, MH - 3);
          u.k = (uint8_t)(pet.rnd() % 3);
          u.born = S.t;
        }
        if (d && ((int)(S.t * 6) != (int)((S.t - dt) * 6))) sound(SND_SCRUB);
      }
      S.had = true; S.lx = dx; S.ly = dy;
    } else S.had = false;
    if (S.prog <= 0 && ((int)(anim_ * 2) & 1)) m_.smallCenter(1, "SCRUB!");
    bool left = false;
    for (int i = 0; i < S.np; i++) if (S.poo[i].hp > 0) left = true;
    if (!left) {   // clean!
      S.done = true; S.doneT = 0;
      const int n = S.box + S.floor;
      // (poops done while this scrub was on screen aren't in it: they stay for the next one)
      pet.scoop(S.box);
      for (int i = 0; i < S.floor; i++) pet.cleanFloor();
      if (pet.d.evolving) pet.train(ACC_CLEAN, 0.1f * n);
      sound(SND_SPARKLE);
    }
  } else {
    S.doneT += dt;
    if (S.doneT > 0.4f) {
      m_.textCenter(9, "CLEAN!");
      if ((int)(anim_ * 6) & 1) { m_.sprite(3, 4, SPR_SPARK); m_.sprite(34, 18, SPR_SPARK); m_.sprite(30, 2, SPR_SPARK); m_.sprite(6, 20, SPR_SPARK); }
    }
    if (S.doneT > 1.5f || (S.doneT > 0.6f && in.tap)) { setScreen(S_MAIN); return; }
  }
  // the suds: each lasts under half a second, thinning out at the end (so they trail the finger instead of piling
  // up); any still there when it's clean pop away
  constexpr float SUDS_SECS = 0.3f, SUDS_FADE = 0.15f;   // full for this long, then faded for this long
  const int ns = S.ns < 48 ? S.ns : 48;
  for (int i = 0; i < ns; i++) {
    const auto& u = S.suds[i];
    const float age = S.t - u.born;
    if (age > SUDS_SECS + SUDS_FADE) continue;
    if (age > SUDS_SECS) { m_.set(u.x, u.y, SH_MID); m_.set(u.x + 2, u.y + 2, SH_MID); continue; }
    if (S.done) {   // each one pops in turn
      const float pt = S.doneT * 3.0f - (i % 8) * 0.12f;
      if (pt > 1.0f) continue;
      if (pt > 0.5f) { m_.set(u.x, u.y, SH_MID); m_.set(u.x + 2, u.y + 2, SH_MID); continue; }
    }
    if (u.k == 0) { m_.set(u.x + 1, u.y, SH_INK); m_.set(u.x, u.y + 1, SH_INK); m_.set(u.x + 2, u.y + 1, SH_INK); m_.set(u.x + 1, u.y + 2, SH_INK); m_.set(u.x + 1, u.y + 1, 0); }
    else if (u.k == 1) { m_.rect(u.x, u.y, 2, 2, SH_MID); }
    else m_.set(u.x + 1, u.y + 1, SH_INK);
  }
}

// ---------------------------------------------------------------------------------------------
// YEET: the fella sits in a slingshot at the left. Press anywhere on the screen and drag back to stretch the band
// (the dotted line shows the way they'll go), let go and they fly. Out past the edge of the screen there's stuff
// you can't see coming: trampolines, springs, bumpers and gusts help; rocks, birds, clouds and mud don't. The
// score is how far they end up (metres: 2 dots = 1 m). Physics is the same for every look in a life stage
// (the Big Blorbus are heavy: they plough through birds and smash rocks); each look has its own way of flying.
// ---------------------------------------------------------------------------------------------
namespace {
constexpr float GY = 27.0f;                 // the ground's surface (the bottom dot row is the ground)
inline float seatX(int w) { return w * 0.5f + 9.5f; }   // where the fella sits in the sling (its middle): room behind to pull all the way back...
// ...and how high: room below to pull all the way down, whatever their size
inline float seatY(int h) { const float a = GY - 10.0f, b = GY - h * 0.5f - 9.5f; return a < b ? a : b; }
constexpr float DOTS_PER_M = 2.0f;
constexpr float YEET_DRAG = 0.4f;          // dots the band stretches per dot the finger moves
struct YeetPhys { float k, g, drag, rest, fric; bool heavy; };
// k: launch speed per dot of pull; g: gravity; drag: air drag; rest: bounce off the ground; fric: rolling
// friction. Light ones fly fast and bounce a lot; heavy ones go less far in the air and roll on further.
const YeetPhys PHYS[5] = {
  {7.3f, 36.0f, 0.12f, 0.62f, 22.0f, false},   // Lil Blob: light and bouncy
  {7.3f, 38.0f, 0.10f, 0.56f, 22.0f, false},   // Lil Blub
  {7.5f, 40.0f, 0.08f, 0.50f, 24.0f, false},   // Lil Blunkus
  {7.7f, 42.0f, 0.06f, 0.44f, 24.0f, false},   // Lil Fella
  {7.3f, 48.0f, 0.03f, 0.28f, 9.0f, true},     // Big Blorbus: heavy, barely bounces, rolls a long way
};
enum ObKind : uint8_t { OB_TRAMP, OB_ROCK, OB_BIRD, OB_CLOUD, OB_GUST, OB_MUD, OB_SPRING, OB_BUMPER, OB_KINDS };
const uint8_t OB_WEIGHT[OB_KINDS] = {3, 4, 4, 3, 2, 3, 2, 3};
// how each look flies, lands, and what it shouts when it goes
enum Fly : uint8_t { FL_SPIN, FL_SPINB, FL_STRETCH, FL_FLAT, FL_FLIP, FL_WOBBLE, FL_STIFF, FL_ROLL, FL_FLAIL, FL_PUFF };
enum Trail : uint8_t { TR_NONE, TR_GOO, TR_STAR, TR_CRUMB, TR_LEAF, TR_BUBBLE, TR_LINES, TR_HEART, TR_SPARK, TR_PUFF };
enum Land : uint8_t { LD_SPLAT, LD_DIZZY, LD_FACE, LD_POSE, LD_PUDDLE, LD_SPINOUT, LD_HOP };
struct Style { uint8_t fly, trail, land; const char* cry; };
const Style STYLE[F_COUNT] = {
  {FL_STRETCH, TR_GOO, LD_SPLAT, "WHEE!"},      // (the dropper: never plays)
  {FL_STRETCH, TR_GOO, LD_SPLAT, "WHEE!"},      // Lil Blob: a teardrop of goo
  {FL_SPIN, TR_STAR, LD_DIZZY, "WOO!"},         // Blobbert: round, so it spins
  {FL_FLAT, TR_PUFF, LD_HOP, "MOCHI!"},         // Mochi: a frisbee
  {FL_PUFF, TR_BUBBLE, LD_POSE, "PFFT!"},       // Cheeks: puffs up
  {FL_FLIP, TR_CRUMB, LD_FACE, "BEAN!"},        // Bean: somersaults, face-plants
  {FL_SPINB, TR_SPARK, LD_SPLAT, "BUN!"},       // Bun
  {FL_ROLL, TR_LINES, LD_DIZZY, "CURL!"},       // Curly: rolls like a wheel
  {FL_STIFF, TR_LINES, LD_POSE, "HUZZAH!"},     // Dapper: stiff as a board, then a bow
  {FL_STIFF, TR_CRUMB, LD_HOP, "BREAD!"},       // Loaf
  {FL_FLAT, TR_GOO, LD_PUDDLE, "SPLOOSH"},      // Puddle
  {FL_WOBBLE, TR_BUBBLE, LD_HOP, "WIBBLE"},     // Jelly
  {FL_FLAIL, TR_LEAF, LD_FACE, "SPROING"},      // Sprout: leaves everywhere
  {FL_SPIN, TR_LINES, LD_SPINOUT, "SPIKE!"},    // SpikyBoi
  {FL_PUFF, TR_PUFF, LD_SPLAT, "POOF!"},        // Puff
  {FL_WOBBLE, TR_GOO, LD_PUDDLE, "SLOSH!"},     // Slosh
  {FL_WOBBLE, TR_STAR, LD_DIZZY, "WOBBLE"},     // Wobbles
  {FL_PUFF, TR_HEART, LD_HOP, "MALLOW"},        // Mallow
  {FL_SPINB, TR_CRUMB, LD_FACE, "TOAST!"},      // Toasty
  {FL_STRETCH, TR_HEART, LD_HOP, "MARSH!"},     // Marshy
  {FL_FLAIL, TR_STAR, LD_POSE, "BOOP!"},        // Boop
  {FL_FLAIL, TR_HEART, LD_DIZZY, "FUBZ!"},      // Fubz
  {FL_STIFF, TR_PUFF, LD_SPINOUT, "ZOOM!"},     // Nimbus: a cloud with places to be
  {FL_ROLL, TR_SPARK, LD_POSE, "ROLLIN"},       // Ballboi
  {FL_WOBBLE, TR_LINES, LD_SPLAT, "BLORB!"},    // Big Blorbus Blobbert
  {FL_ROLL, TR_STAR, LD_SPINOUT, "WHEEE"},      // Big Blorbus Ballboi
  {FL_SPINB, TR_STAR, LD_POSE, "MEOW!"},        // Big Blorbus Alien Cat
};
int obW(uint8_t k) { return k == OB_GUST ? 14 : k == OB_MUD ? 8 : k == OB_CLOUD ? 8 : k == OB_TRAMP ? 7 : k == OB_ROCK ? 6 : 5; }
int obH(uint8_t k) { return k == OB_GUST ? 7 : k == OB_MUD ? 2 : k == OB_CLOUD ? 4 : k == OB_TRAMP ? 3 : k == OB_ROCK ? 4 : k == OB_BIRD ? 3 : k == OB_SPRING ? 6 : 5; }
bool obInAir(uint8_t k) { return k == OB_BIRD || k == OB_CLOUD || k == OB_GUST || k == OB_BUMPER; }
// a line of dots (the sling's band)
void line(Matrix& m, int x0, int y0, int x1, int y1, uint8_t sh) {
  int dx = abs(x1 - x0), dy = -abs(y1 - y0), sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1, e = dx + dy;
  for (int guard = 0; guard < 200; guard++) {
    m.set(x0, y0, sh);
    if (x0 == x1 && y0 == y1) break;
    const int e2 = 2 * e;
    if (e2 >= dy) { e += dy; x0 += sx; }
    if (e2 <= dx) { e += dx; y0 += sy; }
  }
}
}  // namespace

void Game::startYeet() {
  memset(&yt_, 0, sizeof yt_);
  yt_.x = seatX(spriteW(gameSprite())); yt_.y = seatY(spriteH(gameSprite())); yt_.nextOb = 46.0f;   // (the first thing is just past the edge of the screen)
  yt_.lastHit = -1; yt_.gyScr = -1;   // (nothing drawn yet)
  selIcon_ = IC_PLAY;
  setScreen(S_YEET);
}

void Game::yeetLaunch(float pullX, float pullY) {
  Yeet& Y = yt_;
  const YeetPhys& P = PHYS[stageOfForm(form())];
  const float len = sqrtf(pullX * pullX + pullY * pullY);
  if (len > YEET_PULL) { pullX *= YEET_PULL / len; pullY *= YEET_PULL / len; }
  if (pullX > 0) pullX = 0;   // (the band only pulls back)
  const int id = gameSprite();
  const float sy = seatY(spriteH(id));
  if (sy + pullY + spriteH(id) * 0.5f > GY) pullY = GY - spriteH(id) * 0.5f - sy;   // (not down through the ground)
  Y.x = seatX(spriteW(id)) + pullX; Y.y = sy + pullY;
  Y.vx = -pullX * P.k; Y.vy = -pullY * P.k;
  Y.phase = 2; Y.t = 0; Y.aiming = false; Y.maxX = Y.x; Y.splatDone = false;
  sound(SND_WHOOSH);
}

// the fella drawn turned (rot: quarter turns clockwise) and stretched (sx, sy), centred on (cx, cy) in screen dots
void Game::drawYeetFella(float cx, float cy, int rot, float sx, float sy, bool flip, int mode) {
  const int id = gameSprite();
  const Sprite& sp = SPRITES[id];
  const int w = sp.w, h = sp.h;
  const int W2 = (int)lroundf(w * sx) < 1 ? 1 : (int)lroundf(w * sx), H2 = (int)lroundf(h * sy) < 1 ? 1 : (int)lroundf(h * sy);
  rot &= 3;
  const int RW = rot & 1 ? H2 : W2, RH = rot & 1 ? W2 : H2;
  const int x0 = (int)lroundf(cx - RW / 2.0f), y0 = (int)lroundf(cy - RH / 2.0f);
  // Which sprite row each drawn row shows. End to end: the first and last drawn rows are always the sprite's first
  // and last. (Plain v * h / H2 skipped the sprite's last rows whenever it was drawn shorter than it is: a Lil Blob
  // stretched out in flight, or anyone squashed on a bounce, lost the bottom of its outline.) And when rows have to
  // go, the eyes aren't among them: a drawn row next to where they'd be shows them instead.
  int8_t rowOf[64];
  const int HR = H2 < 64 ? H2 : 64;
  for (int v = 0; v < HR; v++) rowOf[v] = (int8_t)(H2 > 1 ? (v * (h - 1) * 2 + (H2 - 1)) / ((H2 - 1) * 2) : h - 1);
  if (HR >= 3 && HR < h) {
    for (int er = 1; er < h - 1; er++) {
      bool eyes = false, shown = false;
      for (int c = 0; c < w; c++) if (sp.px[er * w + c] == 2) eyes = true;
      if (!eyes) continue;
      int best = 1;
      for (int v = 0; v < HR; v++) { if (rowOf[v] == er) shown = true; if (v >= 1 && v <= HR - 2 && abs(rowOf[v] - er) < abs(rowOf[best] - er)) best = v; }
      if (!shown) rowOf[best] = (int8_t)er;
    }
  }
  for (int ry = 0; ry < RH; ry++)
    for (int rx = 0; rx < RW; rx++) {
      int u, v;
      switch (rot) {
        case 1: u = ry; v = H2 - 1 - rx; break;
        case 2: u = W2 - 1 - rx; v = H2 - 1 - ry; break;
        case 3: u = W2 - 1 - ry; v = rx; break;
        default: u = rx; v = ry; break;
      }
      int c = W2 > 1 ? (u * (w - 1) * 2 + (W2 - 1)) / ((W2 - 1) * 2) : (w - 1) / 2;   // (columns end to end too)
      const int r = rowOf[v < HR ? v : HR - 1];
      if (flip) c = w - 1 - c;
      const uint8_t p = sp.px[r * w + c];
      if (!p) continue;
      if (p == 2 && mode == 1 && r + 1 < h && sp.px[(r + 1) * w + c] == 2) continue;   // eyes squeezed shut
      m_.set(x0 + rx, y0 + ry, p == 4 ? SH_MID : SH_INK);
    }
}

void Game::yeetStep(float dtReal) {
  Yeet& Y = yt_;
  if (Y.phase != 2) return;
  const int stage = stageOfForm(form());
  const YeetPhys& P = PHYS[stage];
  const int id = gameSprite();
  const int sw = spriteW(id), sh = spriteH(id);
  // more things out past the edge of the screen
  while (Y.nextOb < Y.camX + 90.0f) {
    int slot = -1;
    for (int i = 0; i < 10; i++) if (!Y.ob[i].on) { slot = i; break; }
    if (slot < 0) break;
    int total = 0; for (int k = 0; k < OB_KINDS; k++) total += OB_WEIGHT[k];
    int r = (int)(pet.rnd() % (uint32_t)total), kind = 0;
    while (r >= OB_WEIGHT[kind]) { r -= OB_WEIGHT[kind]; kind++; }
    YeetOb& o = Y.ob[slot];
    o.on = true; o.kind = (uint8_t)kind; o.t = 0; o.x = Y.nextOb;
    o.y = obInAir(o.kind) ? GY - obH(o.kind) - pet.rndRange(7.0f, 22.0f) : GY - obH(o.kind);
    if (o.kind == OB_MUD) o.y = GY - 1;   // (mud lies in the ground)
    Y.nextOb += pet.rndRange(22.0f, 46.0f);
  }
  const int steps = 4;
  const float h = dtReal / steps;
  for (int s = 0; s < steps; s++) {
    Y.t += h;
    if (Y.hitT > 0) Y.hitT -= h;
    if (Y.squash > 0) Y.squash -= h;
    const bool odd = (Y.rot & 1) != 0;
    const float hx = (odd ? sh : sw) * 0.5f, hy = (odd ? sw : sh) * 0.5f;
    const float px = Y.x, py = Y.y;
    const bool tired = Y.t > 18.0f;   // (a very long flight: everything stops helping and they come down to rest)
    Y.vy += P.g * h;
    Y.vx -= Y.vx * (P.drag + (tired ? 3.0f : 0.0f)) * h;
    Y.vy -= Y.vy * P.drag * 0.5f * h;
    Y.x += Y.vx * h; Y.y += Y.vy * h;
    // the back wall (the sling's post)
    if (Y.x - hx < 0) { Y.x = hx; Y.vx = fabsf(Y.vx) * 0.5f; }
    Y.onGround = false; Y.inMud = false;
    // things out there
    for (int i = 0; i < 10; i++) {
      YeetOb& o = Y.ob[i];
      if (!o.on) continue;
      if (o.kind == OB_BIRD) { o.x -= 5.0f * h; o.t += h; }
      if (o.x < Y.camX - 40.0f) { o.on = false; continue; }
      const float ow = (float)obW(o.kind), oh = (float)obH(o.kind);
      const bool over = Y.x + hx - 0.5f > o.x && Y.x - hx + 0.5f < o.x + ow && Y.y + hy - 0.5f > o.y && Y.y - hy + 0.5f < o.y + oh;
      const bool fresh = !(Y.lastHit == i && Y.hitT > 0);
      if (tired && o.kind != OB_ROCK && o.kind != OB_MUD) continue;
      switch (o.kind) {
        case OB_TRAMP:
          if (over && Y.vy > 0 && py + hy <= o.y + 1.5f) { Y.y = o.y - hy; Y.vy = -(fabsf(Y.vy) * 0.8f + 26.0f); Y.vx += 4.0f; Y.squash = 0.15f; if (fresh) sound(SND_BOING); Y.lastHit = i; Y.hitT = 0.25f; }
          break;
        case OB_SPRING:
          if (over && fresh) { Y.y = o.y - hy; Y.vy = -40.0f; Y.vx = (Y.vx > 18.0f ? Y.vx : 18.0f) + 6.0f; sound(SND_BOING); Y.lastHit = i; Y.hitT = 0.3f; o.t = 0.3f; }
          break;
        case OB_ROCK: {
          const bool hov = Y.x + hx - 0.5f > o.x && Y.x - hx + 0.5f < o.x + ow;
          const bool onTop = hov && Y.vy >= 0 && py + hy <= o.y + 1.0f && Y.y + hy >= o.y - 0.6f;   // (resting on it counts, not just landing)
          if (!over && !onTop) break;
          if (P.heavy && fabsf(Y.vx) > 26.0f) { o.on = false; Y.vx *= 0.75f; sound(SND_POOF); break; }   // smashed
          if (onTop) {   // landed on top
            Y.y = o.y - hy;
            if (Y.vy > 10.0f) { Y.vy = -Y.vy * P.rest; Y.squash = 0.12f; } else { Y.vy = 0; Y.onGround = true; }
          } else if (px < o.x + ow * 0.5f) { Y.x = o.x - hx; Y.vx = -fabsf(Y.vx) * 0.45f; if (fresh) sound(SND_GRUMBLE); Y.lastHit = i; Y.hitT = 0.3f; }
          else { Y.x = o.x + ow + hx; Y.vx = fabsf(Y.vx) * 0.45f; }
          break;
        }
        case OB_BIRD:
          if (over && fresh) {
            o.on = false;
            if (P.heavy) Y.vx *= 0.9f;
            else { Y.vx *= 0.55f; Y.vy = (Y.vy > 0 ? Y.vy : 0) + 18.0f; }
            sound(SND_LOSE);
            for (int k = 0; k < 16; k++) if (Y.pt[k].life <= 0) { Y.pt[k] = {o.x + 2, o.y + 1, pet.rndRange(-8, 8), pet.rndRange(-10, 2), 0.7f, 9}; }
          }
          break;
        case OB_CLOUD:
          if (!over) break;
          if (Y.vy > 0 && py + hy <= o.y + 1.5f) { Y.y = o.y - hy; Y.vy = -(fabsf(Y.vy) * 0.5f > 14.0f ? fabsf(Y.vy) * 0.5f : 14.0f); if (fresh) sound(SND_BLOW); Y.lastHit = i; Y.hitT = 0.25f; }
          else { Y.vx -= Y.vx * 2.5f * h; Y.vy -= Y.vy * 2.0f * h; }   // soft: slows you right down
          break;
        case OB_GUST:
          if (over) { Y.vx += 45.0f * h; Y.vy -= 30.0f * h; if (Y.gustT <= 0) sound(SND_BLOW); Y.gustT = 0.5f; }
          break;
        case OB_BUMPER: {
          const float cx = o.x + 2.5f, cy = o.y + 2.5f, R = 2.5f + (hx < hy ? hx : hy) * 0.8f;
          const float ddx = Y.x - cx, ddy = Y.y - cy, d = sqrtf(ddx * ddx + ddy * ddy);
          if (d < R && d > 0.01f) {
            const float nx = ddx / d, ny = ddy / d;
            Y.x = cx + nx * R; Y.y = cy + ny * R;
            const float vn = Y.vx * nx + Y.vy * ny;
            if (vn < 0) { Y.vx -= 2 * vn * nx; Y.vy -= 2 * vn * ny; }
            Y.vx *= 1.1f; Y.vy *= 1.1f;
            const float sp = sqrtf(Y.vx * Y.vx + Y.vy * Y.vy);
            if (sp < 30.0f && sp > 0.01f) { Y.vx *= 30.0f / sp; Y.vy *= 30.0f / sp; }
            if (fresh) sound(SND_COIN);
            Y.lastHit = i; Y.hitT = 0.2f; o.t = 0.25f;
          }
          break;
        }
        case OB_MUD:
          if (Y.x > o.x && Y.x < o.x + ow && Y.y + hy >= GY - 0.6f) Y.inMud = true;
          break;
        default: break;
      }
    }
    if (Y.gustT > 0) Y.gustT -= h;
    // the ground
    if (Y.y + hy >= GY) {
      Y.y = GY - hy;
      if (Y.vy > 10.0f && !Y.inMud) { Y.vy = -Y.vy * P.rest; Y.squash = 0.15f; sound(SND_BOUNCE); }
      else if (Y.vy >= 0) { if (!Y.splatDone) { Y.splatDone = true; sound(SND_SPLAT); } Y.vy = 0; Y.onGround = true; }   // (on their way up off the ground: leave them be)
    }
    if (Y.onGround) {   // rolling to a stop (mud stops them dead)
      const float dec = P.fric * (Y.inMud ? 9.0f : 1.0f) * h;
      if (fabsf(Y.vx) <= dec) Y.vx = 0; else Y.vx -= Y.vx > 0 ? dec : -dec;
    }
    if (Y.x > Y.maxX) Y.maxX = Y.x;
  }
  // spinning: how fast depends on how fast they're going (rolling looks turn with the ground)
  const Style& st = STYLE[form() < F_COUNT ? form() : 0];
  const float speed = sqrtf(Y.vx * Y.vx + Y.vy * Y.vy);
  if (st.fly == FL_ROLL) Y.spin += Y.vx * dtReal / (sw * 0.8f) * 4.0f / 3.14159f;
  else if (st.fly == FL_SPIN || st.fly == FL_SPINB) { if (!Y.onGround || fabsf(Y.vx) > 5) Y.spin += dtReal * speed * 0.12f; }
  // a trail behind them
  if (st.trail != TR_NONE && !Y.onGround && speed > 8.0f && ((int)(Y.t * 12) != (int)((Y.t - dtReal) * 12))) {
    for (int k = 0; k < 16; k++) if (Y.pt[k].life <= 0) {
      Y.pt[k] = {Y.x - (Y.vx > 0 ? sw * 0.5f : -sw * 0.5f), Y.y + pet.rndRange(-2, 2), -Y.vx * 0.1f, st.trail == TR_GOO ? 6.0f : st.trail == TR_LEAF ? 3.0f : -1.0f, 0.6f, st.trail};
      break;
    }
  }
  for (int k = 0; k < 16; k++) if (Y.pt[k].life > 0) { auto& q = Y.pt[k]; q.life -= dtReal; q.x += q.vx * dtReal; q.y += q.vy * dtReal; if (q.k == TR_GOO || q.k == 9) q.vy += 30.0f * dtReal; }
  // the camera follows (up too, when they go high)
  const float tx = Y.x - 13.0f > 0 ? Y.x - 13.0f : 0;
  Y.camX += (tx - Y.camX) * clampf_(dtReal * 7.0f, 0, 1);
  const bool odd = (Y.rot & 1) != 0;
  const float top = Y.y - (odd ? sw : sh) * 0.5f;
  const float ty = top < 3.0f ? top - 3.0f : 0.0f;
  Y.camY += (ty - Y.camY) * clampf_(dtReal * 7.0f, 0, 1);
  // ...but never so far up that their feet leave the bottom of the screen: coming down fast, the camera used to
  // lag behind and the ground (and their bottom rows) were still off the screen when they landed
  {
    const float tall = (sw > sh ? sw : sh) * 0.75f + 1.0f;        // half their height, with room for a stretch or a turn
    const float lo = Y.y + tall - GY;                              // the camera may not be above this
    if (Y.camY < lo) Y.camY = lo < 0 ? lo : 0;
  }
  Y.meters = (int)((Y.x - seatX(sw)) / DOTS_PER_M + 0.5f);
  if (Y.meters < 0) Y.meters = 0;
  // stopped?
  if (Y.onGround && fabsf(Y.vx) < 0.8f) Y.rest += dtReal; else Y.rest = 0;
  if (Y.rest > 0.5f || Y.t > 30.0f) {
    Y.phase = 3; Y.landT = 0;
    // the score
    const int m = Y.meters;
    Y.hearts = m >= 150 ? 4 : m >= 100 ? 3 : m >= 60 ? 2 : m >= 30 ? 1 : 0;
    pet.exercise(4.0f + 5.0f * Y.hearts, 0.3f + 0.2f * Y.hearts);
    if (pet.d.evolving) pet.train(ACC_STRENGTH, 0.1f * Y.hearts);
    sv.lifeGames++;
    if (m > sv.yeetBest) { Y.best = sv.yeetBest > 0 || m > 0; sv.yeetBest = (uint16_t)(m > 65535 ? 65535 : m); }
  }
}

void Game::screenYeet() {
  Yeet& Y = yt_;
  int ic;
  if (iconTapped(ic)) {
    if (ic == IC_PLAY) { sound(SND_BACK); setScreen(S_MAIN); }
    else { setScreen(S_MAIN); doIcon(ic); }
    return;
  }
  char b[16];
  const Form f = form();
  const Style& st = STYLE[f < F_COUNT ? f : 0];
  if (Y.phase == 0) {   // the card: how to play, and the best so far
    m_.smallCenter(1, "PULL BACK");
    m_.smallCenter(7, "AND RELEASE");
    m_.smallCenter(13, "TO YEET!");
    if (sv.yeetBest) snprintf(b, sizeof b, "BEST %uM", (unsigned)sv.yeetBest); else snprintf(b, sizeof b, "BEST -");
    m_.smallCenter(22, b, SH_MID);
    Y.t += dt;
    if (Y.t > 0.3f && in.tap) { Y.phase = 1; Y.t = 0; sound(SND_TAP); }
    return;
  }
  const int id = gameSprite();
  const int sw = spriteW(id), sh = spriteH(id);
  int dx, dy;
  if (Y.phase == 1) {   // aiming: press anywhere and drag back
    Y.t += dt;
    if (in.pressed && inMatrix(in.px, in.py, dx, dy) && Y.t > 0.2f) Y.aiming = true;
    if (Y.aiming && in.down) {   // the band gives a little: a finger dragged ~15 mm pulls it all the way
      float pxv = (in.x - in.px) * YEET_DRAG / DOT, pyv = (in.y - in.py) * YEET_DRAG / DOT;
      if (pxv > 0) pxv = 0;
      const float len = sqrtf(pxv * pxv + pyv * pyv);
      if (len > YEET_PULL) { pxv *= YEET_PULL / len; pyv *= YEET_PULL / len; }
      if (seatY(sh) + pyv + sh * 0.5f > GY) pyv = GY - sh * 0.5f - seatY(sh);   // (not down through the ground)
      if ((int)(sqrtf(pxv * pxv + pyv * pyv) / 3.0f) != (int)(sqrtf(Y.pullX * Y.pullX + Y.pullY * Y.pullY) / 3.0f)) sound(SND_STRETCH);   // the band creaks as it gives
      Y.pullX = pxv; Y.pullY = pyv;
    }
    if (Y.aiming && !in.down) {   // let go
      Y.aiming = false;
      if (sqrtf(Y.pullX * Y.pullX + Y.pullY * Y.pullY) >= 2.0f) yeetLaunch(Y.pullX, Y.pullY);
      Y.pullX = Y.pullY = 0;
    }
  } else if (Y.phase == 2) yeetStep(dt);
  else Y.landT += dt;
  if (Y.phase == 4) {   // the result, on a clear screen
    snprintf(b, sizeof b, "%dM", Y.meters);
    m_.textCenter(2, b);
    yHearts(m_, 11, Y.hearts);
    if (Y.best) m_.smallCenter(21, ((int)(anim_ * 3) & 1) ? "NEW BEST!" : "", SH_INK);
    else { snprintf(b, sizeof b, "BEST %uM", (unsigned)sv.yeetBest); m_.smallCenter(21, b, SH_MID); }
    if (Y.landT > 0.6f && in.tap) setScreen(S_MAIN);
    return;
  }
  if (Y.phase == 3 && Y.landT > 1.8f) { Y.phase = 4; Y.landT = 0; sound(Y.hearts >= 3 ? SND_WIN : Y.hearts ? SND_COO : SND_SAD); return; }
  // ---- the world ----
  const float cx0 = Y.camX, cy0 = Y.camY;
  auto SXo = [&](float wx) { return (int)lroundf(wx - cx0); };
  auto SYo = [&](float wy) { return (int)lroundf(wy - cy0); };
  // the ground: a dotted line that scrolls by, and a marker every 20 m
  const int gy = SYo(GY);
  if (gy >= 0 && gy < MH) for (int x = 0; x < MW; x++) if (((int)floorf(x + cx0)) % 4 != 0) m_.set(x, gy, SH_MID);
  for (int mk = 20; mk < 2000; mk += 20) {
    const int x = SXo(seatX(sw) + mk * DOTS_PER_M);
    if (x < -8) continue;
    if (x > MW) break;
    m_.sprite(x, gy - 3, SPR_Y_POST, false, 0, 0, SH_MID);
    snprintf(b, sizeof b, "%d", mk);
    m_.small(x + 2, gy - 8, b, SH_FAINT);
  }
  // the sling
  const int slx = SXo(seatX(sw) - 2), sly = SYo(seatY(sh) + sh * 0.5f - 3);   // (the prongs cup their bottom)
  m_.sprite(slx, sly, SPR_Y_SLING);
  for (int y = sly + spriteH(SPR_Y_SLING); y < gy; y++) m_.set(slx + 2, y, SH_INK);   // (a taller post for a bigger fella)
  // things
  for (int i = 0; i < 10; i++) {
    const YeetOb& o = Y.ob[i];
    if (!o.on) continue;
    const int x = SXo(o.x), y = SYo(o.y);
    if (x > MW || x < -16) continue;
    switch (o.kind) {
      case OB_TRAMP: m_.sprite(x, y, SPR_Y_TRAMP); break;
      case OB_ROCK: m_.sprite(x, y, SPR_Y_ROCK); break;
      case OB_BIRD: m_.sprite(x, y, ((int)(o.t * 6) & 1) ? SPR_Y_BIRD1 : SPR_Y_BIRD2); break;
      case OB_CLOUD: m_.sprite(x, y, SPR_Y_CLOUD, false, 0, 0, SH_MID); break;
      case OB_MUD: m_.sprite(x, y, SPR_Y_MUD); break;
      case OB_SPRING: m_.sprite(x, y + (o.t > 0 && Y.hitT > 0 ? 1 : 0), SPR_Y_SPRING); break;
      case OB_BUMPER: m_.sprite(x, y, SPR_Y_BUMPER); if (Y.lastHit == i && Y.hitT > 0) { m_.set(x - 1, y + 2, SH_INK); m_.set(x + 5, y + 2, SH_INK); m_.set(x + 2, y - 1, SH_INK); m_.set(x + 2, y + 5, SH_INK); } break;
      case OB_GUST:   // wind: streaks blowing to the right
        for (int r = 0; r < 3; r++) {
          const int off = ((int)(anim_ * 20) + r * 5) % 14;
          for (int k = 0; k < 4; k++) m_.set(x + (off + k) % 14, y + 1 + r * 2, SH_MID);
        }
        break;
      default: break;
    }
  }
  // particles (the trail; feathers)
  for (int k = 0; k < 16; k++) {
    const auto& q = Y.pt[k];
    if (q.life <= 0) continue;
    const int x = SXo(q.x), y = SYo(q.y);
    switch (q.k) {
      case TR_STAR: case TR_SPARK: if (q.life > 0.3f || (q.k == TR_SPARK && ((int)(anim_ * 10) & 1))) m_.sprite(x - 1, y - 1, SPR_SPARK); else m_.set(x, y, SH_MID); break;
      case TR_BUBBLE: m_.set(x, y - 1, SH_MID); m_.set(x - 1, y, SH_MID); m_.set(x + 1, y, SH_MID); m_.set(x, y + 1, SH_MID); break;
      case TR_LINES: for (int j = 0; j < 3; j++) m_.set(x - j, y, SH_MID); break;
      case TR_LEAF: m_.set(x, y, SH_INK); m_.set(x + 1, y - 1, SH_MID); break;
      case TR_HEART: m_.set(x - 1, y, SH_INK); m_.set(x + 1, y, SH_INK); m_.set(x, y + 1, SH_INK); break;
      case TR_PUFF: m_.rect(x, y, 2, 2, SH_MID); break;
      case TR_GOO: m_.set(x, y, SH_MID); break;
      default: m_.set(x, y, SH_INK); break;   // crumbs, feathers
    }
  }
  // the fella
  float fx, fy;
  if (Y.phase == 1) { fx = seatX(sw) + Y.pullX; fy = seatY(sh) + Y.pullY; }
  else { fx = Y.x; fy = Y.y; }
  float sxs = 1, sys = 1;
  int rot = 0, mode = 0;
  bool flip = Y.phase >= 2 && Y.vx < -1.0f;
  float bob = 0;
  if (Y.phase == 2) {
    const float sp = sqrtf(Y.vx * Y.vx + Y.vy * Y.vy);
    const bool air = !Y.onGround;
    switch (st.fly) {
      case FL_SPIN: rot = (int)floorf(Y.spin) & 3; break;
      case FL_SPINB: rot = (-(int)floorf(Y.spin)) & 3; break;
      case FL_ROLL: rot = (int)floorf(Y.spin) & 3; break;
      case FL_STRETCH: if (air) { sxs = 1.0f + clampf_(sp / 110.0f, 0, sw < 10 ? 0.3f : 0.5f); sys = 1.0f / sxs; } break;   // (a tiny one stretches less: it would lose its face)
      case FL_FLAT: if (air) { sxs = 1.3f; sys = 0.65f + 0.1f * sinf(Y.t * 20.0f); } break;
      case FL_FLIP: if (air) rot = ((int)(Y.t * 4) & 1) * 2; break;
      case FL_WOBBLE: { const float k = 0.2f * sinf(Y.t * 14.0f); sxs = 1 + k; sys = 1 - k; break; }
      case FL_STIFF: mode = 1; break;
      case FL_FLAIL: if (air) { flip = (int)(Y.t * 7) & 1; bob = ((int)(Y.t * 10) & 1) ? -1.0f : 0.0f; } break;
      case FL_PUFF: if (air) { sxs = sys = 1.2f; } break;
      default: break;
    }
    if (Y.squash > 0) { sxs *= 1.3f; sys *= 0.7f; }
    Y.rot = rot;
  } else if (Y.phase == 3) {   // landed: their own landing
    const float t = Y.landT;
    switch (st.land) {
      case LD_SPLAT: { const float k = clampf_(1 - t / 0.8f, 0, 1); sxs = 1 + 0.5f * k; sys = 1 - 0.5f * k; mode = t < 0.8f; break; }
      case LD_DIZZY: {   // stars round the head
        const float a = t * 7.0f;
        for (int k = 0; k < 2; k++) m_.set(SXo(fx + cosf(a + k * 3.14f) * sw * 0.45f), SYo(fy - sh * 0.5f - 2 + sinf(a + k * 3.14f) * 1.2f), SH_INK);
        mode = 1; break;
      }
      case LD_FACE: if (t < 1.2f) { rot = 2; fy += 0; fx += ((int)(t * 12) & 1) ? 0.5f : -0.5f; } break;
      case LD_POSE: bob = -fabsf(sinf(t * 6.0f)) * 2.0f * clampf_(1 - t / 1.2f, 0, 1); if ((int)(t * 5) & 1) m_.sprite(SXo(fx + sw * 0.5f), SYo(fy - sh * 0.5f - 2), SPR_SPARK); break;
      case LD_PUDDLE: { const float k = t < 0.6f ? t / 0.6f : t < 1.2f ? 1 : clampf_(1 - (t - 1.2f) / 0.5f, 0, 1); sxs = 1 + 0.5f * k; sys = 1 - 0.65f * k; break; }
      case LD_SPINOUT: { const float a = t < 1.2f ? (1.2f * t - 0.5f * t * t) * 6.0f : 6.0f * 0.72f; rot = (int)a & 3; if (t > 1.2f) rot = 0; break; }
      case LD_HOP: bob = -fabsf(sinf(t * 9.0f)) * 2.5f * clampf_(1 - t / 1.5f, 0, 1); break;
      default: break;
    }
  }
  // never drawn into the ground: turned on its side, stretched or puffed up, the picture can be taller than the body
  // the physics knows about (that cut their bottom rows off). On the ground they stand on it; in the air, just above it.
  const bool odd = rot & 1;
  const int W2 = (int)lroundf(sw * sxs) < 1 ? 1 : (int)lroundf(sw * sxs), H2 = (int)lroundf(sh * sys) < 1 ? 1 : (int)lroundf(sh * sys);
  const float RH = (float)(odd ? W2 : H2);   // drawn height, as drawYeetFella works it out
  if (Y.phase >= 2 && (Y.onGround || Y.phase == 3) && fy + (odd ? sw : sh) * 0.5f >= GY - 0.6f) fy = GY - RH * 0.5f - bob;
  if (fy + bob + RH * 0.5f > GY) fy = GY - RH * 0.5f - bob;
  drawYeetFella(fx - cx0, fy - cy0 + bob, rot, sxs, sys, flip, mode);
  Y.drawBot = (int)lroundf(fy - cy0 + bob - RH * 0.5f) + (int)RH - 1; Y.gyScr = gy;   // (tests: the lowest row drawn, the ground's row)
  // the band, from the prongs to their back, while aiming (and the dotted line where they'll go)
  if (Y.phase == 1) {
    const int bx = SXo(fx - sw * 0.35f), by = SYo(fy);
    line(m_, slx, sly, bx, by, SH_MID);
    line(m_, slx + 4, sly, bx, by, SH_MID);
    if (Y.aiming && sqrtf(Y.pullX * Y.pullX + Y.pullY * Y.pullY) >= 2.0f) {
      const YeetPhys& P = PHYS[stageOfForm(f)];
      const float vx = -Y.pullX * P.k, vy = -Y.pullY * P.k;
      for (int k = 1; k <= 5; k++) {
        const float t = k * 0.09f;
        m_.set(SXo(fx + vx * t), SYo(fy + vy * t + 0.5f * P.g * t * t), SH_INK);
      }
    }
    if (!Y.aiming && ((int)(anim_ * 2) & 1) && Y.t < 6.0f) { m_.rect(MW - 21, 0, 21, 7, 0); m_.small(MW - 19, 1, "PULL!", SH_MID); }
  }
  // the shout as they go, then how far so far
  // (the words get a clear patch behind them so they read over whatever's flying past)
  if (Y.phase == 2 && Y.t < 0.9f) { m_.rect(0, 0, Matrix::smallWidth(st.cry) + 2, 7, 0); m_.small(1, 1, st.cry); }
  if (Y.phase >= 2) { snprintf(b, sizeof b, "%dM", Y.meters); const int w = Matrix::smallWidth(b); m_.rect(MW - w - 1, 0, w + 1, 7, 0); m_.small(MW - w, 1, b); }
}

}  // namespace lf
