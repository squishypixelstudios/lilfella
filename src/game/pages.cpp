// Full-screen pages (still 4 shades): first-time setup, settings, confirmations.
#include "game.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

namespace lf {

static int daysInMonth(int y, int m) {
  static const int dm[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  int d = dm[(m - 1) % 12];
  if (m == 2 && ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0)) d = 29;
  return d;
}

bool Game::tapIn(int x, int y, int w, int h) const {
  return in.tap && in.px >= x && in.px < x + w && in.py >= y && in.py < y + h;
}

bool Game::pageButton(Frame& f, int x, int y, int w, int h, const char* label, bool enabled) {
  const bool press = enabled && in.down && in.px >= x && in.px < x + w && in.py >= y && in.py < y + h;
  if (press) { f.rect(x, y, w, h, SH_INK); }
  f.roundFrame(x, y, w, h, enabled ? SH_INK : SH_FAINT);
  f.textCenter(x + w / 2, y + (h - 7) / 2, label, press ? SH_BG : (enabled ? SH_INK : SH_MID));
  if (enabled && tapIn(x, y, w, h)) { sound(SND_TAP); return true; }
  return false;
}

// ---------------------------------------------------------------------------------------------
// clock: tap above a number to raise it, below to lower it
// ---------------------------------------------------------------------------------------------
void Game::pageClock(Frame& f) {
  f.textCenter(W / 2, 16, "SET CLOCK", SH_INK, 2);
  int* c = clockSel_;
  char b[24];
  // field boxes: {x, y, w, h, index, min, max}
  struct Fld { int x, y, w, h, i, lo, hi; };
  const int dy = 62, ty = 126;
  const Fld flds[5] = {
    {32, dy, 47, 14, 0, 2025, 2099}, {92, dy, 23, 14, 1, 1, 12}, {128, dy, 23, 14, 2, 1, 31},
    {48, ty, 35, 21, 3, 0, 23}, {102, ty, 35, 21, 4, 0, 59}};
  snprintf(b, sizeof b, "%04d-%02d-%02d", c[0], c[1], c[2]);
  f.text(32, dy, b, SH_INK, 2);
  snprintf(b, sizeof b, "%02d:%02d", c[3], c[4]);
  f.text(48, ty, b, SH_INK, 3);
  // which arrow (if any) is the finger on right now?
  int holdFld = -1, holdDir = 0;
  if (in.down) {
    for (int k = 0; k < 5; k++) {
      const Fld& fl = flds[k];
      if (in.px < fl.x - 4 || in.px >= fl.x + fl.w + 4) continue;
      if (in.py >= fl.y - 22 && in.py < fl.y + fl.h / 2) { holdFld = k; holdDir = 1; }
      else if (in.py >= fl.y + fl.h / 2 && in.py < fl.y + fl.h + 22) { holdFld = k; holdDir = -1; }
    }
  }
  bool step = false;
  if (holdFld < 0) { clockHoldFld_ = -1; clockHoldDir_ = 0; clockHoldT_ = 0; }
  else if (holdFld != clockHoldFld_ || holdDir != clockHoldDir_) {   // finger just landed (or slid to another arrow)
    clockHoldFld_ = holdFld; clockHoldDir_ = holdDir; clockHoldT_ = 0; step = true;
  } else {
    const float before = clockHoldT_;
    clockHoldT_ += dt;
    const float delay = 0.45f;                                         // then it speeds up the longer you hold
    if (clockHoldT_ >= delay) {
      const float rate = clockHoldT_ > 2.0f ? 0.05f : (clockHoldT_ > 1.0f ? 0.1f : 0.18f);
      if (before < delay || (int)((clockHoldT_ - delay) / rate) != (int)((before - delay) / rate)) step = true;
    }
  }
  if (step) {
    const Fld& fl = flds[clockHoldFld_];
    const int hi = fl.i == 2 ? daysInMonth(c[0], c[1]) : fl.hi;
    if (clockHoldDir_ > 0) c[fl.i] = c[fl.i] >= hi ? fl.lo : c[fl.i] + 1;
    else c[fl.i] = c[fl.i] <= fl.lo ? hi : c[fl.i] - 1;
    if (clockHoldT_ < 0.5f) sound(SND_KEY);   // click on the first press only; the repeats stay quiet
  }
  for (const Fld& fl : flds) {
    const char up[2] = {30, 0}, dn[2] = {31, 0};
    f.textCenter(fl.x + fl.w / 2, fl.y - 12, up, SH_MID);
    f.textCenter(fl.x + fl.w / 2, fl.y + fl.h + 5, dn, SH_MID);
  }
  const int dim = daysInMonth(c[0], c[1]);
  if (c[2] > dim) c[2] = dim;
  f.textCenter(W / 2, 92, "date", SH_FAINT);
  const bool fromSettings = clockNext_ == S_SETTINGS;
  if (fromSettings && pageButton(f, 22, 182, 66, 26, "CANCEL")) { setScreen(S_SETTINGS); return; }
  if (pageButton(f, fromSettings ? 96 : 52, 182, fromSettings ? 66 : 80, 26, "OK")) {
    DateTime d = {(uint16_t)c[0], (uint8_t)c[1], (uint8_t)c[2], (uint8_t)c[3], (uint8_t)c[4], 0};
    plat->rtcSet(d);
    sv.clockSet = 1;
    turboOffset_ = 0;
    sv.fallbackClock = toEpoch(d) - nowMs / 1000;
    updateClock();
    autosave(true);
    if (clockNext_ == S_SETUP_NAME) { titleBar_ = -1; setScreen(S_TITLE); }   // a brand new device: the title comes first
    else setScreen(clockNext_);
  }
}


// ---------------------------------------------------------------------------------------------
// title: shown once the clock is set on a brand new device. "LiL FeLLa" drops in a letter at a time, then rides
// a wave; all ten Lil Fellas parade past, hopping in step. Everything repeats every 20 s (8 bars of the tune),
// so the loop has no seam. Tap to start.
// ---------------------------------------------------------------------------------------------
static float wrapf(float a, float b) { a = fmodf(a, b); return a < 0 ? a + b : a; }
static float bounceOut(float u) {
  const float n = 7.5625f, d = 2.75f;
  if (u < 1 / d) return n * u * u;
  if (u < 2 / d) { u -= 1.5f / d; return n * u * u + 0.75f; }
  if (u < 2.5f / d) { u -= 2.25f / d; return n * u * u + 0.9375f; }
  u -= 2.625f / d; return n * u * u + 0.984375f;
}
// a sprite stretched to sx by sy, centred on cx and standing on by (the hop's squash and stretch)
static void titleSprite(Frame& f, int id, float cx, float by, float sx, float sy, uint8_t ink, bool flip) {
  const Sprite& sp = SPRITES[id];
  const int dw = (int)lroundf(sp.w * sx), dh = (int)lroundf(sp.h * sy);
  if (dw < 1 || dh < 1) return;
  const int x0 = (int)lroundf(cx - dw / 2.0f), y0 = (int)lroundf(by - dh);
  for (int j = 0; j < dh; j++) {
    const int r = (int)((j + 0.5f) / dh * sp.h);
    for (int i = 0; i < dw; i++) {
      int c = (int)((i + 0.5f) / dw * sp.w);
      if (flip) c = sp.w - 1 - c;
      const uint8_t v = sp.px[r * sp.w + c];
      if (v) f.set(x0 + i, y0 + j, v == 4 ? (uint8_t)(ink > 1 ? ink - 1 : 1) : ink);
    }
  }
}

void Game::pageTitle(Frame& f) {
  static const uint8_t fellas[10] = {SPR_AD_LOAF, SPR_AD_PUDDLE, SPR_AD_JELLY, SPR_AD_DANGO, SPR_AD_PUFF,
                                     SPR_AD_MARSHY, SPR_AD_BOOP, SPR_AD_FUBZ, SPR_AD_NIMBUS, SPR_AD_BALLBOI};
  constexpr float BEAT = 0.625f, BAR = 2.5f, PI_ = 3.14159265f;
  const float t = scrT_;
  // the tune: one bar at a time, so it stops within a bar when the page is left
  const int bar = (int)(t / BAR);
  if (bar != titleBar_) { titleBar_ = bar; sound((Sound)(SND_TITLE1 + bar % 8)); }
  // bubbles drifting up
  for (int i = 0; i < 7; i++) {
    const float y = 240 - wrapf(i * 37 + t * ((i & 1) ? 26 : 13), 260), x = 14 + i * 26 + 6 * sinf(2 * PI_ * t / 5 + i);
    f.ring((int)lroundf(x), (int)lroundf(y), 3 + i % 3, SH_FAINT);
  }
  // the title
  static const char* const lines[2] = {"LiL", "FeLLa"};
  static const int lineY[2] = {22, 68};
  for (int l = 0, k = 0; l < 2; l++) {
    const int n = (int)strlen(lines[l]);
    for (int c = 0; c < n; c++, k++) {
      const float u = (t - 0.2f - k * 0.18f) / 0.7f;
      if (u <= 0) continue;
      const float y = u < 1 ? -50 + (lineY[l] + 50) * bounceOut(u) : lineY[l] + 3 * sinf(2 * PI_ * t / (BEAT * 2) - k * 0.7f);
      const char ch[2] = {lines[l][c], 0};
      f.text(W / 2 - (n * 30 - 5) / 2 + c * 30, (int)lroundf(y), ch, SH_INK, 5);
    }
  }
  // far row: small and grey, strolling right
  f.hline(0, 150, W, SH_FAINT);
  for (int i = 0; i < 10; i++) {
    const float x = wrapf(i * 30 + t * 15, 300) - 30, p = wrapf(t / BEAT + i * 0.37f, 1);
    titleSprite(f, fellas[(i * 3) % 10], x, 150 - 12 * p * (1 - p), 1, 1, SH_MID, true);
  }
  // near row: big, hopping left in a wave, squashing as they land
  f.hline(0, 196, W, SH_INK);
  for (int i = 0; i < 12; i++) f.set((int)wrapf(i * 16 - t * 23, 192) - 4, 198, SH_MID);
  for (int i = 0; i < 10; i++) {
    const float x = wrapf(i * 46 - t * 23, 460) - 40, p = wrapf(t / BEAT + i * 0.5f, 1);
    if (x > W + 30) continue;
    const float sq = p < 0.18f ? 1 - p / 0.18f : p > 0.9f ? (p - 0.9f) / 0.1f * 0.5f : -0.4f * sinf(PI_ * (p - 0.18f) / 0.72f);
    titleSprite(f, fellas[i], x, 196 - 56 * p * (1 - p), 2 * (1 + 0.14f * sq), 2 * (1 - 0.14f * sq), SH_INK, false);
  }
  if (t > 2.5f && wrapf(t, BEAT * 2) < BEAT * 1.5f) f.textCenter(W / 2, 211, "TAP TO START", SH_MID);
  if (in.tap) { sound(SND_MENU); setScreen(S_SETUP_NAME); }
}
// ---------------------------------------------------------------------------------------------
void Game::pageName(Frame& f) {
  // type a name on the keyboard (up to 10 letters), or take a random one; then "Let's goo!"
  static const int MAXN = 10;
  int len = (int)strlen(nameBuf_);
  f.roundFrame(12, 6, W - 24, 26, SH_INK);
  if (len) f.textCenter(W / 2, 12, nameBuf_, SH_INK, 2);
  else f.textCenter(W / 2, 14, "type a name", SH_FAINT);
  // letters A-Z, then delete and space: 7 keys across
  static const char LET[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  const int kw = 22, kh = 20, gx = 2, gy = 2, x0 = (W - (7 * kw + 6 * gx)) / 2, y0 = 38;
  for (int i = 0; i < 28; i++) {
    const int x = x0 + (i % 7) * (kw + gx), y = y0 + (i / 7) * (kh + gy);
    char lab[4] = {0, 0, 0, 0};
    if (i < 26) lab[0] = LET[i]; else if (i == 26) { lab[0] = 'D'; lab[1] = 'E'; lab[2] = 'L'; } else lab[0] = '_';
    if (!pageButton(f, x, y, kw, kh, lab)) continue;
    if (nameFresh_) { len = 0; nameBuf_[0] = 0; nameFresh_ = false; if (i == 26) continue; }   // the first key starts a name of your own
    if (i == 26) { if (len) nameBuf_[--len] = 0; }
    else if (len < MAXN && !(i == 27 && (!len || nameBuf_[len - 1] == ' '))) {
      char c = i == 27 ? ' ' : (char)(LET[i] + ((!len || nameBuf_[len - 1] == ' ') ? 0 : 32));   // Capital first letters
      nameBuf_[len++] = c; nameBuf_[len] = 0;
    }
  }
  if (pageButton(f, 22, 130, 140, 28, "Random name")) { pickName(); }
  bool any = false;
  for (int i = 0; i < len; i++) if (nameBuf_[i] != ' ') any = true;
  if (pageButton(f, 22, 168, 140, 28, "Let's goo!", any)) {
    while (len && nameBuf_[len - 1] == ' ') nameBuf_[--len] = 0;
    newPet();
    setScreen(S_MAIN);
  }
}

// ---------------------------------------------------------------------------------------------
void Game::pageSettings(Frame& f) {
  // (Speed is a developer option: remove it, and A_SPEED, before the final version.)
  // three short pages with tall rows, so each one is easy to hit with a finger; the two that wipe things are on their own page
  enum Act { A_SOUND, A_BRIGHT, A_SCREEN, A_AUTOOFF, A_MORE, A_FRIENDS, A_JOURNAL, A_CLOCK, A_TOUCH, A_SPEED, A_NEWBLOB, A_ERASE, A_BACK };
  static const Act page1[] = {A_SOUND, A_BRIGHT, A_SCREEN, A_AUTOOFF, A_MORE, A_BACK};
  static const Act page2[] = {A_FRIENDS, A_JOURNAL, A_CLOCK, A_SPEED, A_MORE, A_BACK};
  static const Act page3[] = {A_NEWBLOB, A_ERASE, A_TOUCH, A_BACK};   // (Touch, the debug overlay, lives here)
  static const char* const vol[6] = {"0 OFF", "1", "2", "3", "4", "5 MAX"};
  static const int speeds[4] = {1, 60, 600, 3600};
  if (setPage_ < 0 || setPage_ > 2) setPage_ = 0;
  const bool second = setPage_ > 0;
  const Act* acts = setPage_ == 2 ? page3 : setPage_ == 1 ? page2 : page1;
  const int n = setPage_ == 2 ? 4 : 6;
  char title[20]; snprintf(title, sizeof title, "SETTINGS %d/3", setPage_ + 1);
  f.textCenter(W / 2, 8, title, SH_INK);
  char ver[16]; snprintf(ver, sizeof ver, "v%s", LF_VERSION);
  f.textCenter(W / 2, SETTINGS_Y0 + 6 * SETTINGS_STEP + 4, ver, SH_MID);
  for (int i = 0; i < n; i++) {
    const Act a = acts[i];
    char val[12] = "";
    const char* lab = "";
    switch (a) {
      case A_SOUND: lab = "Sound"; snprintf(val, sizeof val, "%s", vol[sv.volume > 5 ? 2 : sv.volume]); break;
      case A_BRIGHT: lab = "Brightness"; snprintf(val, sizeof val, "%d", sv.brightness); break;
      case A_SCREEN: lab = "Screen"; snprintf(val, sizeof val, "%s", sv.eq[0] == 1 ? "NIGHT" : "LCD"); break;
      case A_AUTOOFF: lab = "Auto off"; snprintf(val, sizeof val, "%s", sv.eq[2] == 1 ? "Never" : sv.eq[2] == 2 ? "15 Sec" : sv.eq[2] == 3 ? "30 Sec" : sv.eq[2] == 4 ? "1 Min" : "5 Sec"); break;
      case A_MORE: lab = "More"; snprintf(val, sizeof val, ">"); break;
      case A_FRIENDS: lab = "Old friends"; snprintf(val, sizeof val, "%d >", sv.hallCount); break;
      case A_JOURNAL: { int tot = 0; const int got = journalCount(-1, &tot); lab = "Journal"; snprintf(val, sizeof val, "%d/%d >", got, tot); break; }
      case A_CLOCK: lab = "Clock"; snprintf(val, sizeof val, "%02d:%02d", hour, minute); break;
      case A_TOUCH: lab = "Touch"; snprintf(val, sizeof val, "%s", showTouch ? "DEBUG" : "OFF"); break;
      case A_SPEED: lab = "Speed"; snprintf(val, sizeof val, "x%d", timeScale); break;
      case A_NEWBLOB: lab = "New blob"; break;
      case A_ERASE: lab = "Erase everything"; break;
      case A_BACK: lab = "Back"; snprintf(val, sizeof val, "%s", second ? "<" : ""); break;
    }
    const int y = SETTINGS_Y0 + i * SETTINGS_STEP;
    const bool press = in.down && in.py >= y && in.py < y + SETTINGS_ROW_H && in.px >= 16 && in.px < W - 16;
    if (press) f.rect(16, y, W - 32, SETTINGS_ROW_H, SH_INK);
    const uint8_t fg = press ? SH_BG : SH_INK;
    const int ty = y + (SETTINGS_ROW_H - 7) / 2;
    f.text(24, ty, lab, fg);
    f.text(W - 24 - Frame::textWidth(val), ty, val, press ? SH_BG : SH_MID);
    if (i < n - 1) f.hline(24, y + SETTINGS_STEP - 1, W - 48, SH_FAINT);
    if (!tapIn(16, y, W - 32, SETTINGS_ROW_H)) continue;
    sound(SND_TAP);
    switch (a) {
      case A_SOUND: sv.volume = (uint8_t)((sv.volume + 1) % 6); sound(SND_COO_SOFT); break;
      case A_BRIGHT: sv.brightness = (uint8_t)(sv.brightness % 5 + 1); break;
      case A_SCREEN: sv.eq[0] = sv.eq[0] == 1 ? 0 : 1; break;
      case A_AUTOOFF: { static const uint8_t nxt[5] = {2, 0, 3, 4, 1}; sv.eq[2] = nxt[sv.eq[2] < 5 ? sv.eq[2] : 0]; break; }   // 5 Sec -> 15 -> 30 -> 1 Min -> Never
      case A_MORE: setPage_++; return;
      case A_FRIENDS: friendsPage_ = 0; setScreen(S_FRIENDS); return;
      case A_JOURNAL: journalSel_ = -1; setScreen(S_JOURNAL); return;
      case A_CLOCK: {
        DateTime d = fromEpoch(epoch);
        clockSel_[0] = d.year; clockSel_[1] = d.month; clockSel_[2] = d.day; clockSel_[3] = d.hour; clockSel_[4] = d.minute;
        clockNext_ = S_SETTINGS; setScreen(S_SETUP_CLOCK); return;
      }
      case A_TOUCH: showTouch = !showTouch; sv.eq[1] = showTouch ? 1 : 0; break;
      case A_SPEED: { int k = 0; for (int j = 0; j < 4; j++) if (speeds[j] == timeScale) k = j; timeScale = speeds[(k + 1) % 4]; break; }
      case A_NEWBLOB: confirm_ = CF_NEWBLOB; setScreen(S_CONFIRM); return;
      case A_ERASE: confirm_ = CF_ERASE; setScreen(S_CONFIRM); return;
      case A_BACK:
        if (second) { setPage_--; return; }   // Back goes to the page before
        autosave(true); setScreen(sv.hasBlob ? S_MAIN : S_SETUP_NAME); return;
    }
    autosave(false);
  }
}

// ---------------------------------------------------------------------------------------------
// old friends: the fellas that came before, newest first (the save keeps the last HALL_MAX)
// ---------------------------------------------------------------------------------------------
void Game::pageFriends(Frame& f) {
  constexpr int PER = 4, Y0 = 26, ROW = 40;
  const int n = sv.hallCount > HALL_MAX ? HALL_MAX : sv.hallCount;
  const int pages = n > PER ? (n + PER - 1) / PER : 1;
  if (friendsPage_ >= pages) friendsPage_ = pages - 1;
  char b[32];
  if (pages > 1) snprintf(b, sizeof b, "OLD FRIENDS %d/%d", friendsPage_ + 1, pages);
  else snprintf(b, sizeof b, "OLD FRIENDS");
  f.textCenter(W / 2, 8, b, SH_INK);
  if (!n) {
    f.textCenter(W / 2, 80, "No one yet.", SH_MID);
    f.textCenter(W / 2, 96, "Fellas you raise are", SH_MID);
    f.textCenter(W / 2, 108, "remembered here.", SH_MID);
  }
  for (int r = 0; r < PER; r++) {
    const int i = friendsPage_ * PER + r;
    if (i >= n) break;
    const HallEntry& e = sv.hall[i];
    const int y = Y0 + r * ROW;
    if (e.look) {   // their look, at 2x, standing on the row's floor
      const int spr = lookSprite(e.look - 1), sw = spriteW(spr) * 2, sh = spriteH(spr) * 2;
      f.sprite(14 + (40 - sw) / 2, y + ROW - 6 - sh, spr, SH_INK, 2);
    }
    char nm[13]; memcpy(nm, e.name, 12); nm[12] = 0;
    f.text(60, y + 3, nm, SH_INK);
    const char* how = endingName(e.ending);
    if (!(e.flags & HE_HOURS)) snprintf(b, sizeof b, "%s, %d %s", how, e.age, e.age == 1 ? "day" : "days");
    else if (e.age < 48) snprintf(b, sizeof b, "%s, %d h", how, e.age < 1 ? 1 : e.age);
    else snprintf(b, sizeof b, "%s, %d days", how, e.age / 24);
    f.text(60, y + 14, b, SH_MID);
    if (e.look) f.text(60, y + 25, lookName(e.look - 1), SH_MID);
    f.hline(14, y + ROW - 2, W - 28, SH_FAINT);
  }
  {   // how many blobs this device has raised: in the first free row (never when the log is empty, never over the buttons)
    const int onPage = n - friendsPage_ * PER < PER ? n - friendsPage_ * PER : PER;
    if (n && onPage < PER && sv.lifeBlobs) {
      snprintf(b, sizeof b, sv.lifeBlobs == 1 ? "%u blob raised so far" : "%u blobs raised so far", (unsigned)sv.lifeBlobs);
      f.textCenter(W / 2, Y0 + onPage * ROW + 12, b, SH_MID);
    }
  }
  const int by = 196, bh = 26;
  if (pages > 1) {
    if (pageButton(f, 16, by, 40, bh, "<", friendsPage_ > 0)) { friendsPage_--; return; }
    if (pageButton(f, W - 56, by, 40, bh, ">", friendsPage_ < pages - 1)) { friendsPage_++; return; }
  }
  if (pageButton(f, 62, by, 60, bh, "BACK")) setScreen(S_SETTINGS);
}

// ---------------------------------------------------------------------------------------------
// the Journal: every look, by stage; the ones you've raised show their picture, the rest a "?". Tap one for its name.
// ---------------------------------------------------------------------------------------------
static const uint8_t JOURNAL[5][11] = {   // each stage's looks, in the order the tree lists them (0 ends a short list)
  {F_BABY},
  {F_S0, F_S0B, F_S0C},
  {F_TN_SPROUT, F_TN_BUN, F_TN_CURL, F_TN_SLOSH, F_TN_WOBBLES, F_TN_MALLOW, F_TN_TOASTY, F_TN_BEAN},
  {F_AD_JELLY, F_AD_DANGO, F_AD_MARSHY, F_AD_PUDDLE, F_AD_PUFF, F_AD_FUBZ, F_AD_BOOP, F_AD_NIMBUS, F_AD_BALLBOI, F_AD_LOAF, F_S1C},
  {F_BB_BLOBBERT, F_BB_BALLBOI, F_BB_ALIEN}};
static const char* const JOURNAL_STAGE[5] = {"LIL BLOB", "LIL BLUB", "LIL BLUNKUS", "LIL FELLA", "BIG BLORBUS"};
static int journalLen(int s) { int n = 0; while (n < 11 && JOURNAL[s][n]) n++; return n; }

int Game::journalCount(int stage, int* total) const {   // stage -1: all of them
  int got = 0, tot = 0;
  for (int s = 0; s < 5; s++) {
    if (stage >= 0 && s != stage) continue;
    for (int i = 0; i < journalLen(s); i++) { tot++; if (seenLook(JOURNAL[s][i])) got++; }
  }
  if (total) *total = tot;
  return got;
}

// a look shrunk to fit a cell: each dot of the result is lit if any dot of its block is (grey only if the whole block is)
static void spriteFit(Frame& f, int cx, int bottom, int id, int maxW, int maxH) {
  const Sprite& sp = SPRITES[id];
  int s = 1;
  while ((sp.w + s - 1) / s > maxW || (sp.h + s - 1) / s > maxH) s++;
  const int w = (sp.w + s - 1) / s, h = (sp.h + s - 1) / s, x0 = cx - w / 2, y0 = bottom - h;
  for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) {
    bool ink = false, grey = false;
    for (int dy = 0; dy < s; dy++) for (int dx = 0; dx < s; dx++) {
      const int sx = x * s + dx, sy = y * s + dy;
      if (sx >= sp.w || sy >= sp.h) continue;
      const uint8_t v = sp.px[sy * sp.w + sx];
      if (v == 4) grey = true; else if (v) ink = true;
    }
    if (ink || grey) f.set(x0 + x, y0 + y, ink ? SH_INK : SH_MID);
  }
}

void Game::pageJournal(Frame& f) {
  constexpr int CW = 21, CH = 18, PER = 8, HEAD = 10;   // cells: 21 x 18 logical px, 8 to a row
  char b[32];
  int tot = 0; const int got = journalCount(-1, &tot);
  snprintf(b, sizeof b, "JOURNAL %d/%d", got, tot);
  f.textCenter(W / 2, 6, b, SH_INK);
  int y = 18;
  for (int s = 0; s < 5; s++) {
    const int n = journalLen(s);
    int st = 0; const int sg = journalCount(s, &st);
    snprintf(b, sizeof b, "%s %d/%d", JOURNAL_STAGE[s], sg, st);
    f.text(8, y + 1, b, SH_MID);
    f.hline(8 + Frame::textWidth(b) + 4, y + 4, W - 16 - Frame::textWidth(b) - 4, SH_FAINT);
    y += HEAD;
    for (int r = 0; r * PER < n; r++) {
      const int inRow = n - r * PER < PER ? n - r * PER : PER, x0 = (W - inRow * CW) / 2;
      for (int k = 0; k < inRow; k++) {
        const int i = r * PER + k, look = JOURNAL[s][i], cx = x0 + k * CW;
        const bool sel = journalSel_ == look;
        if (sel) f.roundFrame(cx, y, CW, CH, SH_INK);
        if (seenLook(look)) spriteFit(f, cx + CW / 2, y + CH - 2, lookSprite(look), CW - 3, CH - 3);
        else f.text(cx + (CW - 5) / 2, y + (CH - 7) / 2, "?", SH_FAINT);
        if (tapIn(cx, y, CW, CH)) { journalSel_ = look; sound(SND_TAP); }
      }
      y += CH;
    }
  }
  // the name of the one tapped (or a "?" for one you haven't raised yet)
  if (journalSel_ >= 0) f.textCenter(W / 2, y + 4, seenLook(journalSel_) ? lookName(journalSel_) : "???", SH_INK);
  else f.textCenter(W / 2, y + 4, "Tap one to see its name", SH_FAINT);
  if (pageButton(f, 62, 198, 60, 24, "BACK")) setScreen(S_SETTINGS);
}

void Game::pageConfirm(Frame& f) {
  char b[32];
  const bool erase = confirm_ == CF_ERASE;
  if (erase) {
    f.textCenter(W / 2, 40, "Erase everything?", SH_INK);
    f.textCenter(W / 2, 58, "Your fella, old friends,", SH_MID);
    f.textCenter(W / 2, 70, "bubbles and settings", SH_MID);
    f.textCenter(W / 2, 82, "are all wiped.", SH_MID);
  } else {
    f.textCenter(W / 2, 50, "Start a new blob?", SH_INK);
    snprintf(b, sizeof b, "%s will be gone.", pet.d.name);
    f.textCenter(W / 2, 66, b, SH_MID);
  }
  if (pageButton(f, 22, 110, 66, 30, "NO")) { setScreen(S_SETTINGS); return; }
  if (pageButton(f, 96, 110, 66, 30, "YES")) {
    if (erase) { eraseAll(); return; }
    logFriend(END_RESET);
    sv.hasBlob = 0;
    pickName();
    autosave(true);
    setScreen(S_SETUP_NAME);
  }
}

// Settings → Erase everything: back to a brand new device (the clock is kept, but you're asked to check it).
void Game::eraseAll() {
  const uint32_t clock = sv.fallbackClock;
  saveDefaults(sv);
  sv.fallbackClock = clock;   // mode stays MODE_UNSET: a restart now also starts at the clock, like a new device
  pet.ended = false; pet.sleeping = false; pet.napping = false; pet.sadAcc = 0;
  timeScale = 1; showTouch = false; setPage_ = 0; friendsPage_ = 0; journalSel_ = -1;
  pottyPending_ = 0; tutWant_ = -1;
  pickName();
  autosave(true);
  DateTime d = fromEpoch(epoch);
  clockSel_[0] = d.year; clockSel_[1] = d.month; clockSel_[2] = d.day; clockSel_[3] = d.hour; clockSel_[4] = d.minute;
  clockNext_ = S_SETUP_NAME;
  setScreen(S_SETUP_CLOCK);
}

}  // namespace lf
