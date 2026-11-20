#include "lcd.h"
#include "font5x7.h"

namespace lf {

static constexpr uint16_t c565(int r, int g, int b) { return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)); }

const Palette PAL_LCD = {{c565(198, 201, 190), c565(180, 184, 172), c565(112, 116, 106), c565(30, 33, 28)}};
const Palette PAL_NIGHT = {{c565(0, 0, 0), c565(30, 32, 29), c565(104, 108, 100), c565(214, 218, 205)}};

// ---------------------------------------------------------------------------------------------
// 3x5 font (digits, capitals, a few marks). Each glyph: 5 rows, 3 bits per row (bit2 = left).
// ---------------------------------------------------------------------------------------------
struct G35 { char c; uint8_t r[5]; };
static const G35 kFont35[] = {
  {'0', {7, 5, 5, 5, 7}}, {'1', {2, 6, 2, 2, 7}}, {'2', {7, 1, 7, 4, 7}}, {'3', {7, 1, 3, 1, 7}},
  {'4', {5, 5, 7, 1, 1}}, {'5', {7, 4, 7, 1, 7}}, {'6', {7, 4, 7, 5, 7}}, {'7', {7, 1, 1, 2, 2}},
  {'8', {7, 5, 7, 5, 7}}, {'9', {7, 5, 7, 1, 7}},
  {'A', {2, 5, 7, 5, 5}}, {'B', {6, 5, 6, 5, 6}}, {'C', {3, 4, 4, 4, 3}}, {'D', {6, 5, 5, 5, 6}},
  {'E', {7, 4, 6, 4, 7}}, {'F', {7, 4, 6, 4, 4}}, {'G', {3, 4, 5, 5, 3}}, {'H', {5, 5, 7, 5, 5}},
  {'I', {7, 2, 2, 2, 7}}, {'J', {1, 1, 1, 5, 2}}, {'K', {5, 5, 6, 5, 5}}, {'L', {4, 4, 4, 4, 7}},
  {'M', {5, 7, 7, 5, 5}}, {'N', {6, 5, 5, 5, 5}}, {'O', {2, 5, 5, 5, 2}}, {'P', {6, 5, 6, 4, 4}},
  {'Q', {2, 5, 5, 6, 3}}, {'R', {6, 5, 6, 5, 5}}, {'S', {3, 4, 2, 1, 6}}, {'T', {7, 2, 2, 2, 2}},
  {'U', {5, 5, 5, 5, 7}}, {'V', {5, 5, 5, 5, 2}}, {'W', {5, 5, 7, 7, 5}}, {'X', {5, 5, 2, 5, 5}},
  {'Y', {5, 5, 2, 2, 2}}, {'Z', {7, 1, 2, 4, 7}},
  {':', {0, 2, 0, 2, 0}}, {'.', {0, 0, 0, 0, 2}}, {'-', {0, 0, 7, 0, 0}}, {'!', {2, 2, 2, 0, 2}},
  {'?', {6, 1, 2, 0, 2}}, {'/', {1, 1, 2, 4, 4}}, {'+', {0, 2, 7, 2, 0}}, {'x', {0, 5, 2, 5, 0}},
  {'%', {5, 1, 2, 4, 5}}, {'=', {0, 7, 0, 7, 0}}, {'&', {2, 5, 2, 5, 3}}, {'\'', {2, 2, 0, 0, 0}},
};
static const G35* glyph35(char c) {
  if (c >= 'a' && c <= 'z' && c != 'x') c = (char)(c - 32);
  for (const G35& g : kFont35) if (g.c == c) return &g;
  return nullptr;
}

// ---------------------------------------------------------------------------------------------
// Frame
// ---------------------------------------------------------------------------------------------
void Frame::rect(int x, int y, int w, int h, uint8_t s) {
  int x0 = x < 0 ? 0 : x, y0 = y < 0 ? 0 : y, x1 = x + w > W ? W : x + w, y1 = y + h > H ? H : y + h;
  for (int yy = y0; yy < y1; yy++) for (int xx = x0; xx < x1; xx++) px[yy * W + xx] = s;
}
void Frame::roundFrame(int x, int y, int w, int h, uint8_t s) {
  hline(x + 2, y, w - 4, s); hline(x + 2, y + h - 1, w - 4, s);
  vline(x, y + 2, h - 4, s); vline(x + w - 1, y + 2, h - 4, s);
  set(x + 1, y + 1, s); set(x + w - 2, y + 1, s); set(x + 1, y + h - 2, s); set(x + w - 2, y + h - 2, s);
}
void Frame::ring(int cx, int cy, int r, uint8_t s) {
  int x = r, y = 0, e = 1 - r;
  while (x >= y) {
    set(cx + x, cy + y, s); set(cx - x, cy + y, s); set(cx + x, cy - y, s); set(cx - x, cy - y, s);
    set(cx + y, cy + x, s); set(cx - y, cy + x, s); set(cx + y, cy - x, s); set(cx - y, cy - x, s);
    y++;
    if (e < 0) e += 2 * y + 1; else { x--; e += 2 * (y - x) + 1; }
  }
}
void Frame::text(int x, int y, const char* s, uint8_t sh, int scale) {
  for (; *s; s++, x += 6 * scale) {
    const uint8_t* g = &kFont5x7[(uint8_t)*s * 5];
    for (int c = 0; c < 5; c++)
      for (int r = 0; r < 8; r++)   // row 7 holds the descenders (g, j, p, q, y)
        if (g[c] & (1 << r)) rect(x + c * scale, y + r * scale, scale, scale, sh);
  }
}
int Frame::textWidth(const char* s, int scale) { int n = 0; while (s[n]) n++; return n ? (n * 6 - 1) * scale : 0; }
void Frame::textCenter(int cx, int y, const char* s, uint8_t sh, int scale) { text(cx - textWidth(s, scale) / 2, y, s, sh, scale); }

void Frame::sprite(int x, int y, int id, uint8_t sh, int scale, bool flip) {
  const Sprite& sp = SPRITES[id];
  const uint8_t grey = sh == SH_INK ? (uint8_t)SH_MID : sh;   // a grey dot is a shade lighter than black ink
  for (int r = 0; r < sp.h; r++) {
    const uint8_t* row = sp.px + r * sp.w;
    for (int c = 0; c < sp.w; c++) {
      const uint8_t v = row[c];
      if (!v) continue;
      const int px_ = x + (flip ? sp.w - 1 - c : c) * scale, py_ = y + r * scale;
      if (scale == 1) set(px_, py_, v == 4 ? grey : sh);   // (the icons: one pixel each, no rectangle call)
      else rect(px_, py_, scale, scale, v == 4 ? grey : sh);
    }
  }
}

// ---------------------------------------------------------------------------------------------
// Matrix
// ---------------------------------------------------------------------------------------------
void Matrix::rect(int x, int y, int w, int h, uint8_t s) {
  for (int yy = y; yy < y + h; yy++) for (int xx = x; xx < x + w; xx++) set(xx, yy, s);
}

void Matrix::sprite(int x, int y, int id, bool flip, int mode, uint8_t fill, uint8_t ink, uint8_t sick) {
  const Sprite& sp = SPRITES[id];
  const int w = sp.w, h = sp.h;
  auto at = [&](int c, int r) -> uint8_t { return (c < 0 || r < 0 || c >= w || r >= h) ? 0 : sp.px[r * w + c]; };
  auto put = [&](int c, int r, uint8_t s) { set(x + (flip ? w - 1 - c : c), y + r, s); };

  // body fill: everything not reachable from outside (4-connected) and not outline
  if (sick) {   // the speckle: inside the body's span along both its row and its column (works for open outlines too)
    int8_t rmin[32], rmax[32], cmin[40], cmax[40];
    if (w <= 40 && h <= 32) {
      for (int r = 0; r < h; r++) { rmin[r] = 127; rmax[r] = -1; }
      for (int c = 0; c < w; c++) { cmin[c] = 127; cmax[c] = -1; }
      for (int r = 0; r < h; r++) for (int c = 0; c < w; c++) if (at(c, r)) {
        if (c < rmin[r]) rmin[r] = (int8_t)c;
        if (c > rmax[r]) rmax[r] = (int8_t)c;
        if (r < cmin[c]) cmin[c] = (int8_t)r;
        if (r > cmax[c]) cmax[c] = (int8_t)r;
      }
      for (int r = 0; r < h; r++) for (int c = 0; c < w; c++)
        if (!at(c, r) && c > rmin[r] && c < rmax[r] && r > cmin[c] && r < cmax[c] && !((x + (flip ? w - 1 - c : c) + y + r) & 1)) put(c, r, sick);
    }
  }
  if (fill) {
    static uint8_t out[40 * 30];
    const int bw = w + 2, bh = h + 2;
    if (bw <= 40 && bh <= 30) {
      for (int i = 0; i < bw * bh; i++) out[i] = 0;
      static int16_t stack[40 * 30];
      int sp_ = 0;
      stack[sp_++] = 0; out[0] = 1;
      while (sp_) {
        int p = stack[--sp_], px = p % bw, py = p / bw;
        const int nx[4] = {px - 1, px + 1, px, px}, ny[4] = {py, py, py - 1, py + 1};
        for (int k = 0; k < 4; k++) {
          if (nx[k] < 0 || ny[k] < 0 || nx[k] >= bw || ny[k] >= bh) continue;
          int q = ny[k] * bw + nx[k];
          if (out[q] || at(nx[k] - 1, ny[k] - 1)) continue;
          out[q] = 1; stack[sp_++] = (int16_t)q;
        }
      }
      for (int r = 0; r < h; r++)
        for (int c = 0; c < w; c++)
          if (!at(c, r) && !out[(r + 1) * bw + c + 1] && !(sick && !((x + (flip ? w - 1 - c : c) + y + r) & 1))) put(c, r, fill);   // (the speckle is fixed to the screen: it doesn't crawl as they walk)
    }
  }

  for (int r = 0; r < h; r++) {
    for (int c = 0; c < w; c++) {
      const uint8_t v = at(c, r);
      if (!v) continue;
      if (v == 2 && mode == 1) {
        // eyes closed: only the bottom row of each eye, stretched one dot sideways
        if (at(c, r + 1) == 2) continue;
        put(c, r, ink);
        if (!at(c + 1, r) && at(c - 1, r) != 2) put(c + 1, r, ink);
        continue;
      }
      if (v == 4 && sick && !((x + (flip ? w - 1 - c : c) + y + r) & 1)) { put(c, r, ink); continue; }   // a grey body gets dark spots when sick
      put(c, r, v == 4 ? (uint8_t)SH_MID : ink);   // 4: a grey dot (rubber, goo)
    }
  }
}

void Matrix::text(int x, int y, const char* s, uint8_t sh) {
  for (; *s; s++, x += 6) {
    const uint8_t* g = &kFont5x7[(uint8_t)*s * 5];
    for (int c = 0; c < 5; c++) for (int r = 0; r < 7; r++) if (g[c] & (1 << r)) set(x + c, y + r, sh);
  }
}
void Matrix::textCenter(int y, const char* s, uint8_t sh) { text((MW - textWidth(s)) / 2, y, s, sh); }

void Matrix::small(int x, int y, const char* s, uint8_t sh, int spaceAdv) {
  for (; *s; s++) {
    if (*s == ' ') { x += spaceAdv; continue; }
    const G35* g = glyph35(*s);
    x += 4;
    if (!g) continue;
    x -= 4;
    for (int r = 0; r < 5; r++) for (int c = 0; c < 3; c++) if (g->r[r] & (4 >> c)) set(x + c, y + r, sh);
    x += 4;
  }
}
// Centered; a line that's a dot or two too wide (like "LIL BLUNKUS") tightens its word gaps to fit.
void Matrix::smallCenter(int y, const char* s, uint8_t sh) {
  int adv = 4;
  while (adv > 1 && smallWidth(s, adv) > MW) adv--;
  small((MW - smallWidth(s, adv)) / 2, y, s, sh, adv);
}
int Matrix::smallWidth(const char* s, int spaceAdv) {
  int w = 0;
  for (const char* p = s; *p; p++) w += *p == ' ' ? spaceAdv : 4;
  if (w && s[0]) { const char* e = s; while (e[1]) e++; if (*e != ' ') w -= 1; }
  return w;
}

void Matrix::render(Frame& f, uint8_t offShade, uint8_t bgShade) const {
  // every dot is a 3x3 block with a 1 px gap: written straight into the frame (the matrix is always fully on screen)
  static_assert(DOT == 4 && MX >= 0 && MX + MW * DOT <= W && MY >= 0 && MY1 <= H, "the dot matrix must fit in the frame (and a dot is 3 lit + 1 gap)");
  for (int y = 0; y < MH; y++) {
    uint8_t* row = f.px + (MY + y * DOT) * W + MX;
    const uint8_t* src = d + y * MW;
    for (int x = 0; x < MW; x++, row += DOT) {
      const uint8_t s = src[x] ? src[x] : offShade;
      for (int r = 0; r < DOT - 1; r++) { uint8_t* q = row + r * W; q[0] = q[1] = q[2] = s; q[3] = bgShade; }
      uint8_t* g = row + (DOT - 1) * W; g[0] = g[1] = g[2] = g[3] = bgShade;   // the gap row under it
    }
  }
}

}  // namespace lf
