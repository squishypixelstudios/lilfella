// 2-bit (4-shade) frame for the classic LCD look.
//   Frame:  184x224 logical pixels, one byte each holding a shade 0..3, shown at 2x on the 368x448 AMOLED.
//   Matrix: the pet's dot-matrix "LCD": 40x28 dots, each dot 4x4 logical px (3x3 lit + 1 px gap).
// Shades: 0 = LCD background, 1 = faint (unlit segments / ghost dots), 2 = mid grey, 3 = black.
#pragma once
#include <stdint.h>
#include <string.h>
#include "sprites.h"

namespace lf {

constexpr int W = 184;   // logical width  (physical 368 / 2)
constexpr int H = 224;   // logical height (physical 448 / 2)

// dot matrix placement (logical px)
constexpr int MW = 40, MH = 28;          // dots
constexpr int DOT = 4;                   // logical px per dot (pitch)
constexpr int MX = (W - MW * DOT) / 2;   // 12
constexpr int MY = 56;                   // top of the matrix
constexpr int MY1 = MY + MH * DOT;      // 168: bottom of the matrix
constexpr int MHX = 44;                  // the most rows the matrix can be given (POP plays on a taller one)

enum Shade : uint8_t { SH_BG = 0, SH_FAINT = 1, SH_MID = 2, SH_INK = 3 };

// 4-entry RGB565 palettes (index = shade)
struct Palette { uint16_t c[4]; };
extern const Palette PAL_LCD;     // classic: light grey LCD, black ink
extern const Palette PAL_NIGHT;   // inverted: black background, light ink (easier on the AMOLED battery)

class Frame {
 public:
  uint8_t px[W * H];
  void clear(uint8_t s) { memset(px, s, sizeof px); }
  inline void set(int x, int y, uint8_t s) { if ((unsigned)x < (unsigned)W && (unsigned)y < (unsigned)H) px[y * W + x] = s; }
  inline uint8_t get(int x, int y) const { return ((unsigned)x < (unsigned)W && (unsigned)y < (unsigned)H) ? px[y * W + x] : 0; }
  void rect(int x, int y, int w, int h, uint8_t s);
  void roundFrame(int x, int y, int w, int h, uint8_t s);
  void hline(int x, int y, int w, uint8_t s) { rect(x, y, w, 1, s); }
  void vline(int x, int y, int h, uint8_t s) { rect(x, y, 1, h, s); }
  void ring(int cx, int cy, int r, uint8_t s);
  // 5x7 text, 6 px advance per char at scale 1
  void text(int x, int y, const char* s, uint8_t sh, int scale = 1);
  void textCenter(int cx, int y, const char* s, uint8_t sh, int scale = 1);
  static int textWidth(const char* s, int scale = 1);
  // sprite at logical px scale (icons): shade for "on" pixels (grey dots a shade lighter); off pixels untouched
  void sprite(int x, int y, int id, uint8_t sh, int scale = 1, bool flip = false);
};

// The dot-matrix LCD. Dots hold a shade (0 = unlit).
class Matrix {
 public:
  uint8_t d[MW * MHX];
  int h = MH;    // rows in use: MH, but a game may ask for a taller LCD (POP: up to MHX)
  int y0 = MY;   // where its top is in the frame (logical px)
  void clear() { memset(d, 0, sizeof d); }
  inline void set(int x, int y, uint8_t s) { if ((unsigned)x < (unsigned)MW && (unsigned)y < (unsigned)h) d[y * MW + x] = s; }
  inline uint8_t get(int x, int y) const { return ((unsigned)x < (unsigned)MW && (unsigned)y < (unsigned)h) ? d[y * MW + x] : 0; }
  void rect(int x, int y, int w, int h, uint8_t s);
  // sprite in dots. mode: 0 normal, 1 eyes closed. fill: shade for the body inside (0 = none)
  // mode: 0 awake, 1 eyes closed, 2 mouth open. fill: shade for the body inside the outline (0 = none).
  // sick: the body gets a sickly speckle (every other dot inside the outline, in `sick` shade; 0 = not sick)
  void sprite(int x, int y, int id, bool flip = false, int mode = 0, uint8_t fill = 0, uint8_t ink = SH_INK, uint8_t sick = 0);
  // 5x7 text in dots (6 dot advance)
  void text(int x, int y, const char* s, uint8_t sh = SH_INK);
  void textCenter(int y, const char* s, uint8_t sh = SH_INK);
  // 3x5 text in dots (4 dot advance)
  void small(int x, int y, const char* s, uint8_t sh = SH_INK, int spaceAdv = 4);
  void smallCenter(int y, const char* s, uint8_t sh = SH_INK);
  static int textWidth(const char* s) { int n = 0; while (s[n]) n++; return n ? n * 6 - 1 : 0; }
  static int smallWidth(const char* s, int spaceAdv = 4);
  // draw into the frame. offShade: shade of unlit dots; bgShade: the gaps between dots
  void render(Frame& f, uint8_t offShade, uint8_t bgShade) const;
};

inline int spriteW(int id) { return SPRITES[id].w; }
inline int spriteH(int id) { return SPRITES[id].h; }

}  // namespace lf
