// One-of-a-kind eating routines. A line-for-line port of the Eating Lab (art/eating/engine.js): doubles and
// JavaScript's rounding (floor(x + 0.5)) throughout, so every frame matches the approved previews.
#include "eatfx.h"
#include "sprites.h"
#include "lcd.h"
#include <math.h>
#include <string.h>
#include <initializer_list>

namespace lf {
namespace eatfx {
namespace {

typedef double D;
constexpr int EW = 40, EH = 28;
static_assert(EW == MW && EH == MH, "the eating scenes are drawn on the matrix-sized frame");
constexpr D PI = 3.141592653589793;

uint8_t* B = nullptr;   // the 40x28 frame being drawn
bool MEAL = true;       // the big meal, else the snack
int MV = 0;             // Marshy's wall
bool INV = false;       // lightning flash

// ---- maths, exactly as the lab's
inline D jr(D x) { return floor(x + 0.5); }   // Math.round
inline long fl(D x) { return (long)floor(x); }
inline D md(D a, D b) { return fmod(a, b); }    // JS %
inline D cl(D v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }
inline D pr(D t, D a, D b) { return cl((t - a) / (b - a)); }
inline D ez(D k) { return k * k * (3 - 2 * k); }
inline D lerp(D a, D b, D k) { return a + (b - a) * k; }
inline D rnd(D i) { const D s = sin(i * 12.9898 + 4.1) * 43758.5453; return s - floor(s); }
inline D hop(D k, D n, D amp) { return jr(fabs(sin(k * n * PI)) * amp); }
inline D dropY(D t, D t0, D d, D y) { const D k = pr(t, t0, t0 + d); return jr(y - (1 - k) * (1 - k) * 22); }
inline D sgn(D x) { return x > 0 ? 1 : (x < 0 ? -1 : 0); }
inline D mx(D a, D b) { return a > b ? a : b; }
inline D mn(D a, D b) { return a < b ? a : b; }

inline void set(D x, D y, int v) {
  x = jr(x); y = jr(y);
  if (x >= 0 && x < EW && y >= 0 && y < EH) B[(int)y * EW + (int)x] = (uint8_t)v;
}

// ---- sprites: the game's, plus a few props only the routines use
enum { X_FORK = 200, X_KNIFE, X_BONE, X_SPIKY_HEAD, X_SPIKY_HEADO, X_SPIKY_BODY };
const char* const FORK_[] = {"X.X.X", "XXXXX", "..X..", "..X..", "..X.."};
const char* const KNIFE_[] = {".XX", ".XX", ".X.", ".X.", ".X."};
const char* const BONE_[] = {".X.X.", "XXXXX", ".X.X."};
struct Spr { int w, h; const uint8_t* px; const char* const* rows; };
Spr sp(int id) {
  switch (id) {
    case X_FORK: return {5, 5, nullptr, FORK_};
    case X_KNIFE: return {3, 5, nullptr, KNIFE_};
    case X_BONE: return {5, 3, nullptr, BONE_};
    case X_SPIKY_HEAD: { const Sprite& s = SPRITES[SPR_AD_DANGO]; return {s.w, 6, s.px, nullptr}; }
    case X_SPIKY_HEADO: { const Sprite& s = SPRITES[SPR_AD_DANGO_EAT]; return {s.w, 6, s.px, nullptr}; }
    case X_SPIKY_BODY: { const Sprite& s = SPRITES[SPR_AD_DANGO]; return {s.w, s.h - 6, s.px + 6 * s.w, nullptr}; }
    default: { const Sprite& s = SPRITES[id]; return {s.w, s.h, s.px, nullptr}; }
  }
}
inline char at(const Spr& s, int r, int c) {
  if (s.rows) return s.rows[r][c];
  switch (s.px[r * s.w + c]) { case 1: return 'X'; case 2: return 'E'; case 3: return 'M'; case 4: return 'G'; default: return '.'; }
}

// drawing options (the lab's `o` object)
struct PO {
  bool shut = false, mid = false, dark = false, noE = false, onlyE = false;
  int rmin = -1, rmax = -1;
  int dxk = 0; D a = 0, b = 0, c = 0;   // per-row offset: 1 = round(sin(a + r*b) * c) (Jelly), 2 = round((5 - r) * a * 0.2) (Bean)
  PO& S(bool v) { shut = v; return *this; }
  PO& Mi(bool v) { mid = v; return *this; }
  PO& Dk(bool v) { dark = v; return *this; }
  PO& R(int a_, int b_) { rmin = a_; rmax = b_; return *this; }
  PO& NoE() { noE = true; return *this; }
  PO& OnlyE() { onlyE = true; return *this; }
};
inline D dxOf(const PO& o, int r) {
  if (o.dxk == 1) return jr(sin(o.a + r * o.b) * o.c);
  if (o.dxk == 2) return jr((5 - r) * o.a * 0.2);
  return 0;
}

void put(int id, D x, D y, const PO& o = PO()) {
  const Spr s = sp(id);
  for (int r = 0; r < s.h; r++) {
    if (o.rmin >= 0 && (r < o.rmin || r > o.rmax)) continue;
    for (int c = 0; c < s.w; c++) {
      const char ch = at(s, r, c);
      if (o.noE && ch == 'E') continue;
      if (o.onlyE && ch != 'E') continue;
      int v = ch == 'X' || ch == 'M' ? 3 : ch == 'E' ? (o.shut ? 0 : 3) : ch == 'G' ? 2 : 0;
      if (!v) continue;
      if (o.mid && v == 3) v = 2;
      if (o.dark && v == 2) v = 3;
      set(x + c, y + r, v);
    }
  }
}
// stretched or squashed to w x h dots (end points map to end points)
void warp(int id, D x, D y, D w, D h, const PO& o = PO()) {
  w = mx(1, jr(w)); h = mx(1, jr(h));
  const Spr s = sp(id);
  for (D dy = 0; dy < h; dy++) for (D dx = 0; dx < w; dx++) {
    const D sx = w > 1 ? jr(dx * (s.w - 1) / (w - 1)) : 0, sy = h > 1 ? jr(dy * (s.h - 1) / (h - 1)) : 0;
    const char ch = at(s, (int)sy, (int)sx);
    int v = ch == 'X' || ch == 'M' ? 3 : ch == 'E' ? (o.shut ? 0 : 3) : ch == 'G' ? 2 : 0;
    if (!v) continue;
    if (o.mid && v == 3) v = 2;
    set(x + dx + dxOf(o, (int)dy), y + dy, v);
  }
}
struct FDs { D w, h; int n[3]; };
FDs FD() { return MEAL ? FDs{9, 5, {SPR_MEAL, SPR_MEAL2, SPR_MEAL3}} : FDs{5, 5, {SPR_SNACK, SPR_SNACK2, SPR_SNACK3}}; }
void food(D cx, D y, int stage, const PO& o = PO()) { const FDs f = FD(); if (stage >= 3) return; put(f.n[stage], jr(cx - f.w / 2), y, o); }
void spark(D x, D y) { put(SPR_SPARK, x, y); }
void bubble(D x, D y) { put(SPR_BUBBLE, x, y); }
void line(D x0, D y0, D x1, D y1, int v) { const D n = mx(mx(fabs(x1 - x0), fabs(y1 - y0)), 1); for (D i = 0; i <= n; i++) set(x0 + (x1 - x0) * i / n, y0 + (y1 - y0) * i / n, v); }
void disc(D cx, D cy, D r, int v) { for (D y = -r; y <= r; y++) for (D x = -r; x <= r; x++) if (x * x + y * y <= r * r + r * 0.5) set(cx + x, cy + y, v); }
void rect(D x, D y, D w, D h, int v) { for (D j = 0; j < h; j++) for (D i = 0; i < w; i++) set(x + i, y + j, v); }
void floorWarp(int id, D cx, D w, D h, D lift, const PO& o = PO()) { warp(id, jr(cx - w / 2), 28 - jr(h) - lift, w, h, o); }
// (the circle tests compare squared distances: the same dots as hypot() < r, without a square root per dot)
inline bool inRim(D d2, D r) { const D lo = r - 0.5, hi = r + 0.5; return (lo <= 0 || d2 >= lo * lo) && d2 < hi * hi; }
inline bool inside(D d2, D r) { const D lo = r - 0.5; return lo > 0 && d2 < lo * lo; }
void ring(D cx, D cy, D r, int v, int fill = -1) {
  for (D dy = -r - 1; dy <= r + 1; dy++) for (D dx = -r - 1; dx <= r + 1; dx++) {
    const D d2 = dx * dx + dy * dy;
    if (fill >= 0 && inside(d2, r)) set(cx + dx, cy + dy, fill);
    else if (inRim(d2, r)) set(cx + dx, cy + dy, v);
  }
}
void shiftBuf(int dx) {   // screen shake
  if (!dx) return;
  uint8_t c[EW * EH]; memcpy(c, B, sizeof c);
  for (int y = 0; y < EH; y++) for (int x = 0; x < EW; x++) { const int sx = x - dx; B[y * EW + x] = sx >= 0 && sx < EW ? c[y * EW + sx] : 0; }
}
int stepIn(D t, const D* ts, int n) { int i = 0; while (i < n && t >= ts[i]) i++; return i; }

// ---- the routines, one per look. (Wobbles' path helpers first.)
struct Pt { D x, y, hx, hy; };
Pt PTS[600]; int NP = 0; D PLT = 0;
void mkPath(D x0, D y0, D A1, D R, D al, D A2) {
  const D step = 0.25; NP = 0; D x = x0, y = y0; const D Lt = A1 + R * al + A2;
  for (D s = 0; s <= Lt + 0.001; s += step) {
    D hx, hy;
    if (s < A1) { hx = 0; hy = -1; }
    else if (s < A1 + R * al) { const D p = (s - A1) / R; hx = sin(p); hy = -cos(p); }
    else { hx = sin(al); hy = -cos(al); }
    if (NP < 600) PTS[NP++] = {x, y, hx, hy};
    x += hx * step; y += hy * step;
  }
  PLT = Lt;
}
Pt pAt(D a) { D i = jr(a / 0.25); if (i > NP - 1) i = NP - 1; if (i < 0) i = 0; return PTS[(int)i]; }

// WOBBLES: a slinky. Stretches up, arches over into an upside-down U, clamps the food with the top of his
// head; it travels the whole U and dissolves in his feet. Then he boings back.
void wobbles(D t) {
  const D R = 8.5, Bv = 27, X0 = 11, fcx = X0 + 2 * R;
  D lam = 0, Ls = 10;
  if (t < 0.7) { /* standing: Ls stays 10 */ }
  else if (t < 1.0) Ls = lerp(10, 8, ez(pr(t, 0.7, 1)));
  else if (t < 1.8) Ls = lerp(8, 25, ez(pr(t, 1, 1.8)));
  else if (t < 3.0) { Ls = 25; lam = ez(pr(t, 1.8, 3.0)); }
  else if (t < 5.6) { Ls = 25; lam = 1; }
  else if (t < 6.2) { Ls = 25; lam = 1 - ez(pr(t, 5.6, 6.2)); }
  else { const D u = t - 6.2; Ls = 10 + 15 * exp(-4.5 * u) * cos(9 * u); }
  const bool open = (t > 2.7 && t < 3.5) || (t > 3.4 && t < 5.0 && fl(t * 6) % 2 == 0);
  const bool shut = t > 3.0 && t < 6.4;
  if (t < 3.4) food(jr(fcx), dropY(t, 0, 0.45, 23), 0);
  const D A1 = lam > 0 ? lerp(25, 11, ez(cl(lam * 1.6))) : Ls, al = lam * PI, A2 = lam * 11;
  if (lam == 0 && Ls < 10.5) { const D h = mx(6, jr(Ls)); warp(open ? SPR_TN_WOBBLES_EAT : SPR_TN_WOBBLES, 5, 28 - h, 13, h, PO().S(shut)); }
  else {
    mkPath(X0, Bv, A1, R, al, A2);
    const D n = jr(PLT);
    static char lines[96][14]; int L = 0;
    auto row = [&](int id, int r) { const Spr s = sp(id); for (int c = 0; c < 13; c++) lines[L][c] = at(s, r, c); lines[L][13] = 0; L++; };
    const int idle = SPR_TN_WOBBLES, hd = open ? SPR_TN_WOBBLES_EAT : SPR_TN_WOBBLES;
    row(idle, 9); row(idle, 8);
    const D m = n - 8;
    if (m <= 2) { if (m >= 2) row(idle, 7); if (m >= 1) row(idle, 6); }
    else for (D i = 0; i < m && L < 88; i++) { memcpy(lines[L], fmod(i, 2) != 0 ? "............." : ".XXXXXXXXXXX.", 14); L++; }
    for (int r = 5; r >= 0; r--) row(hd, r);
    const bool curved = al > 0.05;
    for (int k = 0; k < L; k++) {
      const Pt p = pAt((k + 0.5) * PLT / L); const D nx = -p.hy, ny = p.hx;
      for (int c = 0; c < 13; c++) {
        const char ch = lines[k][c]; if (ch == '.') continue;
        if (ch == 'E' && shut) continue;
        const int v = ch == 'G' ? 2 : 3;
        set(p.x + nx * (c - 6), p.y + ny * (c - 6), v);
        if (curved && c < 12 && lines[k][c + 1] != '.') set(p.x + nx * (c - 5.5), p.y + ny * (c - 5.5), v);
      }
    }
    if (t >= 3.4 && t < 5.0) {
      const D f = ez(pr(t, 3.4, 5.0)); const Pt p = pAt(lerp(PLT - 3.5, 2.5, f));
      static const int o[5][2] = {{0, 0}, {1, 0}, {-1, 0}, {0, 1}, {0, -1}};
      for (const auto& d : o) set(p.x + d[0], p.y + d[1], d[0] || d[1] ? 2 : 3);
    }
  }
  if (t >= 5.0 && t < 5.7) for (int i = 0; i < 7; i++) { const D life = pr(t, 5.0 + i * 0.05, 5.7); if (life < 1) set(X0 - 4 + rnd(i) * 8, 26 - life * 6 * (0.5 + rnd(i + 9)), life < 0.5 ? 3 : 2); }
  if (t > 6.9 && fl(t * 5) % 2 == 0) spark(17, 16);
}

// JELLY: hops over, belly-flops on the food, springs back up with it inside; it floats up and fizzles; bubble burp.
void jelly(D t) {
  const D fcx = 28;
  if (t < 2.4) food(fcx, dropY(t, 0, 0.4, 23), 0);
  D left = 3, w = 14, h = 14, lift = 0;
  if (t >= 0.4 && t < 1.5) { const D k = pr(t, 0.4, 1.5); left = 3 + jr(k * 11); lift = hop(k, 3, 3); }
  else if (t >= 1.5 && t < 2.2) { const D k = pr(t, 1.5, 2.2); left = 14 + jr(ez(k) * 7); lift = jr(sin(k * PI) * 9); }
  else if (t >= 2.2 && t < 2.6) { const D k = ez(pr(t, 2.2, 2.6)); w = 14 + jr(10 * k); h = 14 - jr(10 * k); left = 28 - jr(w / 2); }
  else if (t >= 2.6) { const D k = pr(t, 2.6, 3.4); const D e = 1 - pow(1 - k, 2) * cos(k * 9); w = jr(lerp(24, 14, cl(k))); h = jr(lerp(4, 14, e)); left = 28 - jr(w / 2); }
  const bool swallowed = t >= 2.6 && t < 5.1, shut = swallowed;
  const D amp = t >= 2.6 && t < 5.8 ? 2.2 * exp(-(t - 2.6) * 0.7) : 0;
  const D y = 28 - h - lift;
  PO o; o.shut = shut; o.dxk = 1; o.a = t * 14; o.b = 0.8; o.c = amp;
  warp(t > 5.2 && t < 5.5 ? SPR_AD_JELLY_EAT : SPR_AD_JELLY, left, y, w, h, o);
  if (swallowed && h > 9) {
    const D k = pr(t, 3.3, 5.0), fy = jr(lerp(28 - 14 + 7, 28 - 14 + 2, ez(k))); const int st = k < 0.35 ? 0 : k < 0.7 ? 1 : 2;
    if (k < 1) food(28, fy + (14 - h), st, PO().Mi(true));
  }
  if (t > 5.0 && t < 5.9) { const D k = pr(t, 5.0, 5.9); bubble(jr(30 + sin(k * 6) * 2), jr(10 - k * 8)); if (k > 0.85) spark(30, 2); }
  if (t > 6.0 && fl(t * 4) % 2 == 0) spark(38, 9);
}

// PUDDLE: flows flat across the food, glugs it down in three gulps with bubbles, bunches into a dome, burps, slumps.
void puddle(D t) {
  const D fcx = 29;
  // The food never goes out of sight. It lies there whole until they flow over it; each dot of it goes soft a
  // moment after their edge has passed it, then fades and is gone, every dot in its own time.
  if (t < 0.45) food(fcx, dropY(t, 0, 0.45, 23), 0);
  else if (t < 3.6) {
    const FDs F = FD(); const Spr s = sp(F.n[0]); const D fx0 = jr(fcx - F.w / 2);
    const D edge = lerp(22, 38, ez(pr(t, 0.5, 1.7))), g = pr(t, 2.0, 3.5);
    for (int r = 0; r < s.h; r++) for (int c = 0; c < s.w; c++) {
      if (at(s, r, c) == '.') continue;
      const D go = 0.1 + rnd(r * 5 + c * 11 + 40) * 0.72;   // when this dot starts to go
      if (g >= go + 0.14) continue;
      set(fx0 + c, 23 + r, g >= go ? 1 : edge >= fx0 + c + 1 + rnd(r * 9 + c + 1) * 3 ? 2 : 3);
    }
  }
  D left = 2, w = 20, h = 9; bool shut = false;
  if (t >= 0.5 && t < 1.7) { const D k = ez(pr(t, 0.5, 1.7)), k2 = ez(pr(t, 0.9, 1.7)); const D right = lerp(22, 38, k); left = jr(lerp(2, 14, k2)); w = jr(right - left); h = jr(lerp(9, 4, k)); }
  else if (t >= 1.7 && t < 3.6) { left = 14; w = 24; h = 4 + jr(2 * mx(0, sin((t - 2.0) * 10.5) * (t > 2.0 && t < 3.3 ? 1 : 0))); shut = true; }
  else if (t >= 3.6 && t < 4.5) { const D k = ez(pr(t, 3.6, 4.5)); w = jr(lerp(24, 14, k)); h = jr(lerp(4, 12, k)); left = jr(lerp(14, 20, k)); shut = true; }
  else if (t >= 4.5) { const D k = pr(t, 4.5, 5.6); const D e = exp(-3 * k) * cos(k * 14); w = jr(20 - 6 * e); h = jr(9 + 3 * e); left = 20 - jr((w - 14) / 3); shut = k < 0.6; }
  warp(SPR_AD_PUDDLE, left, 28 - h, w, h, PO().S(shut));
  if (t >= 1.7 && t < 3.7) for (int i = 0; i < 3; i++) { const D k = md(t * 0.9 + i * 0.37, 1); bubble(jr(18 + i * 7 + sin(t * 5 + i) * 1.5), jr(22 - k * 9)); }
  if (t > 4.4 && t < 5.0) { const D k = pr(t, 4.4, 5.0); bubble(24, jr(14 - k * 8)); }
  if (t > 5.6 && fl(t * 5) % 2 == 0) spark(32, 14);
}

// TOASTY: the crust (with his eyes) pops up like a toaster, the fish hops up and slides into the middle, the
// crust comes down: a sandwich. His mouth on the bottom slice does the chewing.
void toasty(D t) {
  const D y0 = 19, mid = 20;
  D x = 3, bob = 0, lid = 0, fx = 28, fy = 23;
  if (t < 0.4) fy = dropY(t, 0, 0.4, 23);
  if (t >= 0.4 && t < 1.5) { const D k = pr(t, 0.4, 1.5); x = 3 + jr(k * 11); bob = hop(k, 5, 1); }
  else if (t >= 1.5) x = 14;
  if (t >= 1.5 && t < 1.85) lid = jr(ez(pr(t, 1.5, 1.85)) * 9);
  else if (t >= 1.85 && t < 3.0) lid = 9;
  else if (t >= 3.0 && t < 3.3) lid = jr(lerp(9, 5, ez(pr(t, 3.0, 3.3))));
  else if (t >= 3.3 && t < 3.7) lid = 5;
  else if (t >= 3.7 && t < 4.3) lid = jr(lerp(5, 3, ez(pr(t, 3.7, 3.9))));
  else if (t >= 4.3 && t < 4.9) lid = jr(lerp(3, 1, ez(pr(t, 4.3, 4.5))));
  else if (t >= 4.9 && t < 5.3) lid = jr(lerp(1, 0, ez(pr(t, 4.9, 5.1))));
  else if (t >= 5.3 && t < 5.9) lid = jr(sin(pr(t, 5.3, 5.9) * PI) * 4);
  if (t >= 1.9 && t < 2.4) { const D k = ez(pr(t, 1.9, 2.4)); fy = jr(lerp(23, 18, k)) - jr(sin(k * PI) * 3); }
  else if (t >= 2.4 && t < 3.0) { fy = 18; fx = jr(lerp(28, mid, ez(pr(t, 2.4, 3.0)))); }
  else if (t >= 3.0) { fy = 18; fx = mid; }
  const int fstage = t < 3.7 ? 0 : t < 4.3 ? 1 : t < 4.9 ? 2 : 3;
  const bool chew = t >= 3.3 && t < 5.1 ? fabs(sin((t - 3.3) * 6.5)) > 0.55 : false;
  const bool open = t >= 3.3 && t < 5.1 && fl((t - 3.3) * 6.5 / PI * 2) % 2 == 0;
  const int spr = open ? SPR_TN_TOASTY_EAT : SPR_TN_TOASTY;
  const bool happy = t >= 5.1 && t < 6.0;
  if (fstage < 3) food(fx, fy + (chew ? 1 : 0), fstage);
  const D by = y0 - bob;
  put(spr, x, by, PO().R(4, 8).NoE());
  put(spr, x, by - lid, PO().R(0, 3));
  put(spr, x, by - lid, PO().R(4, 4).OnlyE().S(happy));
  if (t >= 1.55 && t < 2.0 && fl(t * 12) % 2 == 0) { rect(x + 13, y0 - 8, 1, 3, 3); rect(x + 13, y0 - 4, 1, 1, 3); }
  if (t >= 3.7 && t < 5.1) for (int i = 0; i < 4; i++) { const D k = md((t - 3.7) * 1.6 + i * 0.25, 1); set(x + 6 + (rnd(i) - 0.5) * 18 * k, 21 + 6 * k * k * 3 - 6 * k, 2); }
  if (t > 6.0 && fl(t * 5) % 2 == 0) spark(x + 12, y0 - 1);
}

// MARSHY: pinball. The food drops, bounces off his head, hits a side wall (left or right, picked every feed)
// and ricochets into his open mouth. Three squishy chews.
void marshy(D t) {
  const D x = 11, y0 = 18, mxx = 19.5, my = 23;
  const FDs F = FD(); const bool flip = MV == 1;
  auto X = [&](D v) { return flip ? 39 - v : v; };
  const D cw = 0.5 + F.w / 2, headTop = y0 - F.h;
  D fx = 15, fy = -8, scl = 1; bool show = true;
  if (t < 0.3) show = false;
  else if (t < 0.85) { const D k = pr(t, 0.3, 0.85); fx = 15; fy = lerp(-6, headTop, k * k); }
  else if (t < 1.65) { const D k = pr(t, 0.85, 1.65); fx = lerp(15, cw, k); fy = lerp(headTop, 5, k) - sin(k * PI) * 5; }
  else if (t < 1.8) { fx = cw; fy = 5; }
  else if (t < 2.7) { const D k = pr(t, 1.8, 2.7); fx = lerp(cw, mxx, k); fy = lerp(5, my - 1, k * k * (1.4 - 0.4 * k)); scl = k < 0.65 ? 1 : mx(0.25, 1 - (k - 0.65) / 0.35 * 0.75); }
  else show = false;
  D h = 10, w = 17;
  const D dent = t >= 0.85 && t < 1.05 ? 1 : 0;
  if (t >= 2.75 && t < 4.55) { const long ch = fl((t - 2.75) / 0.6); const D k = md(t - 2.75, 0.6) / 0.6; const D e = ch < 3 ? sin(k * PI) : 0; w = 17 + jr(2 * e); h = 10 + (ch < 3 ? jr(2 * e) : 0); }
  h -= dent;
  const bool open = t > 1.8 && t < 2.8;
  warp(open ? SPR_AD_MARSHY_EAT : SPR_AD_MARSHY, x - jr((w - 17) / 2), 28 - h, w, h, PO().S(t > 2.75 && t < 4.75));
  if (show) { const D ww = mx(2, jr(F.w * scl)), hh = mx(2, jr(F.h * scl)); warp(F.n[0], jr(X(fx) - ww / 2), jr(fy), ww, hh); }
  if (t >= 1.65 && t < 1.9 && fl(t * 20) % 2 == 0) spark(flip ? 37 : 0, 3);
  if (t >= 0.85 && t < 1.05) spark(X(14) - 1, 11);
  if (t > 4.8 && fl(t * 5) % 2 == 0) spark(x + 18, 16);
}

// BOOP: a potion flask. Cork pops, the food arcs in, the liquid rises and fizzes, cork back on, a steam puff.
void boop(D t) {
  const D x = 3, y0 = 14, neckX = x + 6;
  const FDs F = FD();
  if (t < 1.2) food(30, dropY(t, 0, 0.4, 23), 0);
  D cork = 0;
  if (t >= 0.5 && t < 1.0) cork = jr(ez(pr(t, 0.5, 0.7)) * 8);
  else if (t >= 1.0 && t < 4.3) cork = 8;
  else if (t >= 4.3 && t < 4.7) cork = jr((1 - pr(t, 4.3, 4.7)) * 8);
  const D level = t < 2.5 ? 0 : t < 3.4 ? ez(pr(t, 2.5, 3.4)) * 0.6 : t < 5.2 ? 0.6 : mx(0, 0.6 * (1 - pr(t, 5.2, 6.8)));
  const D rows = jr(level * 6);
  const Spr bs = sp(SPR_AD_BOOP);
  for (D k = 0; k < rows; k++) { const int r = (int)(12 - k); const D y = y0 + r; for (int c = 1; c < 12; c++) if (at(bs, r, c) == '.') set(x + c, y, 2); }
  const D bounce = t > 4.3 && t < 4.9 ? jr(sin((t - 4.3) * 25)) : 0;
  const bool shut = t > 1.8 && t < 5.2, open = t > 0.9 && t < 2.0;
  put(open ? SPR_AD_BOOP_EAT : SPR_AD_BOOP, x + bounce, y0, PO().R(4, 13).S(shut));
  put(SPR_AD_BOOP, x + bounce, y0 - cork, PO().R(0, 3));
  if (t >= 1.0 && t < 2.0) {
    const D k = pr(t, 1.0, 2.0), fx = lerp(30, neckX, ez(k));
    const D fy = k < 0.6 ? lerp(23, 4, k / 0.6) : lerp(4, y0 + 5, (k - 0.6) / 0.4);
    const D ww = k < 0.55 ? F.w : mx(3, jr(lerp(F.w, 3, (k - 0.55) / 0.45)));
    warp(F.n[0], jr(fx - ww / 2), jr(fy), ww, k < 0.55 ? 5 : 3);
  }
  if (t >= 2.0 && t < 3.6) { const D k = pr(t, 2.0, 3.6); const D fy = jr(lerp(y0 + 5, 25, mn(1, k * 1.6))); if (k < 0.9) warp(F.n[k < 0.5 ? 0 : 1], neckX - 2, fy, 5, 3, PO().Mi(k > 0.5)); }
  if (t > 2.4 && t < 5.4) for (int i = 0; i < 3; i++) { const D k = md(t * 1.2 + i * 0.33, 1); if (rows > 0) set(x + 4 + i * 3 + jr(sin(t * 9 + i)), 25 - k * (rows + 2), 3); }
  if (t > 4.4 && t < 5.4) { const D k = pr(t, 4.4, 5.4); bubble(neckX - 1 + jr(sin(k * 8) * 2), jr(10 - k * 10)); }
  if (t > 6.7 && fl(t * 5) % 2 == 0) spark(x + 13, 13);
}

// CURLY: a teapot. The lid hops, the food drops in; he heats up, shaking and whistling steam; one big blast.
void curly(D t) {
  const D x = 3, y0 = 15;
  const FDs F = FD();
  if (t < 1.8) {
    if (t < 0.4) food(30, dropY(t, 0, 0.4, 23), 0);
    else if (t < 0.9) food(30, 23, 0);
    else {
      const D k = pr(t, 0.9, 1.7);
      const D fx = lerp(30, x + 7, ez(k)), fy = lerp(23, 2, sin(k * PI / 2)) + (k > 0.8 ? (k - 0.8) / 0.2 * 6 : 0);
      const D ww = mx(3, jr(lerp(F.w, 3, k)));
      warp(F.n[0], jr(fx - ww / 2), jr(fy), ww, mx(3, jr(lerp(5, 3, k))));
    }
  }
  D lid = 0;
  if (t >= 0.8 && t < 1.8) lid = 3;
  else if (t >= 2.0 && t < 4.6) lid = fl(t * 14) % 2;
  else if (t >= 4.6 && t < 5.2) lid = jr(sin(pr(t, 4.6, 5.2) * PI) * 6);
  const bool heating = t > 1.9 && t < 4.7; const D shake = heating ? jr(sin(t * 40) * (0.5 + (t - 1.9) / 3)) : 0;
  const bool shut = t > 1.8 && t < 5.4, open = t > 1.0 && t < 1.9;
  put(open ? SPR_TN_CURL_EAT : SPR_TN_CURL, x + shake, y0, PO().R(4, 12).S(shut));
  put(SPR_TN_CURL, x + shake, y0 - lid, PO().R(0, 3));
  const D spx = x + 14, spy = y0 + 7;
  if (t > 2.0 && t < 6.0) for (int i = 0; i < 6; i++) {
    const D age = md((t - 2.0) * 1.4 + i / 6.0, 1); if (t > 4.7 && t < 5.2) continue;
    const D e = t > 4.6 ? 0.5 : 1;
    const D px = jr(spx + age * 12 * e), py = jr(spy - age * 9 + sin(age * 9 + i) * 1.2);
    if (age < 0.25) set(px, py, 3); else disc(px, py, age < 0.7 ? 1 : 2, 2);
  }
  if (t > 4.6 && t < 5.5) for (int i = 0; i < 7; i++) { const D age = pr(t, 4.7 + i * 0.03, 5.5); if (age < 1) set(x + 6 + (rnd(i) - 0.5) * 10 * age * 2, y0 - 2 - age * 11, age < 0.5 ? 3 : 2); }
  if (t > 6.0 && fl(t * 5) % 2 == 0) spark(x + 14, y0 - 1);
}

// PUFF: a vacuum inhale drags the food in while he puffs up like a balloon; hiccup; leaks air and drifts down.
void puff(D t) {
  const D x0 = 3, fy = 23;
  const D inh = pr(t, 0.9, 2.6), fx = t < 0.9 ? 30 : lerp(30, 14, ez(inh));
  if (t < 2.7) {
    const D sc = t > 1.9 ? mx(0, 1 - pr(t, 1.9, 2.6)) : 1;
    if (t < 0.4) food(30, dropY(t, 0, 0.4, fy), 0);
    else { const D ww = mx(2, jr(FD().w * sc)); warp(FD().n[0], jr(fx - ww / 2), 28 - mx(2, jr(5 * sc)), ww, mx(2, jr(5 * sc))); }
  }
  D sc = 1, lift = 0;
  if (t >= 1.4 && t < 3.0) sc = lerp(1, 1.32, ez(pr(t, 1.4, 3.0)));
  else if (t >= 3.0 && t < 3.5) sc = 1.32 + 0.06 * sin((t - 3.0) * 50);
  else if (t >= 3.5 && t < 5.4) { sc = lerp(1.32, 1, ez(pr(t, 3.5, 5.4))); lift = jr(sin(pr(t, 3.5, 5.4) * PI) * 5); }
  const D w = jr(19 * sc), h = jr(13 * sc), x = jr(12.5 - w / 2 + x0);
  const bool open = t > 0.8 && t < 2.7;
  warp(open ? SPR_AD_PUFF_EAT : SPR_AD_PUFF, x, 28 - h - lift, w, h, PO().S(t > 2.6 && t < 5.6));
  if (t > 0.9 && t < 2.6) for (int i = 0; i < 3; i++) { const D k = md(t * 2.2 + i / 3.0, 1); const D lx = jr(lerp(fx + 6, 22, k)); const D ly = 24 - i * 3; line(lx, ly, lx + 3, ly, k < 0.3 ? 2 : 3); }
  if (t > 3.0 && t < 3.5 && fl(t * 16) % 2 == 0) spark(24, 12);
  if (t > 3.5 && t < 5.4) for (int i = 0; i < 4; i++) { const D k = pr(t, 3.5 + i * 0.25, 4.6 + i * 0.25); if (k > 0 && k < 1) bubble(jr(x + (i % 2 ? w : -3) + (i % 2 ? 1 : -1) * k * 6), jr(28 - h / 2 - lift - k * 4 * (i % 2 ? 1 : -1))); }
  if (t > 5.6 && fl(t * 5) % 2 == 0) spark(22, 12);
}

// LOAF: butt wiggle, pounce, sploot over the food; a lump travels head to tail; he rises like bread, slumps back.
void loaf(D t) {
  const D fcx = 7;
  if (t < 1.5) food(fcx, dropY(t, 0, 0.4, 23), 0);
  D left = 19, w = 20, h = 11, lift = 0;
  const D wig = t >= 0.6 && t < 1.3 ? (sin(t * 40) > 0 ? 0 : -1) : 0;
  if (t >= 1.3 && t < 2.0) { const D k = ez(pr(t, 1.3, 2.0)); left = jr(lerp(19, 4, k)); w = k < 0.5 ? jr(lerp(20, 27, k * 2)) : jr(lerp(27, 24, (k - 0.5) * 2)); lift = jr(sin(k * PI) * 7); }
  else if (t >= 2.0) { left = 4; w = 24; }
  if (t >= 2.0 && t < 2.3) h = 11 - jr(sin(pr(t, 2.0, 2.3) * PI) * 3);
  int mode = 0; D cxk = 0, kk = 0;
  if (t >= 2.4 && t < 4.4) { const D k = pr(t, 2.4, 4.4); cxk = lerp(2, w - 2, k); mode = 1; }
  else if (t >= 4.4 && t < 5.8) { kk = ez(pr(t, 4.4, 5.4)); mode = 2; }
  else if (t >= 5.8 && t < 6.3) { kk = 1 - ez(pr(t, 5.8, 6.3)); mode = 3; }
  auto colH = [&](D c) -> D {
    if (mode == 1) return jr(3 * exp(-pow((c - cxk) / 5, 2)));
    if (mode == 2) return jr(3 * kk * (c > 2 && c < w - 2 ? 1 : 0.4));
    if (mode == 3) return jr(3 * kk);
    return 0;
  };
  const bool shut = t > 2.3 && t < 6.5, open = t > 1.5 && t < 2.4;
  const Spr Rs = sp(open ? SPR_AD_LOAF_EAT : SPR_AD_LOAF);
  for (D cx = 0; cx < w; cx++) {
    const D extra = colH(cx), ch = h + extra, sx = w > 1 ? jr(cx * (Rs.w - 1) / (w - 1)) : 0;
    const D wg = (t >= 0.6 && t < 1.3 && cx >= 15) ? wig : 0;
    for (D dy = 0; dy < ch; dy++) {
      const D sy = ch > 1 ? jr(dy * (Rs.h - 1) / (ch - 1)) : 0; const char c = at(Rs, (int)sy, (int)sx);
      const int v = c == 'X' || c == 'M' ? 3 : c == 'E' ? (shut ? 0 : 3) : c == 'G' ? 2 : 0;
      if (v) set(left + cx, 28 - ch - lift + dy + wg, v);
    }
  }
  if (t > 6.3 && fl(t * 5) % 2 == 0) spark(left + w - 3, 11);
}

// NIMBUS: floats over and vortexes the food into his belly; goes dark, lightning (screen flash), rain; a rainbow.
void nimbus(D t) {
  D left = 3, by = 16; const bool rain = t < 1.0 || (t > 3.9 && t < 6.0), storm = t > 3.3 && t < 5.9;
  if (t >= 1.0 && t < 2.2) left = 3 + jr(ez(pr(t, 1.0, 2.2)) * 19);
  else if (t >= 2.2) left = 22;
  const D hover = jr(sin(t * 3));
  by = 16 + (t >= 1.0 ? -3 : 0) + hover;
  if (t < 3.4) {
    const D fcx = 31;
    if (t < 0.4) food(fcx, dropY(t, 0, 0.4, 23), 0);
    else if (t < 2.4) food(fcx, 23, 0);
    else { const D k = ez(pr(t, 2.4, 3.4)); const D ww = mx(2, jr(FD().w * (1 - k * 0.7))); const D ang = k * 9; if (k < 1) warp(FD().n[0], jr(fcx - ww / 2 + sin(ang) * 3 * (1 - k)), jr(lerp(23, by + 4, k)), ww, mx(2, jr(5 * (1 - k * 0.5))), PO().Mi(k > 0.5)); }
  }
  const bool bshut = t > 3.3 && t < 6.4;   // eyes shut the whole time, through the storm and the clearing
  put(storm ? SPR_AD_NIMBUS_EAT : SPR_AD_NIMBUS, left, by, PO().R(0, 8).S(bshut).Dk((storm && fl(t * 3) % 2 == 0) || (storm && t > 3.6)));
  if (rain) for (int c = 0; c < 3; c++) for (int i = 0; i < 3; i++) {
    const D spd = storm ? 12 : 5, yy = by + 9 + md(t * spd + c * 2 + i * 3.1, 10);
    if (yy < 28) set(left + 2 + c * 5 + (storm ? i % 2 : 0), yy, storm ? 3 : 2);
    if (storm && yy < 28 && c != 1) set(left + 5 + c * 4, md(yy + 4, 12) + by + 9 > 27 ? 27 : by + 9 + md(md(yy + 4, 12) - by, 10), 2);
  }
  if (t >= 3.3 && t < 3.55) INV = fl(t * 30) % 2 == 0;
  if (t >= 3.35 && t < 3.7) { D x = left + 8, y = by + 9; for (int i = 0; i < 5; i++) { const D nx = x + (i % 2 ? 2 : -2), ny = y + 3; line(x, y, nx, ny, 3); x = nx; y = ny; if (y > 27) break; } }
  if (t >= 3.9 && t < 6.0) { const D k = pr(t, 3.9, 5.8); const D w = jr(k * 12); rect(left + 8 - jr(w / 2), 27, w, 1, 2); }
  if (t >= 5.9) {
    const D k = pr(t, 6.0, 6.8);
    for (D a = 0; a < 17 * k; a++) { const D ang = PI * (1 - a / 17); const D cx = left + 8, cy = by - 1; set(cx + cos(ang) * 9, cy - sin(ang) * 7, 3); set(cx + cos(ang) * 8, cy - sin(ang) * 6, 2); }
  }
}

// (Ballboi's two halves: the zoomed-out run-in, then the zoomed-in feast, which starts at BB_T0.)
constexpr D BB_T0 = 3.25;
const char* const BB_HEAD[] = {".XX...XX.", "X..XXX..X", "X.......X", "X.E...E.X", "X...M...X", ".X.....X.", "..XXXXX.."};
void hamsterBall(D cx, D cy, D ang, D lean, D bob) {
  const D R = 6;
  for (D dy = -R - 1; dy <= R + 1; dy++) for (D dx = -R - 1; dx <= R + 1; dx++) {
    if (!inRim(dx * dx + dy * dy, R)) continue;
    const D th = atan2(dy, dx) - ang; bool seam = false;
    for (D o : {0.0, PI}) { const D q = md(md(th - o, 2 * PI) + 3 * PI, 2 * PI) - PI; if (fabs(q) < 0.42) seam = true; }
    set(cx + dx, cy + dy, seam ? 2 : 3);
  }
  set(jr(cx + cos(ang + PI / 2) * 4.6), jr(cy + sin(ang + PI / 2) * 4.6), 2);
  const D hx = cx - 4 + lean, hy = cy - 1 + bob;
  for (int y = 0; y < 7; y++) for (int x = 0; x < 9; x++) if (BB_HEAD[y][x] != '.') set(hx + x, hy + y, 3);
}
void ballboiScene(D t) {
  const D fcx = 31;
  const int gul = t < 1.6 ? 0 : t < 2.1 ? 1 : t < 2.6 ? 2 : 3;
  if (t < 0.4) food(fcx, dropY(t, 0, 0.4, 23), 0); else if (t < 2.7) food(fcx, 23, gul < 2 ? gul : 2);
  D x = 2, bob = 0;
  if (t >= 0.4 && t < 1.5) { const D k = pr(t, 0.4, 1.5); x = 2 + jr(k * 9); bob = hop(k, 4, 1); }
  else if (t >= 1.5) x = 11;
  D cheek = t < 1.6 ? 0 : t < 2.7 ? gul : t < 4.2 ? 3 : t < 4.8 ? 3 * (1 - pr(t, 4.2, 4.8)) : 0;
  cheek = jr(cheek);
  const bool open = t > 1.5 && t < 2.7 && fl(t * 6) % 2 == 0;
  const bool shut = t > 2.7 && t < 5.0;
  const D w = 17 + cheek, hh = 15;
  warp(open ? SPR_AD_BALLBOI_EAT : SPR_AD_BALLBOI, x - jr(cheek / 2), 13 - bob, w, hh, PO().S(shut));
  if (cheek > 0) for (int sd : {-1, 1}) { const D cx = sd < 0 ? x - jr(cheek / 2) + 2 : x - jr(cheek / 2) + w - 3; disc(cx, 21, mn(2, 1 + (cheek > 1 ? 1 : 0)), 2); }
  if (t >= 1.6 && t < 2.7) { const D g = (t - 1.6) / 0.5, gi = floor(g), k = g - gi; if (gi < 3) { const D px = lerp(fcx, x + 8, k), py = 23 - sin(k * PI) * 6 + k * 3; warp(FD().n[0], jr(px - 2), jr(py), 4, 3); } }
  if (t >= 4.3 && t < 5.4) { const D k = pr(t, 4.3, 5.4); const D bx = x + 14 + k * 18, by_ = 20 + 14 * k * k - 10 * k; put(X_BONE, jr(bx), jr(by_)); }
  if (t > 5.4 && fl(t * 5) % 2 == 0) spark(x + 17, 12);
}
// BALLBOI: zoomed out, he runs in inside his hamster ball (the shell spins round him, he stays at the bottom);
// zoom back in: stuffs his cheeks a gulp at a time, chews, ptooey: a fish skeleton.
void ballboi(D t) {
  if (t >= BB_T0) { ballboiScene(t - BB_T0 + 1.5); return; }
  const FDs F = FD();
  const D fw = mx(2, jr(F.w / 2)), fh = mx(2, jr(F.h / 2));
  warp(F.n[0], 35 - jr(fw / 2), t < 0.4 ? dropY(t, 0, 0.4, 27 - fh) : 27 - fh + 1, fw, fh);
  const D R = 6, x0 = -8, x1 = 26 - jr(fw / 2);
  const D q0 = pr(t, 0.2, 2.5), k = 1 - (1 - q0) * (1 - q0), d = (x1 - x0) * k;
  const bool moving = q0 > 0 && q0 < 1;
  const D rock = t > 2.5 ? sin((t - 2.5) * 14) * exp(-(t - 2.5) * 5) : 0;
  const D cx = jr(x0 + d + rock * 1.2), cy = 27 - R;
  const D ang = (d / R) * 2 + rock * 0.6;
  const D spd = moving ? 2 * (1 - q0) : 0;
  const D lean = moving ? (spd > 0.4 ? 1 : 0) : (t > 2.5 && t < 2.8 ? 1 : 0);
  const long step = fl(t * (6 + spd * 8)) % 2;
  const D bob = (moving && step) || (t > 2.5 && t < 2.65) ? -1 : 0;
  hamsterBall(cx, cy, ang, lean, bob);
  for (int i = 0; i < 3 && moving && k < 0.95; i++) set(cx - 8 - i * 3 - jr(rnd(fl(t * 10) + i) * 2), 26 - (i % 2), 2);
  if (t > 2.5 && t < 2.8 && fl(t * 20) % 2 == 0) set(cx + R + 1, cy - 3, 3);
  if (t >= 3.0) {
    const D q = pr(t, 3.0, BB_T0); const D xa = jr(q * 8), xb = 39 - jr(q * 4), ya = jr(q * 12);
    for (D x = xa; x <= xb; x++) { set(x, ya, 3); set(x, 27, 3); }
    for (D y = ya; y <= 27; y++) { set(xa, y, 3); set(xb, y, 3); }
  }
}

// SPROUT: hops onto the food, which fizzles up his stem; the stem shoots up, a flower blooms, pops into petals.
void sprout(D t) {
  const D fcx = 29, x0 = 3;
  D x = x0, hopY = 0;
  if (t >= 0.4 && t < 1.5) { const D k = pr(t, 0.4, 1.5); x = x0 + jr(k * 18); hopY = hop(k, 3, 3); }
  else if (t >= 1.5) x = x0 + 18;
  const int stage = t < 1.7 ? 0 : t < 2.1 ? 1 : t < 2.5 ? 2 : 3;
  if (t < 0.4) food(fcx, dropY(t, 0, 0.4, 23), 0); else food(fcx, 23, stage, PO().Mi(stage > 0));
  D grow = 0;
  if (t >= 2.3 && t < 3.3) grow = jr(ez(pr(t, 2.3, 3.3)) * 6);
  else if (t >= 3.3 && t < 5.6) grow = 6; else if (t >= 5.6 && t < 6.2) grow = jr((1 - ez(pr(t, 5.6, 6.2))) * 6);
  const bool shut = t > 1.7 && t < 5.9;
  const D y0 = 13 - hopY;
  put(SPR_TN_SPROUT, x, y0, PO().R(5, 14).S(shut));
  for (D i = 1; i <= grow; i++) set(x + 7, y0 + 5 - i + 1, 3);
  const D sway = t > 3.3 && t < 5.6 ? jr(sin(t * 4)) : 0;
  put(SPR_TN_SPROUT, x + sway, y0 - grow, PO().R(0, 4));
  if (t >= 1.7 && t < 2.8) for (int i = 0; i < 4; i++) { const D k = md((t - 1.7) * 1.3 + i * 0.25, 1); set(x + 7 + jr(sin(k * 8 + i) * 1.5), y0 + 13 - k * 12, 3); }
  if (t >= 3.2 && t < 5.7) { const D k = ez(pr(t, 3.2, 3.9)); const D s = mx(1, jr(5 * k)); if (s >= 5) put(SPR_FLOWER, x + 5 + sway, y0 - grow - 5); else if (s > 1) rect(x + 7 + sway, y0 - grow - s + 1, 1, s, 3); }
  if (t >= 5.6 && t < 6.6) { const D k = pr(t, 5.6, 6.6); for (int i = 0; i < 4; i++) set(x + 7 + (i - 1.5) * 3 * (0.4 + k) + sin(k * 6 + i) * 1, y0 - 2 + k * 14 + i, 3); }
  if (t > 6.2 && fl(t * 5) % 2 == 0) spark(x + 14, y0 + 1);
}

// FUBZ: three polite bites... then he heaves and a fuzzy hairball rolls out and away.
void fubz(D t) {
  const D x = 3; long bite = fl(t / 0.8); if (bite > 3) bite = 3;
  const bool openb = t < 2.4 && md(t, 0.8) < 0.4;
  if (t < 0.4) food(24, dropY(t, 0, 0.4, 23), 0); else if (t < 2.4) food(24, 23, (int)bite);
  const bool heave = t >= 2.6 && t < 4.4;
  const D dxs = heave ? jr(sin(t * 32) * (0.5 + pr(t, 2.6, 4.4))) : 0;
  const D st = heave ? (D)(fl(t * 5) % 2) : 0;
  const bool bounce = !heave && t < 2.4 && md(t, 0.8) > 0.4 && md(t, 0.8) < 0.55;
  const int sprn = openb || (heave && t < 4.2 && fl(t * 4) % 2 == 0) ? SPR_AD_FUBZ_EAT : SPR_AD_FUBZ;
  const bool shut = (!openb && t >= 0.8 && t < 2.5 && md(t, 0.8) > 0.4) || heave || (t > 5.2 && t < 6.2);
  const D h = 13 + st;
  warp(sprn, x + dxs, 28 - h - (bounce ? 1 : 0), 17, h, PO().S(shut));
  if (t >= 4.1 && t < 6.2) {
    const D k = pr(t, 4.1, 5.2), bx = x + 9 + k * 20, by = 24 - sin(mn(1, k) * PI) * 8;
    const D roll = t > 5.2 ? mn(8, (t - 5.2) * 14) : 0;
    const D cx = jr(bx + roll), cy = jr(t > 5.2 ? 25 : by);
    disc(cx, cy, 2, 2); set(cx - 1, cy - 1, 3); set(cx + 1, cy, 3);
  }
  if (t > 5.0 && t < 5.4 && fl(t * 20) % 2 == 0) spark(x + 19, 12);
  if (t > 6.2 && fl(t * 5) % 2 == 0) spark(x + 16, 13);
}

// DAPPER: fine dining. A knife and fork cut the meal, the fork feeds him two bites, he dabs his mouth.
void dapper(D t) {
  const D x = 2, y0 = 12, fv = 24;
  rect(fv - 1, 27, FD().w + 2, 1, 2);
  const int st = t < 3.2 ? 0 : t < 4.4 ? 1 : t < 5.3 ? 2 : 3;
  if (t < 0.4) food(fv + 4, dropY(t, 0, 0.4, 22), 0); else if (st < 3 && !(t >= 3.0 && t < 3.2)) food(fv + 4, 22, st);
  const bool openm = (t > 3.0 && t < 3.5) || (t > 4.2 && t < 4.7);
  const bool shut = (t >= 3.5 && t < 4.2) || (t >= 4.7 && t < 5.4);
  put(openm ? SPR_S1C_EAT : SPR_S1C_IDLE, x, y0 - (t > 0.4 && t < 0.6 ? 1 : 0), PO().S(shut));
  if (t >= 0.5 && t < 1.2) { const D k = ez(pr(t, 0.5, 1.2)); put(X_FORK, fv + 6, jr(lerp(-5, 15, k))); put(X_KNIFE, fv + 1, jr(lerp(-5, 15, k))); }
  else if (t >= 1.2 && t < 2.9) { const D sw_ = t > 1.7 && t < 2.8 ? jr(sin((t - 1.7) * 16) > 0 ? 1 : 0) : 0; put(X_FORK, fv + 6, 15); put(X_KNIFE, fv + 1 + sw_, 15); if (sw_ != 0 && fl(t * 8) % 4 == 0) spark(fv + 3, 12); }
  else if (t >= 2.9 && t < 3.5) { const D k = ez(pr(t, 2.9, 3.5)); put(X_FORK, jr(lerp(fv + 6, x + 14, k)), jr(lerp(15, y0 + 8, k))); warp(FD().n[2], jr(lerp(fv + 6, x + 14, k)) + 1, jr(lerp(15, y0 + 8, k)) - 3, 3, 3); put(X_KNIFE, fv + 1, 15); }
  else if (t >= 3.5 && t < 4.0) { const D k = ez(pr(t, 3.5, 4.0)); put(X_FORK, jr(lerp(x + 14, fv + 6, k)), jr(lerp(y0 + 8, 15, k))); put(X_KNIFE, fv + 1, 15); }
  else if (t >= 4.0 && t < 4.7) { const D k = ez(pr(t, 4.0, 4.5)); put(X_FORK, jr(lerp(fv + 6, x + 14, k)), jr(lerp(15, y0 + 8, k))); warp(FD().n[2], jr(lerp(fv + 6, x + 14, k)) + 1, jr(lerp(15, y0 + 8, k)) - 3, 3, 3); put(X_KNIFE, fv + 1, 15); }
  else if (t >= 4.7 && t < 5.4) { const D k = ez(pr(t, 4.7, 5.4)); put(X_FORK, jr(lerp(x + 14, fv + 6, k)), jr(lerp(y0 + 8, -6, k))); put(X_KNIFE, fv + 1, jr(lerp(15, -6, k))); }
  if (t >= 5.2 && t < 6.4) { const D k = pr(t, 5.2, 6.4), nx = jr(lerp(x + 15, x + 9, mn(1, k * 3))), wipe = jr(sin(k * 20) * 1.2); rect(nx + wipe, y0 + 8, 4, 3, 2); rect(nx + wipe, y0 + 8, 4, 1, 3); }
  if (t > 6.4 && fl(t * 5) % 2 == 0) spark(x + 16, y0 + 1);
}

// BEAN: a jumping bean. Swallows it whole, it rattles around inside, three wild hops, a little pop, calm.
void bean(D t) {
  const FDs F = FD(); const D fcx = 31, y0 = 17;
  D x = 3, lift = 0, tilt = 0, sq = 0;
  if (t < 0.4) food(fcx, dropY(t, 0, 0.4, 23), 0); else if (t < 1.2) food(fcx, 23, 0);
  if (t >= 0.4 && t < 1.2) { const D k = pr(t, 0.4, 1.2); x = jr(lerp(3, 13, ez(k))); lift = hop(k, 2, 2); }
  else if (t >= 1.2) x = 13;
  if (t >= 1.2 && t < 1.9) { const D k = ez(pr(t, 1.2, 1.9)); const D w = lerp(F.w, 1, k), h = lerp(F.h, 1, k); warp(F.n[0], jr(lerp(fcx, x + 6, k) - w / 2), jr(lerp(25, y0 + 8, k) - h / 2), w, h); }
  static const D hops[3][4] = {{2.3, 3.0, 6, -3}, {3.05, 3.85, 8, 4}, {3.9, 4.5, 4, -2}};
  D hx = 0;
  for (const auto& hp : hops) {
    const D a = hp[0], b = hp[1], ht = hp[2], dx = hp[3];
    if (t >= b) hx += dx;
    else if (t >= a) { const D k = pr(t, a, b); lift = jr(sin(k * PI) * ht); hx += jr(dx * k); tilt = sgn(dx) * (k < 0.5 ? 1 : -1); }
  }
  for (const auto& hp : hops) if (t >= hp[1] && t < hp[1] + 0.08) sq = 1;
  if (t >= 4.5 && t < 4.65) sq = 1;
  x += hx;
  const bool open = (t >= 1.2 && t < 1.9) || (t >= 4.85 && t < 5.0);
  const bool shut = t > 5.0 && t < 6.2;
  const D h = 11 - sq, w = 13 + sq * 2;
  PO o; o.shut = shut; o.dxk = 2; o.a = tilt;
  warp(open ? SPR_TN_BEAN_EAT : SPR_TN_BEAN, x - sq, 28 - h - lift, w, h, o);
  if (t >= 1.9 && t < 4.85) {
    const D lx = 5 + jr(sin(t * 13) * 2.4), ly = (lift > 2 ? 7 : 3) + jr(cos(t * 17));
    rect(x + lx - tilt * jr((5 - ly) * 0.2), 28 - h - lift + ly, 3, 2, 2);
    if (t > 2.3 && fl(t * 12) % 2 == 0) { set(x - 2, 28 - h - lift + 3, 3); set(x + 15, 28 - h - lift + 5, 3); set(x - 3, 28 - h - lift + 6, 3); }
  }
  if (t >= 1.95 && t < 2.3) spark(x + 6, y0 - 6);
  if (t >= 4.85 && t < 5.15) ring(x + 6, y0 - 3, 1 + jr(pr(t, 4.85, 5.15) * 2), 3);
  if (t > 5.6 && fl(t * 5) % 2 == 0) spark(x + 14, y0);
}

void baoBody(int id, D x, D y, D g, bool over, bool shut) {   // g: how far the pleats at the top are pulled apart; over: upside down
  const Spr s = sp(id);
  for (int r = 0; r < s.h; r++) for (int c = 0; c < s.w; c++) {
    const char ch = at(s, r, c);
    if (ch == '.' || (ch == 'E' && shut)) continue;
    D dx = 0;
    if (r < 3 && g > 0) { if (c == 7) continue; dx = c < 7 ? -g : g; }   // the knot is gone, the pleats lie open
    set(x + c + dx, over ? y + s.h - 1 - r : y + r, 3);
  }
}
// BUN: a bao bun, and it stuffs itself. The pinched top comes undone, it flips over onto the food (which goes in
// through the top, as the filling), flips back, pinches itself shut again and steams: a puff with every chew.
void bun(D t) {
  const D x0 = 8, x1 = 21, x2 = 12, y0 = 17, fcx = x1 + 7;   // (x1: standing right over the food)
  D g = 0;
  if (t >= 0.6 && t < 1.0) g = jr(ez(pr(t, 0.6, 1.0)) * 3);
  else if (t >= 1.0 && t < 3.1) g = 3;
  else if (t >= 3.1 && t < 3.4) g = jr(lerp(3, 0, pr(t, 3.1, 3.4)));
  D bx = x0, by = y0; bool over = false;
  if (t >= 1.0 && t < 1.9) { const D k = pr(t, 1.0, 1.9); bx = jr(lerp(x0, x1, k)); by = jr(y0 - sin(k * PI) * 11); over = k > 0.5; }
  else if (t >= 1.9 && t < 2.4) { bx = x1 + (t >= 2.1 ? fl(t * 12) % 2 : 0); over = true; }   // a wriggle: stuffing it in
  else if (t >= 2.4 && t < 3.1) { const D k = pr(t, 2.4, 3.1); bx = jr(lerp(x1, x2, k)); by = jr(y0 - sin(k * PI) * 9); over = k < 0.5; }
  else if (t >= 3.1) bx = x2;
  const D chewk = (t - 3.4) / 0.6;
  const bool chewing = t >= 3.4 && t < 5.2, puff = chewing && md(chewk, 1) < 0.5;
  const int st = t < 3.4 ? 0 : (int)mn(3, floor(chewk) + (md(chewk, 1) > 0.5 ? 1 : 0));
  // the food: on the floor, then the filling (seen through the dough)
  if (t < 0.4) food(fcx, dropY(t, 0, 0.4, 23), 0);
  else if (t < 2.4) food(fcx, 23, 0);
  else if (t < 3.1) food(bx + 7.5, by + jr(lerp(6, 4, pr(t, 2.4, 3.1))), 0, PO().Mi(true));
  else if (st < 3) food(bx + 7.5, by + 4, st, PO().Mi(true));
  const bool shut = t >= 5.2 && t < 6.0;
  if (puff) warp(SPR_TN_BUN_EAT, bx - 1, by - 1, 17, 12);   // swells with each chew
  else baoBody(SPR_TN_BUN, bx, by, g, over, shut);
  if (t >= 1.9 && t < 2.1) for (D sx : {bx - 2, bx + 16}) { set(sx, 26, 3); set(sx - sgn(sx - bx - 7), 24, 3); }   // whump
  if (t >= 3.4 && t < 3.6) spark(bx + 6, by - 4);   // pinched shut
  for (int i = 0; i < 3; i++) {   // steam out of the top
    const D t0 = 3.45 + i * 0.6; if (t < t0 || t > t0 + 1.2) continue;
    const D k = pr(t, t0, t0 + 1.2); disc(jr(bx + 7 + sin(k * 7 + i) * 2 + (i - 1) * 2), jr(by - 3 - k * 11), k < 0.6 ? 1 : 0, 2);
  }
  if (t >= 5.2 && t < 5.9) { const D k = pr(t, 5.2, 5.9); set(bx + 5, by - 2 - jr(k * 5), 2); set(bx + 9, by - 3 - jr(k * 5), 2); set(bx + 7, by - 4 - jr(k * 6), 3); }   // and a last little whistle of it
  if (t > 5.8 && fl(t * 5) % 2 == 0) spark(bx + 17, by);
}

void sloshBody(int id, D x, D y, D L, D T, D wph, bool shut) {
  const Spr s = sp(id); const int w = s.w; const D mid = (w - 1) / 2.0;
  for (int r = 0; r < s.h; r++) {
    int l = -1, rr = -1;
    for (int c = 0; c < w; c++) if (at(s, r, c) == 'X') { if (l < 0) l = c; rr = c; }
    if (l < 0) continue;
    int le = l; while (le + 1 < w && at(s, r, le + 1) == 'X') le++;
    int rs = rr; while (rs - 1 >= 0 && at(s, r, rs - 1) == 'X') rs--;
    const bool edge = r == 0 || r == s.h - 1;
    for (int c = 0; c < w; c++) {
      const char ch = at(s, r, c);
      if (ch == 'X' && (edge || c <= le || c >= rs)) set(x + c, y + r, 3);
      if (!edge && c > le && c < rs) {
        const D surf = L + T * (c - mid) / mid;
        if (r > surf + 0.5) set(x + c, y + r, 2);
        else if (fabs(r - surf) <= 0.5) set(x + c, y + r, md(md(c + wph, 4) + 4, 4) < 2 ? 3 : 2);
      }
      if (ch == 'E' && !shut) set(x + c, y + r, 3);
      if (ch == 'M') set(x + c, y + r, 3);
    }
  }
}
// SLOSH: a jar of liquid. Slurps the food up, level rises, sloshes, a drop spills; he hops over and slurps it too.
void slosh(D t) {
  const D fcx = 31, y0 = 16;
  D x = 2, L = 6.4, T = 0, lift = 0;
  if (t < 0.4) food(fcx, dropY(t, 0, 0.4, 23), 0);
  if (t >= 0.4 && t < 1.0) { const D k = pr(t, 0.4, 1.0); x = jr(lerp(2, 11, ez(k))); lift = hop(k, 2, 1); T = sin(k * 12) * 1.2; }
  else if (t >= 1.0) x = 11;
  static const D sts[3] = {1.5, 2.0, 2.5};
  const int st = stepIn(t, sts, 3);
  if (t >= 0.4 && t < 2.6) food(fcx, 23, st);
  if (t >= 1.0 && t < 2.6) {
    L = lerp(6.4, 4.6, pr(t, 1.0, 2.6));
    for (int i = 0; i < 4; i++) { const D k = md((t - 1.0) * 2.2 + i / 4.0, 1); set(jr(lerp(fcx - 3, x + 8, k)), jr(lerp(25, y0 + 5, k) - sin(k * PI) * 3), 2); }
  } else if (t >= 2.6) L = 4.6;
  D sx = 0;
  if (t >= 2.6 && t < 4.4) { const D k = t - 2.6; T = 2.6 * sin(k * 7) * exp(-k * 0.6); sx = -jr(sin(k * 7) * exp(-k * 0.6)); }
  const D dropT0 = 2.6 + (PI / 2) / 7;
  bool hasDrop = false; D dx_ = 0, dy_ = 0;
  if (t >= dropT0 && t < dropT0 + 0.55) { const D k = pr(t, dropT0, dropT0 + 0.55); dx_ = jr(lerp(x + 14, x + 21, k)); dy_ = jr(y0 + 4 - sin(k * PI) * 5 + k * 18); hasDrop = true; }
  else if (t >= dropT0 + 0.55 && t < 4.75) { dx_ = x + 21; dy_ = 26; hasDrop = true; }
  if (t >= 4.4 && t < 4.75) { const D k = pr(t, 4.4, 4.75); sx = jr(k * 6); lift = hop(k, 1, 2); }
  else if (t >= 4.75 && t < 5.0) sx = 6;
  else if (t >= 5.0 && t < 5.5) { const D k = pr(t, 5.0, 5.5); sx = jr((1 - k) * 6); lift = hop(k, 1, 2); }
  if (t >= 4.75) L = 4.3;
  if (hasDrop) { if (dy_ < 26) put(SPR_DROP, dx_ - 1, dy_ - 2); else { set(dx_ - 1, 27, 2); set(dx_, 27, 2); set(dx_ + 1, 27, 2); set(dx_, 26, 2); } }
  const bool open = (t >= 1.0 && t < 2.6) || (t >= 4.7 && t < 4.95);
  const D wph = floor(t * 6);
  sloshBody(open ? SPR_TN_SLOSH_EAT : SPR_TN_SLOSH, x + sx, y0 - lift, L, T, wph, t > 5.6 && t < 6.6);
  if (t >= 4.75 && t < 4.95) spark(x + 16, y0 - 2);
  if (t > 6.0 && fl(t * 5) % 2 == 0) spark(x + 16, y0);
}

void campfire(D x, D t, bool out) {
  set(x, 27, 3); set(x + 4, 27, 3); set(x + 1, 26, 3); set(x + 3, 26, 3); set(x + 2, 27, 3);
  if (out) return;
  const long f = fl(t * 10) % 3;
  static const int s0[][2] = {{1, 25}, {2, 25}, {3, 25}, {1, 24}, {2, 24}, {3, 24}, {2, 23}, {3, 23}, {3, 22}};
  static const int s1[][2] = {{1, 25}, {2, 25}, {3, 25}, {2, 24}, {3, 24}, {4, 24}, {3, 23}, {4, 23}, {4, 22}, {5, 21}};
  static const int s2[][2] = {{1, 25}, {2, 25}, {3, 25}, {1, 24}, {2, 24}, {3, 24}, {2, 23}, {2, 22}};
  if (f == 0) for (const auto& p : s0) set(x + p[0], p[1], 3);
  else if (f == 1) for (const auto& p : s1) set(x + p[0], p[1], 3);
  else for (const auto& p : s2) set(x + p[0], p[1], 3);
  set(x + 2 + f, 21 - (f == 1 ? 1 : 0), 2);
}
// MALLOW: a s'more. Sits on the food, a tiny campfire toasts him melty, chocolate drips, fire out, happy puddle.
void mallow(D t) {
  const D fcx = 28, fx0 = 14;
  D cx = 9, lift = 0, w = 12, h = 10, base = 28;
  if (t < 0.4) food(fcx, dropY(t, 0, 0.4, 23), 0);
  if (t >= 0.4 && t < 1.2) { const D k = pr(t, 0.4, 1.2); cx = lerp(9, fcx, ez(k)); base = lerp(28, 24, ez(k)); lift = jr(sin(k * PI) * 6); }
  else if (t >= 1.2) { cx = fcx; base = 24; }
  if (t >= 1.2 && t < 1.35) { h = 9; w = 13; }
  const D melt = t >= 1.9 && t < 4.3 ? ez(pr(t, 1.9, 3.8)) : 0;
  const int st = t < 2.4 ? 0 : t < 3.1 ? 1 : t < 3.8 ? 2 : 3;
  if (t >= 0.4 && st < 3 && t < 4.3) food(fcx, 23, st, PO().Mi(t >= 2.4));
  if (melt != 0) { h = jr(10 - melt * 3); w = jr(12 + melt * 4); }
  if (t >= 4.3) { cx = fcx; const D k = pr(t, 4.3, 4.5); base = jr(lerp(24, 28, k)); h = 7; w = 16; }
  if (t >= 4.9) { const D k = ez(pr(t, 4.9, 6.0)); h = jr(lerp(7, 10, k)); w = jr(lerp(16, 12, k)); }
  const D by = base - h - lift;
  warp(SPR_TN_MALLOW, jr(cx - w / 2), by, w, h, PO().S(t >= 2.0 && t < 6.0));
  if (melt > 0.1 || (t >= 4.3 && t < 5.2)) {
    const D len = t < 4.3 ? jr(melt * (h + 2)) : mx(0, jr((h - 2) * (1 - pr(t, 4.3, 5.2))));
    const D lx = jr(cx - w / 2);
    const D cs[3][2] = {{2, 1}, {5, 0.7}, {w - 4, 0.9}};
    for (const auto& cf : cs) {
      const D c = cf[0], n = jr(len * cf[1]);
      for (D i = 0; i < n; i++) set(lx + c, by + 3 + i, 2);
      if (n > 1) { set(lx + c + 1, by + 2 + n, 2); set(lx + c, by + 2 + n, 2); }
    }
  }
  if (t >= 1.5 && t < 4.6) campfire(fx0, t, t >= 3.9);
  if (t >= 1.5 && t < 1.9 && fl(t * 10) % 2 == 0) spark(fx0 - 4, 20);
  if (t >= 3.9 && t < 4.8) for (int i = 0; i < 3; i++) { const D k = md((t - 3.9) * 1.4 + i * 0.3, 1); set(fx0 + 2 + jr(sin(k * 6 + i) * 1.5), jr(25 - k * 10), 2); }
  if (t >= 5.0 && t < 5.6) disc(jr(cx + 9), jr(20 - pr(t, 5.0, 5.6) * 6), 1, 2);
  if (t > 6.0 && fl(t * 5) % 2 == 0) spark(cx + 8, 16);
}

// SPIKYBOI: his head boings across on a spring, chomps, twangs back; his belly balloons, then a burp.
void spiky(D t) {
  const D x0 = 4, fcx = 31;
  if (t < 0.4) food(fcx, dropY(t, 0, 0.4, 23), 0);
  const int st = t < 1.6 ? 0 : t < 2.0 ? 1 : t < 2.4 ? 2 : 3;
  if (t >= 0.4 && st < 3) food(fcx, 23, st);
  D bw = 13, bh = 7;
  if (t >= 0.4 && t < 0.8) bh = 6;
  if (t >= 3.6 && t < 5.0) { const D k = ez(pr(t, 3.6, 4.4)); bw = jr(13 + k * 5 + sin(t * 20) * 0.6 * k); bh = jr(7 + k * 2); }
  if (t >= 5.0 && t < 5.3) { const D k = pr(t, 5.0, 5.3); bw = jr(18 - k * 5); bh = jr(9 - k * 2); }
  const D bcx = x0 + 6.5;
  floorWarp(X_SPIKY_BODY, bcx, bw, bh, 0);
  const D btop = 28 - bh;
  const D restX = x0, restY = btop - 6;
  D hx = restX, hy = restY;
  const D fx = fcx - 6.5, fy = 17;
  if (t >= 0.4 && t < 0.8) { /* at rest */ }
  else if (t >= 0.8 && t < 1.4) { const D k = ez(pr(t, 0.8, 1.4)); hx = lerp(restX, fx, k); hy = lerp(restY, fy, k) - sin(k * PI) * 7; }
  else if (t >= 1.4 && t < 2.6) { hx = fx; hy = fy + (md(t - 1.4, 0.4) < 0.2 ? 1 : 0); }
  else if (t >= 2.6 && t < 3.6) {
    const D k = pr(t, 2.6, 2.95); hx = lerp(fx, restX, ez(k)); const D base_ = lerp(fy, restY, ez(k));
    hy = base_ - (t > 2.95 ? jr(6 * exp(-(t - 2.95) * 6) * cos((t - 2.95) * 18)) : sin(k * PI) * 4);
  }
  else hy = restY - (t >= 5.0 && t < 5.25 ? 2 : 0);
  hx = jr(hx); hy = jr(hy);
  const D ax = bcx, ay = btop, bx = hx + 6.5, by = hy + 5;
  const D n = jr(hypot(bx - ax, by - ay));
  if (n > 2) for (D i = 0; i <= n; i++) {
    const D k = i / n, px = lerp(ax, bx, k), py = lerp(ay, by, k), nx = -(by - ay) / n, ny = (bx - ax) / n;
    const long ii = (long)i; const D z = (ii % 3 == 1 ? 1 : ii % 3 == 2 ? -1 : 0);
    set(jr(px + nx * z), jr(py + ny * z), 3);
  }
  const bool chomp = t >= 1.4 && t < 2.6 && md(t - 1.4, 0.4) < 0.2;
  const bool open = (t >= 1.25 && t < 1.4) || chomp || (t >= 5.0 && t < 5.4);
  put(open ? X_SPIKY_HEADO : X_SPIKY_HEAD, hx, hy, PO().S(t >= 5.4 && t < 6.2));
  if (chomp) { set(fcx + 5 + jr(rnd(fl(t * 10)) * 2), 21, 3); set(fcx - 6, 22 - jr(rnd(fl(t * 10) + 3) * 2), 3); }
  if (t >= 2.95 && t < 3.6) { const D k = pr(t, 2.95, 3.6); disc(jr(bcx), jr(lerp(restY + 4, 24, k)), 1, 2); }
  if (t >= 3.6 && t < 5.0) disc(jr(bcx), 24, jr(1 + ez(pr(t, 3.6, 4.4))), 2);
  if (t >= 5.0 && t < 5.6) { const D k = pr(t, 5.0, 5.6); ring(jr(restX + 14 + k * 4), jr(restY + 2 - k * 6), 1, 3); }
  if (t > 5.8 && fl(t * 5) % 2 == 0) spark(x0 + 15, restY);
}

// BLOBBERT: absorbs it... ah... ah... CHOO! Sneezes it across the room, charges, absorbs it properly. Wobbly.
void blobbert(D t) {
  const D fcx = 29, fx2 = 8;
  D cx = 10.5, w = 15, h = 12; bool shut = false;
  if (t < 0.4) food(fcx, dropY(t, 0, 0.4, 23), 0);
  if (t >= 0.4 && t < 1.2) { const D k = pr(t, 0.4, 1.2); cx = lerp(10.5, fcx, ez(k)); w = 15 + jr(sin(k * PI) * 3); h = 12 - jr(sin(k * PI) * 2); }
  else if (t >= 1.2 && t < 2.6) cx = fcx;
  if (t >= 0.4 && t < 1.2) food(fcx, 23, 0);
  if (t >= 1.2 && t < 2.6) { const D k = ez(pr(t, 1.2, 1.9)); food(fcx, jr(lerp(23, 20, k)), t < 1.7 ? 0 : 1, PO().Mi(true)); }
  if (t >= 2.0 && t < 2.3) { w = 16; h = 13; }
  if (t >= 2.3 && t < 2.6) { w = 17; h = 14; shut = true; }
  if (t >= 2.6 && t < 2.8) { w = 19; h = 9; cx = fcx + 2; shut = true; }
  if (t >= 2.8 && t < 3.6) { cx = fcx + jr(2 * (1 - pr(t, 2.8, 3.2))); w = 15 + jr(sin((t - 2.8) * 20) * exp(-(t - 2.8) * 4) * 2); }
  if (t >= 2.6 && t < 3.6) {
    const D k = pr(t, 2.6, 3.15), k2 = pr(t, 3.15, 3.45);
    const D fy = t < 3.15 ? 20 - sin(k * PI) * 6 + k * 3 : 23 - sin(k2 * PI) * 2;
    food(t < 3.15 ? lerp(fcx, fx2, k) : fx2, jr(fy), 0);
    if (t < 2.9) for (int i = 0; i < 4; i++) set(jr(fcx - 9 - i * 2 - (t - 2.6) * 20), 18 + (i % 3) * 2, 2);
  }
  if (t >= 3.3 && t < 3.42) shut = true;
  if (t >= 3.6 && t < 4.2) { const D k = pr(t, 3.6, 4.2); cx = lerp(fcx, fx2, ez(k)); w = 19 - jr(fabs(k - 0.5) * 6); h = 10 + jr(fabs(k - 0.5) * 3); food(fx2, 23, 0); }
  if (t >= 4.2) {
    cx = fx2;
    const int st = t < 4.6 ? 0 : t < 5.0 ? 1 : t < 5.3 ? 2 : 3;
    if (st < 3) food(fx2, 21, st, PO().Mi(true));
    const D g = ez(pr(t, 4.2, 5.3));
    w = 15 + g * 3; h = 12 + g * 2;
    if (t >= 5.3) { const D k = t - 5.3, a = exp(-k * 1.6); w = 17 + sin(k * 13) * 2.4 * a; h = 13 - sin(k * 13) * 1.6 * a; shut = k > 0.3 && k < 1.1; }
  }
  floorWarp(SPR_S0_IDLE, cx, jr(w), jr(h), 0, PO().S(shut));
  if (t >= 2.6 && t < 2.85) spark(jr(cx - w / 2) - 5, 14);
  if (t > 6.2 && fl(t * 5) % 2 == 0) spark(jr(cx + 10), 13);
}

// (Mochi is drawn with mochiShape / mochiFace, which at 13 x 8 with ears of 2 are the sprite dot for dot, so the
// body can squash, stretch and grow a pouch and still come back to exactly what it started as.)
void mochiShape(D l, D w, D top, D bot, D ear) {   // the outline: l..l+w-1, top..bot, ears "ear" dots tall (0: none)
  l = jr(l); w = jr(w); top = jr(top); bot = jr(bot); ear = jr(ear);
  const D r = l + w - 1, pl = l + 3, pr_ = r - 3;
  for (D x = l + 2; x <= r - 2; x++) if (ear <= 0 || (fabs(x - pl) >= ear && fabs(x - pr_) >= ear)) set(x, top, 3);
  if (ear > 0) for (D p : {pl, pr_}) for (D dx = -ear; dx <= ear; dx++) set(p + dx, top - (ear - fabs(dx)), 3);
  set(l + 1, top + 1, 3); set(r - 1, top + 1, 3);
  for (D y = top + 2; y <= bot - 2; y++) { set(l, y, 3); set(r, y, 3); }
  set(l + 1, bot - 1, 3); set(r - 1, bot - 1, 3);
  for (D x = l + 2; x <= r - 2; x++) set(x, bot, 3);
}
// the face: eyes on row ey (0 open, 1 shut and happy), cheeks and mouth on row cy
void mochiFace(D fx, D ey, D cy, D l, D r, int eyes, D look = 0) {
  fx = jr(fx); ey = jr(ey); cy = jr(cy); l = jr(l); r = jr(r);
  for (D ex : {fx - 3 + look, fx + 3 + look}) {
    if (eyes == 0) set(ex, ey, 3);
    else { set(ex - 1, ey, 3); set(ex, ey - 1, 3); set(ex + 1, ey, 3); }
  }
  set(l + 2, cy, 3); set(l + 3, cy, 3); set(r - 3, cy, 3); set(r - 2, cy, 3);
  set(fx, cy, 3); set(fx - 1, cy + 1, 3); set(fx + 1, cy + 1, 3);
}
void mochiRest(D l, D lift, int eyes, D ear) {   // as the sprite stands
  mochiShape(l, 13, 20 - lift, 27 - lift, ear);
  mochiFace(l + 6, 22 - lift, 24 - lift, l, l + 12, eyes);
}

void dumpling(D cx, D w, D top, D ry, D open) {   // flat on the floor, a domed top; open: the gap left in the dome (dots from the middle, each side)
  const D rx = (w - 1) / 2, l = jr(cx - rx), r = jr(cx + rx), sy = jr(top + ry);
  for (D y = sy; y <= 25; y++) { set(l, y, 3); set(r, y, 3); }
  set(l + 1, 26, 3); set(r - 1, 26, 3);
  for (D x = l + 2; x <= r - 2; x++) set(x, 27, 3);
  D py = sy;
  for (D x = l; x <= cx; x++) {   // the dome, a column at a time (and its mirror image, so the two sides match dot for dot)
    const D nx = (cx - x) / rx, y = jr(sy - ry * sqrt(mx(0, 1 - nx * nx)));
    if (cx - x < open) break;
    for (D yy = py - 1; yy >= y; yy--) { set(x, yy, 3); set(l + r - x, yy, 3); }
    if (y == py) { set(x, y, 3); set(l + r - x, y, 3); }
    py = y;
  }
}
// MOCHI: the daifuku wrap. Squashes into a low bun, slides under the food, the sides grow up round it and close
// over the top (a pinch and a twist); three gulps take the filling down, and the twist pops undone into two ears.
void mochi(D t) {
  const D fcx = 27, x0 = 10;
  if (t < 1.8) {   // squash, slide under
    const D k = ez(pr(t, 0.5, 1.0)), s = ez(pr(t, 1.0, 1.7));
    const D w = 13 + 2 * jr(2 * k), hb = 8 - jr(2 * k), ear = jr(2 * (1 - pr(t, 0.5, 0.85)));
    const D cx = jr(lerp(x0, fcx, s)), l = cx - (w - 1) / 2, top = 28 - hb;
    D fy = t < 0.4 ? dropY(t, 0, 0.4, 23) : 23;
    if (t >= 1.25) { const D h = pr(t, 1.25, 1.65); fy = jr(lerp(23, 17, h) - sin(h * PI) * 3); }   // up it goes, onto their head
    food(fcx, fy, 0);
    mochiShape(l, w, top, 27, ear);
    mochiFace(cx, top + 2, top + (hb >= 8 ? 4 : 3), l, l + w - 1, 0, t > 0.45 && t < 1.7 ? 1 : 0);
    return;
  }
  const D cx = fcx;
  if (t < 2.6) {   // the sides grow up and close over it
    const D k1 = pr(t, 1.8, 2.15), k2 = ez(pr(t, 2.15, 2.6));
    food(cx, 17, 0);
    for (D x = cx - 7; x <= cx + 7; x++) set(x, 22, 3);   // (what it's sitting on)
    if (k1 < 1) { const D up = jr(lerp(22, 17, k1)); for (D y = up; y <= 25; y++) { set(cx - 8, y, 3); set(cx + 8, y, 3); } set(cx - 7, 26, 3); set(cx + 7, 26, 3); for (D x = cx - 6; x <= cx + 6; x++) set(x, 27, 3); }
    else dumpling(cx, 17, 12, 5, jr(8 * (1 - k2)));
    mochiFace(cx, 24, 25, cx - 8, cx + 8, 0);
    return;
  }
  static const D gulps[3] = {3.0, 3.5, 4.0};
  D G = 0; for (D g : gulps) G += ez(pr(t, g, g + 0.3));
  const int st = stepIn(t, gulps, 3);
  if (t < 4.4) {   // wrapped: a twist on top, and down it goes
    const D f = G / 3, w = 13 + 2 * jr(2 * (1 - f)), top = jr(lerp(12, 20, f)), ey = jr(lerp(24, 22, f));
    D sq = 0; for (D g : gulps) if (t >= g && t < g + 0.12) sq = 1;   // each gulp: a dot wider for a moment
    if (st < 3) food(cx, 17 + (st ? 1 : 0), st, PO().Mi(true));
    if (f < 1) dumpling(cx, w + 2 * sq, top, jr(lerp(5, 2, f)), 0); else mochiShape(cx - 6, 13, 20, 27, 0);
    mochiFace(cx, ey, ey + (f > 0.8 ? 2 : 1), cx - (w - 1) / 2, cx + (w - 1) / 2, sq ? 1 : 0);
    set(cx, top - 1, 3); set(cx - 1, top - 2, 3); set(cx + 1, top - 2, 3); if (t < 3.0) set(cx, top - 3, 3);   // the twist
    if (t < 2.8) spark(cx + 3, 7);
    return;
  }
  // the twist pops undone: two ears
  const D k = pr(t, 4.4, 4.8), ear = t < 4.8 ? jr(lerp(0, 2, k) + sin(k * PI) * 1.6) : 2;
  const D lift = t >= 4.9 && t < 5.3 ? hop(pr(t, 4.9, 5.3), 1, 2) : 0;
  mochiRest(cx - 6, lift, t >= 4.9 && t < 5.6 ? 1 : 0, ear);
  if (t < 4.8) { set(cx - 2 - jr(k * 5), 17 - jr(k * 6), 2); set(cx + 2 + jr(k * 5), 17 - jr(k * 6), 2); }
  if (t > 5.0 && fl(t * 5) % 2 == 0) spark(cx + 8, 15);
}

// CHEEKS: chipmunk nibbling, cheeks swell till he floats off the ground; GULP and down he drops.
void cheeks(D t) {
  const D fcx = 28, y0 = 18;
  D x = 3, lift = 0, sq = 0;
  if (t < 0.4) food(fcx, dropY(t, 0, 0.4, 23), 0);
  if (t >= 0.4 && t < 1.0) { const D k = pr(t, 0.4, 1.0); x = jr(lerp(3, 12, ez(k))); lift = hop(k, 2, 2); }
  else if (t >= 1.0) x = 12;
  const int st = t < 1.6 ? 0 : t < 2.2 ? 1 : t < 2.8 ? 2 : 3;
  if (t >= 0.4 && st < 3) food(fcx, 23, st);
  const bool nib = t >= 1.0 && t < 2.8;
  D r = 0;
  if (t >= 1.0 && t < 4.2) r = mn(4, 0.5 + pr(t, 1.0, 2.8) * 3.5);
  if (t >= 4.2 && t < 4.35) r = 4 * (1 - pr(t, 4.2, 4.35));
  if (t >= 2.8 && t < 4.2) lift = jr(ez(pr(t, 2.8, 3.8)) * 9 + sin(t * 5) * 1);
  if (t >= 4.2 && t < 4.5) { const D k = pr(t, 4.2, 4.5); lift = jr(9 * (1 - k * k)); }
  if (t >= 4.5 && t < 4.65) sq = 1;
  const D top = 28 - 10 - lift;
  const D shake = nib ? (D)(fl(t * 24) % 2) : 0;
  const bool shut = t >= 4.7 && t < 5.6;
  if (sq != 0) floorWarp(SPR_S0C_IDLE, x + 5.5, 13, 8, 0, PO().S(shut)); else put(SPR_S0C_IDLE, x + shake, top, PO().S(shut));
  if (nib && fl(t * 12) % 2 == 0) { set(x + 4 + shake, top + 8, 3); set(x + 6 + shake, top + 8, 3); set(x + 5 + shake, top + 9, 0); }
  if (nib) for (int i = 0; i < 2; i++) { const D k = md(t * 3 + i * 0.5, 1); set(jr(fcx - 4 + (i ? 10 : -3) * k), jr(22 - sin(k * PI) * 4 + k * 4), 3); }
  const D R = jr(r);
  if (R > 0 && sq == 0) { const D o = jr(R * 0.6); ring(x + 1 - o + shake, top + 6, R, 3, 0); ring(x + 9 + o + shake, top + 6, R, 3, 0); }
  if (lift > 1 && t < 4.2) { set(x + 3, top + 10, 3); set(x + 7, top + 10 + (fl(t * 6) % 2), 3); }
  if (t >= 4.2 && t < 4.45) spark(x + 12, top - 2);
  if (t >= 4.6 && t < 5.0) { const D k = pr(t, 4.6, 5.0); set(x + 2 + jr(k * 7), 13 - jr(sin(k * PI) * 2), 3); }
  if (t > 5.4 && fl(t * 5) % 2 == 0) spark(x + 12, y0 - 2);
}

// BIG BLOBBERT: a trampoline. The food lands on his head, BOING, caught, sinks through him (eyes follow), burp shake.
void bigBlobbert(D t) {
  const D cx = 20;
  D h = 25, w = 36, fy = 0, ey = 0; bool hasF = false, mid = false, shut = false; int st = 0, shake = 0;
  if (t < 0.5) h = jr(lerp(25, 21, ez(pr(t, 0, 0.5))));
  else if (t < 1.0) h = 21;
  else if (t < 1.3) h = jr(lerp(21, 24, pr(t, 1.0, 1.15)));
  else if (t < 1.7) h = t < 1.6 ? 24 : 22;
  else if (t < 3.8) h = 22;
  if (t >= 3.8 && t < 4.6) { const D k = t - 3.8; h = jr(22 + sin(k * 15) * 2 * exp(-k * 3)); w = jr(36 - sin(k * 15) * 1.5 * exp(-k * 3)); }
  if (t >= 4.6 && t < 5.0) { h = 24; w = 37; }
  if (t >= 5.0) { h = 22; w = 36; }
  if (t >= 5.0 && t < 5.5) shake = fl(t * 20) % 2 ? 1 : -1;
  const D top = 28 - h;
  if (t >= 0.2 && t < 1.0) { fy = dropY(t, 0.2, 0.4, 28 - 21 - 4); hasF = true; }   // falls onto the top of his head, and rests there
  if (t >= 1.0 && t < 1.6) { const D k = pr(t, 1.0, 1.6); fy = jr(lerp(3, 28 - 24 - 4, k) - sin(k * PI) * 9); hasF = true; }
  if (t >= 1.6 && t < 3.8) { const D k = pr(t, 1.7, 3.8); fy = jr(lerp(top - 4, 18, k)); mid = fy > top - 1; st = t < 2.7 ? 0 : t < 3.3 ? 1 : 2; hasF = true; }
  if (hasF) ey = mx(-2, mn(2, jr((fy - 9) / 4)));
  if (t < 0.3) ey = -2;
  if (t >= 5.6) shut = true;
  const D bx = jr(cx + 0.5 - w / 2);
  if (hasF && mid) food(cx, fy, st, PO().Mi(true));   // once it is inside him it is drawn first, so his outline is never broken by it
  // His body, squashed to w x h. Not warp(), which drops outline dots when shrinking: here every dot of the outline
  // is put where it lands, so a joined-up line stays joined up (warp's dots are added when stretching).
  {
    const Spr s = sp(SPR_BB_BLOBBERT); const D ww = mx(1, jr(w)), hh = mx(1, jr(h));
    for (D dy = 0; dy < hh; dy++) for (D dx = 0; dx < ww; dx++) {
      const char ch = at(s, (int)(hh > 1 ? jr(dy * (s.h - 1) / (hh - 1)) : 0), (int)(ww > 1 ? jr(dx * (s.w - 1) / (ww - 1)) : 0));
      if (ch == 'G' || (ch == 'X' && (ww > s.w || hh > s.h))) set(bx + dx, top + dy, ch == 'G' ? 2 : 3);
    }
    for (int r = 0; r < s.h; r++) for (int c = 0; c < s.w; c++) if (at(s, r, c) == 'X')
      set(bx + jr(c * (ww - 1) / (s.w - 1)), top + jr(r * (hh - 1) / (s.h - 1)), 3);
  }
  const D eyy = top + jr(10 * (h - 1) / 24);
  for (D ex : {bx + jr(11 * (w - 1) / 35), bx + jr(24 * (w - 1) / 35)}) {
    if (shut) { set(ex - 1, eyy + 2, 3); set(ex, eyy + 1, 3); set(ex + 1, eyy + 2, 3); }
    else for (int i = 0; i < 3; i++) set(ex, eyy + i + ey, 3);
  }
  if (hasF && !mid) food(cx, fy, st);
  if (t >= 1.0 && t < 1.2) { spark(bx + 4, top - 2); spark(bx + w - 9, top - 2); }
  if (t >= 5.0 && t < 6.4) { const D k = pr(t, 5.0, 6.4); const D by = jr(top - 2 - k * 10); if (by > -3) bubble(cx - 1 + jr(sin(k * 8)), by); }
  if (t > 6.0 && fl(t * 5) % 2 == 0) spark(bx + w - 7, top + 1);
  shiftBuf(shake);
}

void bigShell(D cx, D cy, D ang, bool hatchOn) {
  const D R = 13, hatch = -PI / 2;   // the hatch sits at the top of the ball
  for (D dy = -R - 1; dy <= R + 1; dy++) for (D dx = -R - 1; dx <= R + 1; dx++) {
    if (!inRim(dx * dx + dy * dy, R)) continue;
    if (hatchOn) {   // (the angle is only worked out while the hatch is open)
      const D th = atan2(dy, dx);
      if (fabs(md(md(th - hatch, 2 * PI) + 3 * PI, 2 * PI) - PI) < 0.24) continue;
    }
    set(cx + dx, cy + dy, 3);
  }
  for (D o : {0.0, PI}) for (D rr : {11.0, 12.0}) set(jr(cx + cos(ang + o) * rr), jr(cy + sin(ang + o) * rr), 3);
  static const D gl[4][2] = {{-2.45, 11}, {-2.3, 11}, {-2.15, 11}, {-2.4, 10}};
  for (const auto& g : gl) set(jr(cx + cos(g[0]) * g[1]), jr(cy + sin(g[0]) * g[1]), 2);
}
// BIG BALLBOI: rolls onto the food (squashed flat, stuck to the ball), rolls back half a turn so the pancake rides
// up the back to the top, a hatch opens, it drops onto his head and into his mouth.
void bigBallboi(D t) {
  const FDs F = FD(); const D fcx = 26, cy = 14, R = 13, M = 1.6;
  D cx = 13, lean = 0, bob = 0; bool open = false;
  const D xs[3] = {13, 26, 13};
  if (t >= 0.5 && t < 1.6) cx = lerp(xs[0], xs[1], ez(pr(t, 0.5, 1.6)));
  else if (t >= 1.6 && t < 2.0) cx = xs[1];
  else if (t >= 2.0 && t < 3.5) { const D k = pr(t, 2.0, 3.5); cx = lerp(xs[1], xs[2], k < 0.8 ? k * 0.9 / 0.8 : 0.9 + (1 - pow(1 - (k - 0.8) / 0.2, 2)) * 0.1); }
  else if (t >= 3.5) cx = xs[2];
  const D M2 = PI * R / (xs[1] - xs[2]);
  const D dR = t >= 1.6 ? xs[1] - xs[0] : cx - xs[0], dL = t >= 2.0 ? xs[1] - cx : 0;
  const D angMax = (xs[1] - xs[0]) / R * M;
  const D ang = dR / R * M - dL / R * M2;
  const bool moving = (t >= 0.5 && t < 1.6) || (t >= 2.0 && t < 3.5);
  if (moving) { lean = t < 1.6 ? 1 : -1; bob = fl(t * 10) % 2 ? -1 : 0; }
  const D peel = 3.75;
  if (t < 0.4) food(fcx, dropY(t, 0, 0.4, 23), 0);
  else if (t < 1.6) {   // squished as the ball rolls over it: every column of it is pressed into what room there is under the ball there
    const Spr s = sp(F.n[0]); const D fx0 = jr(fcx - F.w / 2);
    for (int c = 0; c < s.w; c++) {
      const D dx = fx0 + c - jr(cx), in2 = (R - 1) * (R - 1) - dx * dx;
      const D room = in2 > 0 ? 27 - (cy + sqrt(in2)) : s.h;          // dots between the floor and the ball, in this column
      const int hh = (int)mx(1, mn(s.h, floor(room + 0.5)));
      for (int j = 0; j < hh; j++) {
        bool on = false;   // (pressed flat, a column keeps a dot if it had any)
        if (hh == 1) { for (int r = 0; r < s.h; r++) if (at(s, r, c) != '.') on = true; }
        else on = at(s, (int)jr(j * (s.h - 1.0) / (hh - 1)), c) != '.';
        if (on) set(fx0 + c, 27 - hh + 1 + j, 3);
      }
    }
  }
  const bool hasPan = t >= 1.6 && t < peel; const D pan = PI / 2 + (ang - angMax);
  bigShell(jr(cx), cy, ang, t >= 3.6 && t < peel + 0.5);
  if (hasPan) {
    for (int i = -3; i <= 3; i++) { const D a = pan + i / 13.0; set(jr(cx + cos(a) * 13), jr(cy + sin(a) * 13), 2); }
    for (int i = -2; i <= 2; i++) { const D a = pan + i / 14.0; set(jr(cx + cos(a) * 14), jr(cy + sin(a) * 14), 3); }
  }
  const D hx = jr(cx) - 8 + lean, hy = 9 + bob;
  if (t >= 1.6 && t < 2.0) { set(hx + 5, hy + 8, 3); set(hx + 11, hy + 8, 3); }
  put(SPR_BB_BALL_HAM, hx, hy, PO().S(t >= 5.8 && t < 6.8));
  if (t >= peel && t < 4.05) { const D k = pr(t, peel, 4.05); warp(F.n[0], jr(cx) - 2, jr(lerp(1, hy - 2, k * k)), 5, 2); }
  if (t >= 4.05 && t < 4.3) { warp(F.n[0], hx + 6, hy - 2, 5, 2); spark(hx + 13, hy - 4); }
  if (t >= 4.3 && t < 4.6) { const D k = pr(t, 4.3, 4.6); warp(F.n[0], hx + 6, jr(lerp(hy - 1, hy + 9, k)), 5, 2); open = true; }
  const bool chew = t >= 4.6 && t < 5.8; const D ck = (t - 4.6) / 0.4;
  if (chew) { const D cst = mn(2, floor(ck)); if (cst < 2 && md(ck, 1) < 0.5) warp(F.n[(int)cst + 1], hx + 7, hy + 10, 3, 2, PO().Mi(true)); open = md(ck, 1) < 0.5; }
  if (open) { set(hx + 7, hy + 11, 3); set(hx + 9, hy + 11, 3); set(hx + 8, hy + 12, 3); }
  if (t > 6.2 && fl(t * 5) % 2 == 0) spark(jr(cx) + 14, 3);
}

// ALIEN CAT: eyes glow, a tractor beam lifts the food to his mouth, three bites, a hiccup spins the bow tie,
// a paw straightens it, and he burps out a tiny spiral galaxy.
void alien(D t) {
  const D x = 3, y = 3, fcx = 31;
  const bool glow = t >= 0.7 && t < 3.9;
  put(SPR_BB_ALIEN, x, y, PO().NoE());
  const Spr Ra = sp(SPR_BB_ALIEN);
  const bool blink = t >= 5.0 && t < 5.15;
  for (int r = 0; r < Ra.h; r++) for (int c = 0; c < Ra.w; c++) if (at(Ra, r, c) == 'E') {
    if (blink) continue;
    const int v = glow ? (fl(t * 8 + c + r) % 3 == 0 ? 2 : 3) : 3; set(x + c, y + r, v);
  }
  if (blink) for (int ex : {4, 10}) for (int i = 0; i < 4; i++) set(x + ex + i, y + 10, 3);
  D fx = fcx, fy = 23; int st = 0;
  if (t < 0.4) fy = dropY(t, 0, 0.4, 23);
  if (t >= 1.2 && t < 2.6) { const D k = ez(pr(t, 1.2, 2.6)); fx = lerp(fcx, x + 22, k); fy = lerp(23, y + 10, k) + sin(t * 9) * 0.8; }
  static const D bites[3] = {2.8, 3.25, 3.7};
  if (t >= 2.6) { st = stepIn(t, bites, 3); fx = x + 22 - st * 1.5; fy = y + 10; }
  if (st < 3) food(jr(fx), jr(fy), st);
  if (glow && t >= 1.0 && st < 3) {
    for (D ex : {x + 5, x + 12}) {
      const D eyv = y + 9.5, tx = fx, ty = fy + 2, n = jr(hypot(tx - ex, ty - eyv));
      for (D i = 2; i < n - 3; i++) if (md(i + floor(t * 12), 3) != 0) set(jr(lerp(ex, tx, i / n)), jr(lerp(eyv, ty, i / n)), 2);
    }
  }
  bool open = false;
  if (st < 3 && t >= 2.6) for (D b : bites) if (t >= b - 0.2 && t < b) open = true;
  if (open || (t >= 5.2 && t < 5.5)) { set(x + 7, y + 14, 3); set(x + 8, y + 14, 0); set(x + 9, y + 14, 3); set(x + 8, y + 15, 3); set(x + 7, y + 13, 0); set(x + 9, y + 13, 0); }
  static const int bt[3][7][2] = {
    {{-1, -1}, {-1, 0}, {-1, 1}, {0, 0}, {1, -1}, {1, 0}, {1, 1}},
    {{-1, -1}, {0, -1}, {1, -1}, {0, 0}, {-1, 1}, {0, 1}, {1, 1}},
    {{-1, 0}, {-2, 1}, {0, 0}, {1, -1}, {1, 0}, {2, -1}, {-1, 1}}};
  long bi = 0;
  if (t >= 3.95 && t < 4.4) bi = fl(t * 14) % 2;
  else if (t >= 4.4 && t < 4.95) bi = 2;
  for (const auto& p : bt[bi]) set(x + 8 + p[0], y + 16 + p[1], 2);
  if (t >= 3.9 && t < 4.05) spark(x + 15, y + 12);
  if (t >= 4.5 && t < 5.1) { const D k = pr(t, 4.5, 4.8), k2 = pr(t, 4.85, 5.1); const D px = jr(lerp(x + 2, x + 6, k) - k2 * 4), py = jr(lerp(y + 21, y + 17, k) + k2 * 4); rect(px, py, 2, 2, 0); set(px - 1, py, 3); set(px + 2, py, 3); }
  if (t >= 4.9 && t < 5.15) spark(x + 12, y + 14);
  if (t >= 5.3 && t < 6.8) {
    const D k = pr(t, 5.3, 6.8), gx = lerp(x + 10, x + 26, k), gy = lerp(y + 14, y + 3, k), rot = t * 5, rr = mn(4, 1 + k * 6);
    set(jr(gx), jr(gy), 3);
    for (int i = 0; i < 8; i++) { const D a = rot + i * 0.8, r2 = rr * (0.35 + i / 10.0); set(jr(gx + cos(a) * r2), jr(gy + sin(a) * r2 * 0.6), i % 2 ? 2 : 3); }
    for (int i = 0; i < 4; i++) if (fl(t * 6 + i) % 3 == 0) set(jr(gx + (rnd(i) - 0.5) * 16), jr(gy + (rnd(i + 9) - 0.5) * 10), 3);
  }
}

// ---- the table
struct Entry { void (*fn)(D); float dur; float cues[6]; int ncue; };
constexpr Entry TABLE[] = {   // in Routine enum order
  {nullptr, 0, {0}, 0},
  {wobbles, 7.6f, {3.0f, 5.0f}, 2},
  {jelly, 7.0f, {2.2f, 5.25f}, 2},
  {puddle, 6.6f, {2.0f, 2.6f, 3.2f}, 3},
  {toasty, 6.6f, {3.7f, 4.3f, 4.9f}, 3},
  {marshy, 6.2f, {2.75f, 3.35f, 3.95f}, 3},
  {boop, 7.2f, {2.0f, 4.7f}, 2},
  {curly, 6.8f, {1.7f, 4.7f}, 2},
  {puff, 6.6f, {2.6f, 3.0f}, 2},
  {loaf, 7.0f, {2.0f, 2.4f}, 2},
  {nimbus, 7.4f, {3.3f}, 1},
  {ballboi, 8.4f, {3.35f, 3.85f, 4.35f, 6.05f}, 4},   // BB_T0 + 0.1, 0.6, 1.1, 2.8
  {sprout, 7.2f, {1.7f, 2.1f, 2.5f}, 3},
  {fubz, 7.0f, {0.4f, 1.2f, 2.0f, 4.1f}, 4},
  {dapper, 7.2f, {3.4f, 4.6f}, 2},
  {bean, 6.4f, {1.85f, 2.3f, 3.05f, 3.9f, 4.85f}, 5},
  {bun, 6.4f, {1.9f, 3.4f, 4.0f, 4.6f}, 4},
  {slosh, 6.8f, {1.5f, 2.0f, 2.5f, 4.75f}, 4},
  {mallow, 6.6f, {2.4f, 3.1f, 3.8f}, 3},
  {spiky, 6.4f, {1.4f, 1.8f, 2.2f, 5.0f}, 4},
  {blobbert, 6.8f, {1.2f, 2.6f, 4.6f, 5.0f}, 4},
  {mochi, 5.8f, {2.6f, 3.0f, 3.5f, 4.0f}, 4},
  {cheeks, 6.2f, {1.6f, 2.2f, 2.8f, 4.2f}, 4},
  {bigBlobbert, 6.8f, {1.0f, 2.7f, 3.3f, 5.0f}, 4},
  {bigBallboi, 7.0f, {1.45f, 4.05f, 4.6f, 5.0f, 5.4f}, 5},
  {alien, 6.9f, {2.8f, 3.25f, 3.7f, 5.3f}, 4},
};
static_assert(sizeof(TABLE) / sizeof(TABLE[0]) == R_COUNT, "one TABLE entry per Routine");
// each entry's ncue is the number of leading non-zero cues
constexpr bool cuesMatch() {
  for (const Entry& e : TABLE) {
    int n = 0;
    while (n < 6 && e.cues[n] > 0.0f) n++;
    if (n != e.ncue) return false;
  }
  return true;
}
static_assert(cuesMatch(), "ncue must match the cues listed");

}  // namespace

float duration(int routine) { return routine > R_NONE && routine < R_COUNT ? TABLE[routine].dur : 0.0f; }

bool render(int routine, double t, bool meal, int variant, uint8_t* buf) {
  B = buf; MEAL = meal; MV = variant; INV = false;
  memset(B, 0, EW * EH);
  if (routine > R_NONE && routine < R_COUNT) {
    const D d = duration(routine);
    TABLE[routine].fn(t < d ? t : d);
  }
  return INV;
}

const float* chomps(int routine, int& count) {
  if (routine <= R_NONE || routine >= R_COUNT) { count = 0; return nullptr; }
  count = TABLE[routine].ncue;
  return TABLE[routine].cues;
}

}  // namespace eatfx
}  // namespace lf
