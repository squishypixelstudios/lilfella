// The dot-matrix LCD: the fella, their mess, icons, animations, meters and the two mini-games.
#include "game.h"
#include "breeds.h"
#include "eatfx.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

namespace lf {

static constexpr int FLOOR = MH - 1;    // bottom dot row things stand on

static int clampi(int v, int a, int b) { return v < a ? a : (v > b ? b : v); }
static void hearts(Matrix& m, int y, float v);   // (meter pages)

// ---------------------------------------------------------------------------------------------
// sprites for each form
// ---------------------------------------------------------------------------------------------
void Game::blobSprite(Form f, int& idle, int& alt, int& sleep) const {
  alt = -1;
  switch (f) {
    case F_DROPPER: idle = sleep = SPR_DROPPER; break;
    case F_BABY: idle = SPR_BABY_IDLE; alt = SPR_BABY_HOP; sleep = SPR_BABY_SLEEP1; break;
    case F_S0: idle = SPR_S0_IDLE; sleep = SPR_S0_SLEEP; break;
    case F_S0B: idle = sleep = SPR_S0B_IDLE; break;
    case F_S0C: idle = sleep = SPR_S0C_IDLE; break;
    case F_TN_BEAN: idle = sleep = SPR_TN_BEAN; break;
    case F_TN_BUN: idle = sleep = SPR_TN_BUN; break;
    case F_TN_CURL: idle = sleep = SPR_TN_CURL; break;
    case F_S1C: idle = sleep = SPR_S1C_IDLE; break;
    case F_AD_LOAF: idle = sleep = SPR_AD_LOAF; break;
    case F_AD_PUDDLE: idle = sleep = SPR_AD_PUDDLE; break;
    case F_AD_JELLY: idle = sleep = SPR_AD_JELLY; break;
    case F_TN_SPROUT: idle = sleep = SPR_TN_SPROUT; break;
    case F_AD_DANGO: idle = sleep = SPR_AD_DANGO; break;
    case F_AD_PUFF: idle = sleep = SPR_AD_PUFF; break;
    case F_TN_SLOSH: idle = sleep = SPR_TN_SLOSH; break;
    case F_TN_WOBBLES: idle = sleep = SPR_TN_WOBBLES; break;
    case F_TN_MALLOW: idle = sleep = SPR_TN_MALLOW; break;
    case F_TN_TOASTY: idle = sleep = SPR_TN_TOASTY; break;
    case F_AD_MARSHY: idle = sleep = SPR_AD_MARSHY; break;
    case F_AD_BOOP: idle = sleep = SPR_AD_BOOP; break;
    case F_AD_FUBZ: idle = sleep = SPR_AD_FUBZ; break;
    case F_AD_NIMBUS: idle = sleep = SPR_AD_NIMBUS; break;
    case F_AD_BALLBOI: idle = sleep = SPR_AD_BALLBOI; break;
    case F_BB_BLOBBERT: idle = sleep = SPR_BB_BLOBBERT; break;
    case F_BB_BALLBOI: idle = sleep = SPR_BB_BALLBOI; break;
    case F_BB_ALIEN: idle = sleep = SPR_BB_ALIEN; break;
    default: idle = sleep = SPR_TN_BEAN; break;
  }
}

// each look's own eating routine (eatfx.cpp, ported from the Eating Lab previews)
int Game::eatRoutine() const {
  using namespace eatfx;
  switch (form()) {
    case F_S0: return R_BLOBBERT;
    case F_S0B: return R_MOCHI;
    case F_S0C: return R_CHEEKS;
    case F_TN_BEAN: return R_BEAN;
    case F_TN_BUN: return R_BUN;
    case F_TN_CURL: return R_CURLY;
    case F_TN_SLOSH: return R_SLOSH;
    case F_TN_WOBBLES: return R_WOBBLES;
    case F_TN_MALLOW: return R_MALLOW;
    case F_TN_TOASTY: return R_TOASTY;
    case F_TN_SPROUT: return R_SPROUT;
    case F_S1C: return R_DAPPER;
    case F_AD_LOAF: return R_LOAF;
    case F_AD_PUDDLE: return R_PUDDLE;
    case F_AD_JELLY: return R_JELLY;
    case F_AD_DANGO: return R_SPIKY;
    case F_AD_PUFF: return R_PUFF;
    case F_AD_MARSHY: return R_MARSHY;
    case F_AD_BOOP: return R_BOOP;
    case F_AD_FUBZ: return R_FUBZ;
    case F_AD_NIMBUS: return R_NIMBUS;
    case F_AD_BALLBOI: return R_BALLBOI;
    case F_BB_BLOBBERT: return R_BIG_BLOBBERT;
    case F_BB_BALLBOI: return R_BIG_BALLBOI;
    case F_BB_ALIEN: return R_ALIEN;
    default: return R_NONE;
  }
}
float Game::eatSecs() const {
  if (absorbsFood()) return ABSORB_SECS;
  const int r = eatRoutine();
  return r ? eatfx::duration(r) + 0.6f : EAT_SECS;   // a moment's hold on the last frame
}

// the open-mouth eating frame drawn for each look
int Game::eatSprite(int idle) const {
  switch (idle) {
    case SPR_TN_BEAN: return SPR_TN_BEAN_EAT;
    case SPR_TN_BUN: return SPR_TN_BUN_EAT;
    case SPR_TN_CURL: return SPR_TN_CURL_EAT;
    case SPR_S1C_IDLE: return SPR_S1C_EAT;
    case SPR_AD_PUDDLE: return SPR_AD_PUDDLE_EAT;
    case SPR_AD_JELLY: return SPR_AD_JELLY_EAT;
    case SPR_TN_SPROUT: return SPR_TN_SPROUT_EAT;
    case SPR_AD_DANGO: return SPR_AD_DANGO_EAT;
    case SPR_AD_PUFF: return SPR_AD_PUFF_EAT;
    case SPR_AD_LOAF: return SPR_AD_LOAF_EAT;
    case SPR_TN_SLOSH: return SPR_TN_SLOSH_EAT;
    case SPR_TN_WOBBLES: return SPR_TN_WOBBLES_EAT;
    case SPR_TN_MALLOW: return SPR_TN_MALLOW_EAT;
    case SPR_TN_TOASTY: return SPR_TN_TOASTY_EAT;
    case SPR_AD_MARSHY: return SPR_AD_MARSHY_EAT;
    case SPR_AD_BOOP: return SPR_AD_BOOP_EAT;
    case SPR_AD_FUBZ: return SPR_AD_FUBZ_EAT;
    case SPR_AD_NIMBUS: return SPR_AD_NIMBUS_EAT;
    case SPR_AD_BALLBOI: return SPR_AD_BALLBOI_EAT;
    default: return idle;
  }
}

// The four life stages: Lil Blob (baby), Lil Blub (child), Lil Blunkus (teen), Lil Fella (adult).
// Named from the look they actually have, so a new stage shows only once they've grown into it (awake, picked).
const char* Game::lifeStageName() const {
  const Form f = form();
  if (f == F_DROPPER) return "DROPPER";
  if (f == F_BABY) return "LIL BLOB";
  const int st = stageOfForm(f);
  if (st == 1) return "LIL BLUB";
  if (st == 2) return "LIL BLUNKUS";
  if (st == 4) return "BIG BLORBUS";
  return "LIL FELLA";
}

// a look's name for the friends log: the life stage, then the look picked
const char* Game::lookName(int f) {
  switch (f) {
    case F_BABY: return "Lil Blob";
    case F_S0: return "Lil Blub Blobbert";
    case F_S0B: return "Lil Blub Mochi";
    case F_S0C: return "Lil Blub Cheeks";
    case F_TN_BEAN: return "Lil Blunkus Bean";
    case F_TN_BUN: return "Lil Blunkus Bun";
    case F_TN_CURL: return "Lil Blunkus Curly";
    case F_S1C: return "Lil Fella Dapper";
    case F_AD_LOAF: return "Lil Fella Loaf";
    case F_AD_PUDDLE: return "Lil Fella Puddle";
    case F_AD_JELLY: return "Lil Fella Jelly";
    case F_TN_SPROUT: return "Lil Blunkus Sprout";
    case F_AD_DANGO: return "Lil Fella SpikyBoi";
    case F_AD_PUFF: return "Lil Fella Puff";
    case F_TN_SLOSH: return "Lil Blunkus Slosh";
    case F_TN_WOBBLES: return "Lil Blunkus Wobbles";
    case F_TN_MALLOW: return "Lil Blunkus Mallow";
    case F_TN_TOASTY: return "Lil Blunkus Toasty";
    case F_AD_MARSHY: return "Lil Fella Marshy";
    case F_AD_BOOP: return "Lil Fella Boop";
    case F_AD_FUBZ: return "Lil Fella Fubz";
    case F_AD_NIMBUS: return "Lil Fella Nimbus";
    case F_AD_BALLBOI: return "Lil Fella Ballboi";
    case F_BB_BLOBBERT: return "Big Blorbus Blobbert";
    case F_BB_BALLBOI: return "Big Blorbus Ballboi";
    case F_BB_ALIEN: return "Big Blorbus Alien Cat";
    default: return "";
  }
}
int Game::lookSprite(int f) const { int idle, alt, sl; blobSprite((Form)f, idle, alt, sl); return idle; }

uint8_t Game::coatFill() const {
  if (pet.d.evolving || stageOfForm(form()) < 2) return 0;
  return pet.d.breed == SP_VOID ? SH_MID : 0;   // the void fella is dark all over
}

// mode: 0 awake, 1 eyes closed, 2 mouth open, 3 asleep. alt: second frame (walk / hop)
// Big Ballboi: the ball and, inside it, a hamster that slides around with the ball's motion
void Game::drawBallboi(int x, int y, bool still) {
  m_.sprite(x, y, SPR_BB_BALL_SHELL);
  const int hw = spriteW(SPR_BB_BALL_HAM), hh = spriteH(SPR_BB_BALL_HAM);
  const float hx = still ? 0 : ballHx_, hy = still ? 0 : ballHy_;
  int px = x + 5 + (int)lroundf(hx), py = y + 6 + (int)lroundf(hy);
  if (!still && ballSq_ > 0) {   // pressed flat against the ball's wall: wider and shorter... or squeezed thin
    const int sw = hw - 2, sh = hh + 1;
    drawSquish(hx > 0 ? px + 2 : px, py - 1, SPR_BB_BALL_HAM, sw, sh, false, sickShade());
    return;
  }
  m_.sprite(px, py, SPR_BB_BALL_HAM, false, pet.sick() && !still ? 1 : 0, 0, SH_INK, sickShade());
}

void Game::updateBall() {
  const bool moving = fabsf(blobX_ - blobTx_) >= 1 && !pet.sleeping;
  const float step = blobX_ - ballPrevX_;
  ballPrevX_ = blobX_;
  if (step > 0.01f) ballRun_ = 1; else if (step < -0.01f) ballRun_ = -1;
  if (moving && !ballWasMoving_) ballHv_ -= ballRun_ * 7;                  // starting to roll: the hamster lags behind
  if (!moving && ballWasMoving_) { ballHv_ += ballRun_ * 22; ballHy_ = 0; }  // stopping: the hamster carries on into the wall
  if (moving && step != 0) ballHv_ -= (step > 0 ? 1 : -1) * 1.5f;        // each roll step shakes it a little
  ballWasMoving_ = moving;
  ballHv_ += (-26.0f * ballHx_ - 3.0f * ballHv_) * dt;                      // a spring pulls it back to the middle
  ballHx_ += ballHv_ * dt;
  if (ballHx_ > 4) { ballHx_ = 4; if (ballHv_ > 3) ballSq_ = 0.35f; if (ballHv_ > 0) ballHv_ = -ballHv_ * 0.45f; }
  if (ballHx_ < -4) { ballHx_ = -4; if (ballHv_ < -3) ballSq_ = 0.35f; if (ballHv_ < 0) ballHv_ = -ballHv_ * 0.45f; }
  ballHy_ = -fabsf(ballHx_) * 0.5f;                                          // up the curve of the wall
  if (ballSq_ > 0) ballSq_ -= dt;
}

// Sick: a speckled body, droopy half-shut eyes and a shiver now and then (every look, the same way)
uint8_t Game::sickShade() const { return pet.sick() ? (coatFill() == SH_MID ? SH_FAINT : SH_MID) : 0; }

void Game::drawBlob(int x, int y, bool flip, int mode, bool alt) {
  const Form f = form();
  int idle, a2, sl;
  blobSprite(f, idle, a2, sl);
  const uint8_t sk = sickShade();
  if (sk) {
    if (mode == 0) mode = 1;                                         // too poorly to open their eyes properly
    if (mode != 3 && ((int)(anim_ * 9) % 7) == 0) x += ((int)(anim_ * 18) & 1) ? 1 : -1;   // a shiver
  }
  if (f == F_BB_BALLBOI) { drawBallboi(x, y - (alt && mode != 3 ? 1 : 0), mode == 3); return; }
  if (f == F_BB_BLOBBERT) {   // pushed against a side of the screen it squashes: narrower and taller
    const int w = spriteW(idle), h = spriteH(idle);
    const int ov = (x < 0 ? -x : 0) + (x + w > MW ? x + w - MW : 0);
    if (ov) {
      const int nx = x < 0 ? 0 : x, nw = w - ov, nh = h + (ov > 2 ? 2 : (ov + 1) / 2);
      drawSquish(nx, y + h - nh, idle, nw, nh, flip, sk);
      return;
    }
  }
  if (mode == 3) {
    if (f == F_BABY) { m_.sprite(x, y, alt ? SPR_BABY_SLEEP2 : SPR_BABY_SLEEP1, flip, 0, 0, SH_INK, sk); return; }
    if (sl != idle) { m_.sprite(x, y + spriteH(idle) - spriteH(sl), sl, flip, 0, coatFill(), SH_INK, sk); return; }
    m_.sprite(x, y, idle, flip, 1, coatFill(), SH_INK, sk);
    return;
  }
  if (f == F_BABY && mode == 1) { m_.sprite(x, y, SPR_BABY_BLINK, flip, 0, 0, SH_INK, sk); return; }
  if (alt && a2 >= 0) { m_.sprite(x, y + spriteH(idle) - spriteH(a2), a2, flip, mode, coatFill(), SH_INK, sk); return; }
  m_.sprite(x, y - (alt ? 1 : 0), idle, flip, mode, coatFill(), SH_INK, sk);
}

// The games draw the young smaller, so they fit between falling things; older looks play full size.
int Game::gameSprite() const {
  switch (form()) {
    case F_BABY: return SPR_MINI_BLOB;
    case F_S0: return SPR_MINI_ROUND;
    case F_S0B: return SPR_MINI_MOCHI;
    case F_S0C: return SPR_MINI_CHEEKS;
    case F_BB_BLOBBERT: case F_BB_BALLBOI: case F_BB_ALIEN: { int idle, alt, sl; blobSprite(lilFellaForm(), idle, alt, sl); return idle; }   // the games use the Lil Fella they grew from
    default: { int idle, alt, sl; blobSprite(form(), idle, alt, sl); return idle; }
  }
}
void Game::drawGameBlob(int x, int y, bool flip, int mode, bool alt) {
  const int sp = gameSprite();
  int idle, a2, sl; blobSprite(form(), idle, a2, sl);
  if (sp == idle) { drawBlob(x, y, flip, mode, alt); return; }
  m_.sprite(x, y - (alt ? 1 : 0), sp, flip, mode);
}

// ---------------------------------------------------------------------------------------------
// icons
// ---------------------------------------------------------------------------------------------
void Game::drawIcons(Frame& f) {
  static const uint8_t ids[IC_COUNT] = {SPR_IC_FEED, SPR_IC_LIGHT, SPR_IC_PLAY, SPR_IC_MEDS, SPR_IC_LITTER, SPR_IC_METER, SPR_IC_UNLOCK, SPR_IC_CALL};
  const bool call = callReason() != CR_NONE;
  int pressed = -1;   // the slot under the finger
  if (in.down) {   // (the same spot a tap will be judged by, so the icon that lights is the one that fires)
    const int col = in.ax < 54 ? 0 : in.ax < 92 ? 1 : in.ax < 130 ? 2 : 3;
    if (in.py < MY - 4) pressed = col;
    else if (in.py > MY1 + 4) pressed = 4 + col;
  }
  if (scr == S_EVOLVE) f.textCenter(W / 2, ICON_TOP_Y + 4, "Pick new form:", SH_INK);   // the look picker's heading
  for (int s = 0; s < IC_COUNT; s++) {
    if (s < 4 && (scr == S_DODGE || scr == S_RUN || scr == S_ORBIT || scr == S_EVOLVE)) continue;   // Dodge, Run, Orbit and the look picker: no icons above the LCD
    const int i = ICON_AT[s];
    const int x = ICON_X[s % 4], y = s < 4 ? ICON_TOP_Y : ICON_BOT_Y;
    bool on = i == selIcon_ || s == pressed || (i == IC_CALL && call) || (i == IC_LOCK && locked());   // the padlock stays lit (and shut) while they're locked
    f.sprite(x, y, i == IC_LOCK && locked() ? SPR_IC_LOCK : ids[i], on ? SH_INK : (sv.eq[0] == 1 ? SH_MID : SH_FAINT));   // Night: the off icons are a clear mid grey (the faint one is nearly black there)
  }
}

bool Game::iconTapped(int& which) const {
  if (!in.tap) return false;
  // Which icon: by where the finger rested over the whole press (in.ax), not the very first spot it touched. A
  // fingertip lands on its edge and rolls flat, so the first reading can be a few mm to one side: enough to turn
  // Meter into Medicine, its neighbour. (Top or bottom row is still decided by where it went down.)
  int col = in.ax < 54 ? 0 : in.ax < 92 ? 1 : in.ax < 130 ? 2 : 3;
  if (in.py < MY - 4) { which = ICON_AT[col]; return true; }
  if (in.py > MY1 + 4) { which = ICON_AT[4 + col]; return true; }
  return false;
}

// The LCD's touch area runs edge to edge and right up to the icon rows (no dead strips in between).
bool Game::inMatrix(int x, int y, int& dx, int& dy) const {
  if (x < 0 || x >= W || y < MY - 4 || y > MY1 + 4) return false;
  dx = clampi((x - MX) / DOT, 0, MW - 1);
  dy = clampi((y - MY) / DOT, 0, MH - 1);
  return true;
}

void Game::startAnim(AnimId a, float len, int arg) {
  if (scr == S_ANIM && an_ == AN_MEDS && anT_ < anLen_) return;   // the medicine plays out: nothing cuts it short
  an_ = a; anT_ = 0; anLen_ = len; anArg_ = arg;
  if (a == AN_EAT_MEAL || a == AN_EAT_SNACK || a == AN_EAT_DIET) { eatVar_ = (uint8_t)(pet.rnd() & 1); eatFromX_ = (int)blobX_; }
  if (scr != S_ANIM) setScreen(S_ANIM);
}

static const char* reasonText(CallReason r) {
  switch (r) {
    case CR_HUNGRY: return "HUNGRY";
    case CR_BORED: return "BORED";
    case CR_SICK: return "SICK";   // (the count: see doIcon)
    case CR_MESS: return "MESSY";
    case CR_LIGHT: return "SLEEPY";
    case CR_UPSET: return "UPSET";
    case CR_HELLO: return "HELLO!";
    case CR_CHECK: return "CHECK ME";
    default: return "HAPPY";
  }
}

void Game::doIcon(int ic) {
  sound(SND_TAP);
  const CallReason why = callReason();
  // first-hour lessons: this kind of care has been tried. Medicine and the light only count when they're done for
  // real: a sickness cured, the light turned off for a sleeping fella (see below).
  if (sv.hasBlob && form() != F_DROPPER && ic != IC_MEDS && ic != IC_LIGHT) markIcon(ic);
  if (scr == S_MENU && selIcon_ == ic) { sound(SND_BACK); setScreen(S_MAIN); return; }   // same icon again: back out
  if (ic == IC_METER) { sound(SND_MENU); selIcon_ = ic; meterPage_ = 0; setScreen(S_METER); selIcon_ = ic; return; }
  if (ic == IC_LIGHT) {   // the bulb is a switch: a soft click off, a bright one on
    setLight(!lightOff()); sound(lightOff() ? SND_LIGHT_OFF : SND_LIGHT_ON);
    if (lightOff() && pet.sleeping && sv.hasBlob && form() != F_DROPPER) markIcon(IC_LIGHT);   // the light lesson: off while they sleep
    setScreen(S_MAIN); return;
  }
  if (ic == IC_CALL) {
    if (why == CR_SICK && pet.sickStack() > 1) { char b[12]; snprintf(b, sizeof b, "SICK X%d", pet.sickStack()); say(b, 2.5f); }
    else if (why != CR_NONE) say(reasonText(why), 2.5f);
    setScreen(S_MAIN); return;
  }   // all good: nothing to say
  if (!sv.hasBlob || form() == F_DROPPER) { setScreen(S_MAIN); return; }
  if (pet.d.exploring) { sound(SND_NOPE); say("OUT"); setScreen(S_MAIN); return; }
  const bool asleep = pet.sleeping;
  switch (ic) {
    case IC_FEED:
      if (stageOfForm(form()) == 4) {   // Big Blorbus doesn't eat: the Feed icon is a star game instead
        if (asleep) { sound(SND_NOPE); say("ZZZ"); setScreen(S_MAIN); return; }
        sound(SND_MENU); memset(&orb_, 0, sizeof orb_); selIcon_ = ic; setScreen(S_ORBIT); return;
      }
      if (asleep && pet.d.hunger >= 40) { sound(SND_NOPE); say("ZZZ"); setScreen(S_MAIN); return; }   // a hungry fella wakes up for food
      feedMeal(); return;   // the Feed icon serves a meal, straight away
    case IC_PLAY:
      if (asleep) { sound(SND_NOPE); say("ZZZ"); setScreen(S_MAIN); return; }
      sound(SND_MENU); setScreen(S_MENU); selIcon_ = ic; return;
    case IC_MEDS:
      if (pet.sick()) { pet.vetVisit(); markIcon(IC_MEDS); sound(SND_MEDS); startAnim(AN_MEDS, MEDS_SECS, pet.sickStack()); }   // the dose works at once (arg: sicknesses left). The medicine lesson: a sickness cured
      else startAnim(AN_REFUSE, 1.6f);
      selIcon_ = ic; return;
    case IC_LITTER:
      startScrub(); return;
    case IC_LOCK:   // asks first, either way
      sound(SND_MENU); lockSel_ = -1; setScreen(S_LOCK); selIcon_ = ic; return;
    default: break;
  }
}

// ---------------------------------------------------------------------------------------------
// the home LCD
// ---------------------------------------------------------------------------------------------
void Game::updateBlobWalk() {
  int idle, alt, sl;
  blobSprite(form(), idle, alt, sl);
  const int w = spriteW(idle);
  const int fm = pet.d.floorMess > 4 ? 4 : pet.d.floorMess;
  const bool boxOut = pet.d.droppings > 0;   // the litter box is out (bottom right) while there's poop in it
  // the whole floor is theirs, except the box and the mess at the right
  int xmax = fm || boxOut ? BOX_X - 1 - w - fm * 4 : MW - w;
  if (xmax < 0) xmax = 0;
  // and keep off a gift at the left, so it can be tapped on its own (a very wide fella with a full floor may
  // still have to stand partly over it)
  const int items = pet.d.giftOnFloor ? 9 : 0;
  int xmin = items ? items + 1 : 0;
  if (xmin > xmax) xmin = xmax;
  if (form() == F_BB_BLOBBERT) { if (!items) xmin = -3; if (!fm && !boxOut) xmax = MW - w + 3; }   // too big for the screen: it squishes against the sides
  updateBall();
  if (blobTx_ > xmax) blobTx_ = (float)xmax;
  if (blobX_ > xmax) blobX_ = (float)xmax;
  if (blobTx_ < xmin) blobTx_ = (float)xmin;
  if (blobX_ < xmin) blobX_ = (float)xmin;
  blinkT_ -= dt;
  if (blinkT_ < -0.15f) blinkT_ = pet.rndRange(2.0f, 5.0f);
  stepT_ -= dt;
  if (stepT_ > 0) return;
  stepT_ = pet.rndRange(0.45f, 0.9f);
  stepPhase_ ^= 1;
  if (pet.sleeping || form() == F_DROPPER) return;
  if (pet.rndf() < 0.3f) blobTx_ = (float)(int)pet.rndRange((float)xmin, (float)xmax + 0.99f);
  if (fabsf(blobX_ - blobTx_) >= 1) { faceLeft_ = blobTx_ < blobX_; blobX_ += blobTx_ < blobX_ ? -1 : 1; }
  else if (pet.rndf() < 0.15f) faceLeft_ = !faceLeft_;
}

// What's wrong with them, beside the head: the sickness (cycling through each kind they have) with how many
// they've got when it's more than one. Very low health without a sickness (neglect) shows the skull.
void Game::drawSickIcon(int cx, int cy, int w) {
  if (!((int)(anim_ * 2) & 1)) return;   // blinks
  const int n = pet.sickStack();
  int sp = -1;
  if (n) {
    int kinds[SICK_KINDS], nk = 0;
    for (int k = 0; k < SICK_KINDS; k++) if (pet.d.sick[k]) kinds[nk++] = k;
    const int k = kinds[((int)(anim_ / 1.0f)) % nk];
    sp = k == SICK_BUG ? SPR_SICK_BUG : k == SICK_HUNGER ? SPR_SICK_HUNGER : SPR_SICK_LIGHT;
  } else if (pet.d.health < 20) sp = SPR_SKULL;
  if (sp < 0) return;
  const int iw = spriteW(sp), tw = n > 1 ? 4 : 0;
  int ix = cx + w;                                       // right of the head, or left when there's no room
  if (ix + iw + tw > MW) ix = cx - iw - tw - 1;
  ix = clampi(ix, 0, MW - iw - tw);
  const int iy = clampi(cy - 3, 0, MH - spriteH(sp));
  m_.sprite(ix, iy, sp);
  if (n > 1) { char b[2] = {(char)('0' + (n > 9 ? 9 : n)), 0}; m_.small(ix + iw + 1, iy, b); }
}

// Draws the room contents (fella, mess, gifts). withBlob=false leaves the fella out.
void Game::drawScene(bool withBlob) {
  int idle, alt, sl;
  const Form f = form();
  blobSprite(f, idle, alt, sl);
  const int h = spriteH(idle);
  const int cx = (int)blobX_, cy = FLOOR - h;
  if (withBlob && !pet.d.exploring) {
    if (f == F_DROPPER) {   // hanging over the floor, goo inside: push it
      m_.sprite((MW - spriteW(SPR_DROPPER)) / 2, 2, SPR_DROPPER);
      if ((int)(anim_ * 2) & 1) m_.smallCenter(21, "PUSH");   // blinks under the dropper's tip
    } else if (pet.sleeping) {
      drawBlob(cx, cy, faceLeft_, 3, ((int)(anim_ * 1.2f) & 1));
      drawZzz(cx, cy, spriteW(idle), SH_INK);
    } else {
      const bool blink = blinkT_ < 0;
      drawBlob(cx, cy, faceLeft_, blink ? 1 : 0, stepPhase_ == 1);
      drawSickIcon(cx, cy, spriteW(idle));
      if (pet.grump > 0 && ((int)(anim_ * 2) & 1)) m_.sprite(clampi(cx - 3, 0, MW), clampi(cy - 2, 0, MH), SPR_ANGRY);
    }
  }
  if (pet.d.exploring && withBlob) {   // they're out exploring: a goo trail wobbles off the screen
    const int k = (int)(anim_ * 2) % 6;
    for (int i = 0; i < 3; i++) m_.sprite(8 + (i + k) * 5 % 30, FLOOR - 1 - ((i + k) & 1), SPR_TRAIL);
  }
  drawLitter(pet.d.droppings, pet.d.floorMess);
  // the exploring gift, bottom-left
  const int ix = 0;
  if (pet.d.giftOnFloor) {
    const uint8_t gt = pet.d.giftOnFloor;
    const int sp = gt == GIFT_SHELL ? SPR_SHELL : gt == GIFT_PEBBLE ? SPR_STONE : gt == GIFT_BERRY ? SPR_BERRY : gt == GIFT_GEM ? SPR_GEM : SPR_FLOWER;
    m_.sprite(ix, FLOOR - spriteH(sp), sp);
  }
}

// The litter box (bottom right) is only out while there's poop in it waiting to be cleaned: a tray, every poop
// they've done since it was last cleaned (up to BOX_MAX) and the smell. Six make a tidy pile. Leave it longer and
// it's a tower, one on another, swaying more the taller it gets: a fly finds it at 8, somebody plants a flag on the
// top at 10, and a second fly joins at 11. Floor mess sits in front of it, to the left.
void Game::drawLitter(int boxPoos, int floorPoos, bool box) {
  const bool st = (int)(anim_ * 2.5f) & 1;
  if (boxPoos > BOX_MAX) boxPoos = BOX_MAX;
  if (boxPoos >= 1 || box) {
    for (int x = BOX_X; x < BOX_X + BOX_W; x++) m_.set(x, FLOOR, SH_INK);                                  // the tray
    for (int y = FLOOR - 3; y < FLOOR; y++) { m_.set(BOX_X, y, SH_INK); m_.set(BOX_X + BOX_W - 1, y, SH_INK); }
    static const int8_t PX[6] = {1, 4, 7, 2, 6, 4}, PY[6] = {2, 2, 2, 4, 4, 6};   // three along the bottom, two on those, one on top
    static const int8_t LEAN[BOX_MAX - 6] = {1, 0, -1, 0, 1, 0};                  // then the tower: none of it quite straight
    int tx = BOX_X + 4, ty = FLOOR - 2;   // the one on top
    for (int i = 0; i < boxPoos; i++) {
      if (i < 6) { tx = BOX_X + PX[i]; ty = FLOOR - PY[i]; }
      else { tx = BOX_X + 4 + LEAN[i - 6] + (int)lroundf(sinf(anim_ * 1.7f) * (i - 5) * 0.3f); ty = FLOOR - 6 - 2 * (i - 5); }
      m_.sprite(tx, ty, SPR_POO);
    }
    const int top = boxPoos > 6 ? 6 + 2 * (boxPoos - 6) : boxPoos > 5 ? 6 : boxPoos > 3 ? 4 : 2;
    if (boxPoos > 6) {   // the smell comes off the sides of a tower (the top is taken)
      m_.sprite(BOX_X + 8, FLOOR - top + 1, st ? SPR_STINK1 : SPR_STINK2);
      m_.sprite(BOX_X, FLOOR - top - 1, st ? SPR_STINK2 : SPR_STINK1);
    } else {
      if (boxPoos >= 1) m_.sprite(BOX_X + 6, FLOOR - top - 5, st ? SPR_STINK1 : SPR_STINK2);
      if (boxPoos >= 3) m_.sprite(BOX_X + 1, FLOOR - top - 7, st ? SPR_STINK2 : SPR_STINK1);
    }
    for (int k = 0; k < (boxPoos >= 11 ? 2 : boxPoos >= 8 ? 1 : 0); k++) {   // flies
      const float a = anim_ * (6.0f + k * 2.5f) + k * 3.0f;
      m_.set(BOX_X + 5 + (int)lroundf(cosf(a) * 5.0f), FLOOR - 4 - top / 2 + (int)lroundf(sinf(a * 1.3f) * 3.0f), SH_INK);
    }
    if (boxPoos >= 10) {   // the flag: it flaps
      for (int y = ty - 4; y < ty; y++) m_.set(tx + 1, y, SH_INK);
      m_.set(tx + 2, ty - 4, SH_INK); m_.set(tx + 2, ty - 3, SH_INK); m_.set(tx + 3, st ? ty - 4 : ty - 3, SH_INK);
    }
  }
  for (int i = 0; i < floorPoos && i < 4; i++) {
    const int x = BOX_X - 4 - i * 4;
    m_.sprite(x, FLOOR - 1, SPR_POO);
    m_.sprite(x, FLOOR - 5, st ^ (i & 1) ? SPR_STINK1 : SPR_STINK2);
  }
}

// Three Zs float up from the sleeper's head, growing as they rise (z, Z, then a big Z), on a loop.
void Game::drawZzz(int cx, int cy, int w, uint8_t shade) {
  const bool right = cx + w + 6 <= MW;           // drift away from the nearer wall
  const int x0 = right ? cx + w - 3 : cx - 2, y0 = cy - 2;
  for (int i = 0; i < 3; i++) {
    float p = fmodf(anim_ * 0.45f + i / 3.0f, 1.0f);   // 0 = just appeared, 1 = gone
    const int sp = p < 0.33f ? SPR_ZSMALL : p < 0.66f ? SPR_ZZ : SPR_ZBIG;
    const int x = x0 + (right ? 1 : -1) * (int)(p * 7) - (right ? 0 : spriteW(sp));
    const int y = y0 - spriteH(sp) - (int)(p * 12) + (((int)(p * 12)) & 1);   // a little wobble on the way up
    if (y < -spriteH(sp)) continue;
    m_.sprite(clampi(x, 0, MW - spriteW(sp)), y, sp, false, 0, 0, shade);
  }
}

void Game::drawMain(bool interactive) {
  if (form() != F_DROPPER && sv.hasBlob) updateBlobWalk();
  if (lightOff()) {   // lights out: a dark LCD, only the Zs
    if (pet.sleeping && sv.hasBlob) {   // the Zs drift up from where they're sleeping
      int idle, alt, sl;
      blobSprite(form(), idle, alt, sl);
      drawZzz((int)blobX_, FLOOR - spriteH(idle), spriteW(idle), SH_FAINT);   // (shade 0 would read as an unlit dot)
    }
    if (clockShowT_ > 0) {   // a tap in the dark shows the time too (light dots on the dark LCD, over the Zs)
      char b[8];
      snprintf(b, sizeof b, "%02d:%02d", hour, minute);
      m_.rect(4, 0, 32, 9, 0);
      m_.text((MW - Matrix::textWidth(b)) / 2, 1, b, SH_FAINT);
    }
  } else {
    drawScene(true);
    if (clockShowT_ > 0) {
      char b[8];
      snprintf(b, sizeof b, "%02d:%02d", hour, minute);
      m_.rect(4, 0, 32, 9, 0);
      m_.textCenter(1, b);
    } else if (msgT_ > 0) {
      m_.rect(0, 0, MW, 6, 0);
      m_.smallCenter(0, msg_);
    }
  }
  if (interactive) mainInput();
}

void Game::mainInput() {
  int ic;
  if (iconTapped(ic)) { doIcon(ic); return; }
  int dx, dy;
  if (in.longPress && inMatrix(in.px, in.py, dx, dy)) { openSettings(); return; }
  if (sv.hasBlob && form() == F_DROPPER) {   // push the dropper (anywhere on the LCD): out comes the Lil Blob
    if (in.tap && inMatrix(in.px, in.py, dx, dy)) { squeezeOut(); sound(SND_POOF); }
    return;
  }
  if (!in.tap || !inMatrix(in.px, in.py, dx, dy)) return;
  if (lightOff()) { clockShowT_ = 3.0f; sound(SND_TAP); return; }   // in the dark nothing can be touched, but a tap still shows the time
  // Everything on the floor is small next to a finger, so the tap goes to the nearest thing within reach
  // (TOUCH_REACH dots of its outline), not only to a dead-on hit.
  auto dist = [&](int x, int y, int w, int h) {   // dots from the tap to a rectangle (0 = inside)
    const int ex = dx < x ? x - dx : dx >= x + w ? dx - (x + w - 1) : 0;
    const int ey = dy < y ? y - dy : dy >= y + h ? dy - (y + h - 1) : 0;
    return ex > ey ? ex : ey;
  };
  enum { T_NONE, T_GIFT, T_BOX, T_BLOB };
  int best = T_NONE, bestD = TOUCH_REACH + 1, bestC = 1 << 20;
  // nearest outline wins; when two overlap (a wide fella over a gift) the one whose middle is nearer wins
  auto consider = [&](int kind, int x, int y, int w, int h) {
    const int d = dist(x, y, w, h), cdx = 2 * dx - (2 * x + w - 1), cdy = 2 * dy - (2 * y + h - 1), c = cdx * cdx + cdy * cdy;
    if (d < bestD || (d == bestD && c < bestC)) { bestD = d; bestC = c; best = kind; }
  };
  if (pet.d.giftOnFloor) consider(T_GIFT, 0, FLOOR - 4, 8, 5);
  const bool hasPet = sv.hasBlob && form() != F_DROPPER;
  if (hasPet && !pet.d.exploring && !pet.sleeping) {
    int idle, alt, sl;
    blobSprite(form(), idle, alt, sl);
    const int h = spriteH(idle);
    consider(T_BLOB, (int)blobX_, FLOOR + 1 - h, spriteW(idle), h);
  }
  if (hasPet) {
    const int boxH = pet.d.droppings > 6 ? 10 + 2 * (pet.d.droppings - 6) : 10;   // (a tower of it is taller)
    if (pet.d.droppings) consider(T_BOX, BOX_X, FLOOR + 1 - boxH, BOX_W, boxH);   // the litter box: tap it to clean up
    for (int i = 0; i < pet.d.floorMess && i < 4; i++) consider(T_BOX, BOX_X - 4 - i * 4, FLOOR - 2, 3, 3);   // mess in front
  }
  switch (best) {
    case T_GIFT: pet.takeGift(); pet.petIt(5); sound(SND_COIN); say("THANKS!"); return;   // their treasure, for you
    case T_BOX: doIcon(IC_LITTER); return;   // tap the litter box to clean it
    case T_BLOB:
      if (petCd_ <= 0) { pet.petIt(4); petCd_ = 2.0f; sound(SND_PET); startAnim(AN_PET, 1.3f); }
      return;
  }
  clockShowT_ = 3.0f;   // empty spot: show the time
  sound(SND_TAP);
}

// ---------------------------------------------------------------------------------------------
// menus (feed / light / play)
// ---------------------------------------------------------------------------------------------
// Feed: a meal, every time (no menu)
void Game::feedMeal() {
  selIcon_ = IC_FEED;
  if (pet.sleeping && pet.d.hunger < 40) pet.wakeGently();       // they wake for a meal (and doze off again after)
  if (!pet.canFeed()) { startAnim(AN_REFUSE, 1.6f); return; }
  bool over = false;
  pet.feed(FOOD_KIBBLE, over);
  sound(SND_SCOOP);   // the food is served
  if (pet.d.evolving) pet.train(ACC_LOVE, 0.05f);
  sv.lifeFed++;
  startAnim(AN_EAT_MEAL, eatSecs());
}

// The Play menu: four games in big cells (small letters)
//   MOVE | RUN
//   YEET | POP
void Game::screenMenu() {
  static const char* play[4] = {"MOVE", "RUN", "YEET", "POP"};
  static const int8_t CX[4] = {10, 30, 10, 30}, CY[4] = {4, 4, 18, 18};   // the middle of each cell, the text's top
  int dx, dy;
  auto cellAt = [&](int x, int y) { return (y < 14 ? 0 : 2) + (x < 20 ? 0 : 1); };
  const int hover = in.down && inMatrix(in.px, in.py, dx, dy) ? cellAt(dx, dy) : -1;
  for (int x = 1; x < MW; x += 2) m_.set(x, 13, SH_MID);
  for (int y = 1; y < MH; y += 2) m_.set(20, y, SH_MID);
  for (int i = 0; i < 4; i++) {
    const int w = Matrix::smallWidth(play[i]);
    m_.small(CX[i] - w / 2, CY[i], play[i]);
    if (i == hover) m_.rect(CX[i] - w / 2, CY[i] + 6, w, 1, SH_INK);   // pressed: underlined
  }
  if (scrT_ > 15) { setScreen(S_MAIN); return; }
  int ic;
  if (iconTapped(ic)) { doIcon(ic); return; }
  if (!in.tap || !inMatrix(in.px, in.py, dx, dy)) return;
  sound(SND_TAP);
  switch (cellAt(dx, dy)) {
    case 0: memset(&dg_, 0, sizeof dg_); setScreen(S_DODGE); break;   // MOVE (it was Dodge)
    case 1: memset(&rn_, 0, sizeof rn_); setScreen(S_RUN); break;
    case 2: startYeet(); break;
    default: startPop(); break;                                      // POP: the bubble shooter (pop.cpp)
  }
  selIcon_ = IC_PLAY;
}

// ---------------------------------------------------------------------------------------------
// animations
// ---------------------------------------------------------------------------------------------
// A sprite stretched (or squashed) to w x h dots, nearest-neighbour: the body flowing toward the food.
void Game::drawSquish(int x, int y, int id, int w, int h, bool flip, uint8_t sick) {
  const Sprite& sp = SPRITES[id];
  int8_t rmin[32], rmax[32], cmin[40], cmax[40];   // (sick: the body's spans, for the speckle, as in Matrix::sprite)
  const bool spans = sick && sp.w <= 40 && sp.h <= 32;
  if (spans) {
    for (int r = 0; r < sp.h; r++) { rmin[r] = 127; rmax[r] = -1; }
    for (int c = 0; c < sp.w; c++) { cmin[c] = 127; cmax[c] = -1; }
    for (int r = 0; r < sp.h; r++) for (int c = 0; c < sp.w; c++) if (sp.px[r * sp.w + c]) {
      if (c < rmin[r]) rmin[r] = (int8_t)c;
      if (c > rmax[r]) rmax[r] = (int8_t)c;
      if (r < cmin[c]) cmin[c] = (int8_t)r;
      if (r > cmax[c]) cmax[c] = (int8_t)r;
    }
  }
  for (int dy = 0; dy < h; dy++)
    for (int dx = 0; dx < w; dx++) {
      // (end points map to end points, so squashing never drops the bottom row of the outline)
      const int mx = flip ? w - 1 - dx : dx;
      const int sx = w > 1 ? mx * (sp.w - 1) / (w - 1) : 0, sy = h > 1 ? dy * (sp.h - 1) / (h - 1) : 0;
      const uint8_t v = sp.px[sy * sp.w + sx];
      if (v) m_.set(x + dx, y + dy, v == 4 ? SH_MID : SH_INK);
      else if (spans && !((x + dx + y + dy) & 1) && sx > rmin[sy] && sx < rmax[sy] && sy > cmin[sx] && sy < cmax[sx]) m_.set(x + dx, y + dy, sick);
    }
}

// A sprite with extra dots let into it: `ew` columns in its side (between the face and the edge the food is on) and
// `eh` rows in its belly (under the eyes). The face keeps its shape and the outline stays one dot thick, so a body
// can reach out, or swell round a meal, without the eyes stretching (drawSquish scales everything).
void Game::drawBelly(int x, int y, int id, int ew, int eh, bool flip) {
  const Sprite& sp = SPRITES[id];
  int eyeR = sp.h / 2;
  for (int r = 0; r < sp.h; r++) for (int c = 0; c < sp.w; c++) if (sp.px[r * sp.w + c] == 2) eyeR = r;
  const int splitR = eyeR + 1 < sp.h - 1 ? eyeR + 1 : sp.h - 2, splitC = sp.w - 3;
  const int W2 = sp.w + ew, H2 = sp.h + eh;
  for (int dy = 0; dy < H2; dy++)
    for (int dx = 0; dx < W2; dx++) {
      const int mx = flip ? W2 - 1 - dx : dx;
      const int sx = mx <= splitC ? mx : mx <= splitC + ew ? splitC : mx - ew;
      const int sy = dy <= splitR ? dy : dy <= splitR + eh ? splitR : dy - eh;
      const uint8_t v = sp.px[sy * sp.w + sx];
      if (v) m_.set(x + dx, y + dy, v == 4 ? SH_MID : SH_INK);
    }
}

// The Lil Blob eats by absorbing, in one unbroken shot (no zoom, no cuts): the food lands on the floor, they wobble
// over to it, a side of the body flows out over the food and closes round it, the rest of the body follows and
// swells into a belly with the meal inside, under the eyes. There it dissolves dot by dot while the belly gurgles,
// and they shrink back to their own size, content. The food never jumps: it stays where it landed, lifting a
// little as the belly forms round it.
void Game::drawAbsorb(float t) {
  int idle, alt, sl;
  blobSprite(form(), idle, alt, sl);
  const int bw = spriteW(idle), bh = spriteH(idle), by = FLOOR - bh;
  const int food = an_ == AN_EAT_SNACK ? SPR_SNACK : SPR_MEAL;
  const Sprite& fs = SPRITES[food];
  const int fw = fs.w, fh = fs.h, fy = FLOOR - fh;
  // The food lands on the roomier side. If it lands close by, they hop back in surprise first, so there's a walk.
  // (From where they stood when it began, not blobX_: that is moved to where they end up before the last frame is
  // drawn, and laying the scene out again from there put the food, and them, on the other side for a frame.)
  const int startX = clampi(eatFromX_, 0, MW - bw);
  const bool right = startX + bw / 2 < MW / 2;          // food on the right, so they walk right
  const int fx0 = right ? MW - fw - 3 : 3;               // (room on its far side for the body to close round it)
  constexpr int GAP = 3;                                 // dots between them and the food when they stop
  const int stopX = right ? fx0 - GAP - bw : fx0 + fw + GAP;
  const int backX = right ? 1 : MW - bw - 1;             // where a startled hop back takes them
  const bool hopBack = abs(stopX - startX) < 10;
  const int walkFrom = hopBack ? backX : startX;
  // the belly: wide enough for the food with a dot to spare each side, tall enough to hold it under the eyes
  const int bellyW = (fw + 4 > bw ? fw + 4 - bw : 0), bellyH = fh;        // extra columns / rows at its fullest
  const int reachW = GAP + fw + 2;                                          // extra columns when it has flowed right over the food
  const int endX = right ? fx0 + fw + 2 - bw : fx0 - 2;                     // where they end up: round where the food was
  constexpr float T_HOP = 0.5f, T_WALK = 2.0f, T_REACH = 2.35f, T_ENGULF = 2.75f, T_PULL = 3.35f, T_DONE = 5.3f;
  auto ease = [](float k) { k = k < 0 ? 0 : k > 1 ? 1 : k; return k * k * (3 - 2 * k); };
  if (t < T_WALK) {
    if (t < T_HOP && hopBack) {      // boing: a startled hop back, away from the food
      const float k = t / T_HOP;
      drawBlob(startX + (int)((backX - startX) * k), by - (int)(4 * sinf(k * 3.14159f)), right, 0, true);
      if (t < 0.3f) m_.small(clampi(startX + bw / 2 - 1, 0, MW - 3), clampi(by - 9, 0, MH), "!");
    } else {                         // a slow wobble over, a dot at a time
      const float k = (t - (hopBack ? T_HOP : 0)) / (T_WALK - (hopBack ? T_HOP : 0));
      drawBlob(walkFrom + (int)((stopX - walkFrom) * k), by - (((int)(t * 5)) & 1), !right, 0, ((int)(t * 5)) & 1);
    }
    m_.sprite(fx0, fy - (t < 0.25f ? (int)((0.25f - t) * 40) : 0), food);   // it drops in
    return;
  }
  if (t >= T_DONE) {                 // full and content, eyes shut, where the food was
    drawBlob(endX, by, !right, 1, false);
    if (t < T_DONE + 0.6f && ((int)(t * 6) & 1)) m_.sprite(clampi(right ? endX - 3 : endX + bw, 0, MW - 3), clampi(by - 3, 0, MH), SPR_SPARK);
    blobX_ = blobTx_ = (float)endX; faceLeft_ = !right;   // they stay where they ate
    return;
  }
  // from here to T_DONE the body is drawn with drawBelly: ew extra columns, eh extra rows
  int ew = 0, eh = 0, lift = 0;
  bool nearFixed = false;            // which side stays put: the far one while reaching, the food's side after that
  float d = -1;                      // digesting: 0 -> 1
  if (t < T_REACH) ew = (int)lroundf(GAP * ease((t - T_WALK) / (T_REACH - T_WALK)));               // a side of the body reaches the food
  else if (t < T_ENGULF) {                                                                             // ...flows over it and closes round it
    const float k = ease((t - T_REACH) / (T_ENGULF - T_REACH));
    ew = GAP + (int)lroundf((reachW - GAP) * k); eh = (int)lroundf(1.0f * k);
  } else if (t < T_PULL) {                                                                             // the rest of the body follows: a belly, the food in it
    const float k = ease((t - T_ENGULF) / (T_PULL - T_ENGULF));
    ew = reachW + (int)lroundf((bellyW - reachW) * k); eh = 1 + (int)lroundf((bellyH - 1) * k);
    lift = (int)lroundf(2 * k); nearFixed = true;
  } else {                                                                                             // digesting
    d = (t - T_PULL) / (T_DONE - T_PULL);
    const float shrink = d < 0.72f ? 0.0f : ease((d - 0.72f) / 0.28f);                              // back to their own size once it's gone
    ew = (int)lroundf(bellyW * (1 - shrink)); eh = (int)lroundf(bellyH * (1 - shrink));
    lift = 2; nearFixed = true;
  }
  const bool gurgle = d >= 0 && d < 0.72f && ((int)(t * 5) & 1);   // the belly gurgles: a dot wider each side and a dot flatter, and back
  if (gurgle) { ew += 2; eh -= 1; }
  const int W2 = bw + ew, H2 = bh + eh;
  int bx;
  if (nearFixed) bx = right ? fx0 + fw + 2 - W2 : fx0 - 2;
  else bx = right ? stopX : stopX + bw - W2;
  if (gurgle) bx += right ? 1 : -1;
  drawBelly(bx, FLOOR - H2, idle, ew, eh, !right);
  // the food: whole until it's in the belly, then it goes soft and dissolves, each dot in its own time
  for (int r = 0; r < fh; r++) for (int c = 0; c < fw; c++) {
    if (!fs.px[r * fw + c]) continue;
    uint8_t sh = SH_INK;
    if (d >= 0) {
      uint32_t hx = (uint32_t)(r * fw + c + 1) * 0x9E3779B1u; hx ^= hx >> 15; hx *= 0x85EBCA6Bu; hx ^= hx >> 13;   // scrambled, so the bits left over never line up into shapes
      const float gone = 0.12f + (hx & 15) / 16.0f * 0.45f;   // when this dot starts to go
      if (d >= gone + 0.1f) continue;
      if (d >= gone) sh = SH_MID;
    }
    m_.set(fx0 + c, fy - lift + r, sh);
  }
  if (d > 0.2f && d < 0.8f) for (int i = 0; i < 2; i++) {   // little bubbles rise off them as it goes down
    const float a = fmodf((d - 0.2f) * 3.0f + i * 0.5f, 1.0f);
    m_.set(bx + W2 / 2 + (i ? 3 : -2) + (((int)(a * 6)) & 1), FLOOR - H2 - 1 - (int)(a * 5), SH_MID);
  }
  if (t - dt < T_ENGULF && t >= T_ENGULF) sound(SND_GULP);
  if (d >= 0.3f && (t - dt - T_PULL) / (T_DONE - T_PULL) < 0.3f) sound(SND_BUBBLE);
}

void Game::screenAnim() {
  anT_ += dt;
  const float t = anT_;
  int idle, alt, sl;
  const Form f = form();
  blobSprite(f, idle, alt, sl);
  const int w = spriteW(idle), h = spriteH(idle);
  const int cy = FLOOR - h;
  switch (an_) {
    case AN_EAT_MEAL: case AN_EAT_SNACK: case AN_EAT_DIET: {
      if (absorbsFood()) { drawAbsorb(t); break; }
      if (const int r = eatRoutine()) {   // their own routine: a whole scene, frame for frame as previewed
        const bool flash = eatfx::render(r, t, an_ != AN_EAT_SNACK, eatVar_, m_.d);
        if (flash) for (int i = 0; i < MW * MH; i++) m_.d[i] = m_.d[i] == 0 ? SH_INK : m_.d[i] == SH_INK ? 0 : m_.d[i];
        int n = 0; const float* cue = eatfx::chomps(r, n);
        for (int i = 0; i < n; i++) if (t - dt < cue[i] && t >= cue[i]) sound(SND_EAT);
        break;
      }
      // 3 bites: mouth wide open on the food, then a chomp (mouth shut, eyes closed) that takes a piece off
      const int bite = (int)(t / 0.8f);
      const bool open = bite < 3 && fmodf(t, 0.8f) < 0.4f;
      const int left = bite >= 3 ? 3 : bite + (open ? 0 : 1);   // pieces eaten so far
      const int bw = spriteW(idle);
      const int cx = (MW - bw - 11) / 2;
      if (open) m_.sprite(cx, cy, eatSprite(idle), false, 0, coatFill());
      else drawBlob(cx, cy - (bite < 3 && fmodf(t, 0.8f) < 0.55f ? 1 : 0), false, 1, false);   // chomp: a little bounce
      static const uint8_t meal[3] = {SPR_MEAL, SPR_MEAL2, SPR_MEAL3};
      static const uint8_t snack[3] = {SPR_SNACK, SPR_SNACK2, SPR_SNACK3};
      if (left < 3) {
        const int sp = an_ == AN_EAT_SNACK ? snack[left] : meal[left];
        m_.sprite(cx + bw + 2, FLOOR - spriteH(sp), sp);
      }
      if (bite < 3 && fabsf(fmodf(t, 0.8f) - 0.4f) < dt * 0.75f) sound(SND_EAT);   // on the chomp
      break;
    }
    case AN_REFUSE: {
      const bool fl = ((int)(t * 4)) & 1;
      const int cx = (MW - w) / 2;
      drawBlob(cx, cy, fl, 0, false);
      m_.sprite(clampi(cx + w, 0, MW - 3), clampi(cy - 2, 0, MH), SPR_ANGRY);
      if (t < dt * 1.5f) sound(SND_NOPE);
      break;
    }
    case AN_MEDS: {
      const int cx = w > MW - 12 ? MW - 8 - w : 4;   // (the very widest squeeze up against the wall to leave room for it)
      drawBlob(cx, cy, false, t > 1.2f ? 1 : 0, false);
      // The needle goes to where their side really is: the dot of them furthest right at the needle's height, or
      // at the nearest height they have any (a short fella is jabbed lower down). It comes in until it touches
      // them, then goes in by a dot.
      int ny = FLOOR - 6, nx = -1;
      for (int k = 0; k < 2 * MH && nx < 0; k++) {   // FLOOR - 6, then a row lower, a row higher, two lower...
        const int y = FLOOR - 6 + ((k & 1) ? (k + 1) / 2 : -(k / 2));
        if (y < 0 || y > FLOOR) continue;
        for (int x = MW - 1; x >= 0 && nx < 0; x--) if (m_.get(x, y)) { nx = x; ny = y; }
      }
      if (nx < 0) nx = cx + w - 1;
      const int sx = t < 1.0f ? MW - (int)((MW - (nx + 1)) * t) : t < 1.1f ? nx + 1 : nx;
      m_.sprite(sx, ny - 2, SPR_SYRINGE);
      if (t > 1.2f && ((int)(t * 6) & 1)) m_.sprite(clampi(cx + w, 0, MW - 3), clampi(cy - 3, 0, MH), SPR_SPARK);
      if (t >= 1.2f && t - dt < 1.2f) sound(SND_SPARKLE);   // it's working
      break;
    }
    case AN_POTTY: {   // arg 1: in the box, 2: on the floor (a protest). Walk over, squat, go, step out.
      const bool box = anArg_ == 1;
      const int home = clampi((int)blobX_, 0, MW - w);
      // in the box: they walk over to it (the box comes out for them), squat over it, and step back off it
      const int spot = box ? clampi(BOX_X + BOX_W / 2 - w / 2, 0, MW - w) : home;
      const int fm = pet.d.floorMess > 4 ? 4 : pet.d.floorMess;
      const int back = box ? clampi(BOX_X - 1 - w - fm * 4, 0, home) : home;   // (where they may stand once the box is out)
      const bool done = t > 1.9f;                         // the poop appears here
      const bool squat = t >= 0.5f && t < 2.2f;
      const int x = t < 0.5f ? home + (int)lroundf((spot - home) * t / 0.5f) : t < 2.2f ? spot : spot + (int)lroundf((back - spot) * ((t - 2.2f) / 0.5f > 1 ? 1 : (t - 2.2f) / 0.5f));
      drawLitter(pet.d.droppings - (box && !done ? 1 : 0), pet.d.floorMess - (!box && !done ? 1 : 0), box);
      drawBlob(x, cy, false, squat ? 1 : 0, (t < 0.5f || t >= 2.2f) && ((int)(t * 6) & 1));
      if (squat && !done && ((int)(t * 4) & 1)) { m_.set(clampi(x + w, 0, MW - 1), cy + 1, SH_INK); m_.set(clampi(x + w + 1, 0, MW - 1), cy, SH_INK); }   // straining
      if (!box && ((int)(t * 3) & 1)) m_.sprite(clampi(x + w, 0, MW - 3), clampi(cy - 2, 0, MH), SPR_ANGRY);
      if (done && box && t < 2.4f) m_.sprite(clampi(x + w / 2 - 1, 0, MW - 3), clampi(cy - 3, 0, MH), SPR_SPARK);
      if (t >= 0.5f && t - dt < 0.5f) sound(box ? SND_SCRATCH : SND_GRUMBLE);   // they squat (a protest: with a grumble)
      if (t >= 1.9f && t - dt < 1.9f) sound(SND_PLOP);
      break;
    }
    case AN_LOCK: {   // the padlock snaps shut (arg 1) or springs open (arg 0) over them
      const int cx = (MW - w) / 2;
      drawBlob(cx, cy, false, anArg_ && t > 0.5f ? 1 : 0, !anArg_ && ((int)(t * 6) & 1));
      const bool shut = anArg_ ? t > 0.5f : t < 0.4f;
      const int lx = MW / 2 - 4, ly = clampi(cy - 11, 0, MH - 9);
      // a little padlock: the body, and the shackle shut or swung open
      m_.rect(lx, ly + 4, 8, 5, SH_INK); m_.rect(lx + 3, ly + 5, 2, 2, SH_BG);
      if (shut) { m_.rect(lx + 1, ly + 1, 1, 3, SH_INK); m_.rect(lx + 6, ly + 1, 1, 3, SH_INK); m_.rect(lx + 2, ly, 4, 1, SH_INK); }
      else { m_.rect(lx + 6, ly, 1, 4, SH_INK); m_.rect(lx + 7, ly - 1 < 0 ? 0 : ly - 1, 4, 1, SH_INK); m_.rect(lx + 11, ly, 1, 2, SH_INK); }
      if (anArg_ && t > 0.5f && t - dt <= 0.5f) sound(SND_LOCK);
      if (!anArg_ && t > 0.4f && t - dt <= 0.4f) sound(SND_UNLOCK);
      if (t > 0.6f && ((int)(t * 6) & 1)) { m_.sprite(clampi(lx - 5, 0, MW - 3), ly + 2, SPR_SPARK); m_.sprite(clampi(lx + 13, 0, MW - 3), ly + 5, SPR_SPARK); }
      break;
    }
    case AN_PET: {   // eyes closed, a heart floats up
      const int cx = (int)blobX_;
      drawBlob(cx, cy, faceLeft_, 1, false);
      const int hy = clampi(cy - 2 - (int)(t * 4), 0, MH);
      m_.sprite(clampi(cx + w / 2 - 3, 0, MW - 7), hy, SPR_HEART_E);
      break;
    }
    case AN_SQUEEZE: {
      // Push: the bulb squeezes and a drop swells at the tip. Then it falls a long way, as if from the sky: the view
      // follows it down (the dropper slides off the top, clouds and streaks rush up past it), the floor comes up to
      // meet it, splat, and there's the Lil Blob.
      const int dw = spriteW(SPR_DROPPER), dx0 = (MW - dw) / 2, tipX = dx0 + dw / 2, tipY = 2 + spriteH(SPR_DROPPER);
      const int bx = tipX - spriteW(SPR_BLOBDROP) / 2;
      const float T_SWELL = 0.9f, T_FLOOR = 4.3f, T_LAND = 5.0f;
      if (t >= T_SWELL && t - dt < T_SWELL) sound(SND_BLOW);     // the drop lets go and whistles down
      if (t >= T_LAND - 0.42f && t - dt < T_LAND - 0.42f) sound(SND_SAD);   // "uh oh": the floor is right there
      { const float L = t - T_LAND; if (L >= 1.10f && L - dt < 1.10f) sound(SND_BOING);   // it springs up out of the goo
        if (L >= 2.40f && L - dt < 2.40f) sound(SND_COO); }                                // hello!
      if (t < T_SWELL) {
        m_.sprite(dx0, 2, t < 0.5f ? SPR_DROPPER_SQ : SPR_DROPPER);
        if (t < 0.5f) m_.set(tipX, tipY, SH_MID);                          // a bead of goo at the tip
        else m_.sprite(bx, tipY - 1, SPR_BLOBDROP);                          // it swells
      } else if (t < T_LAND) {
        const float ft = t - T_SWELL;
        const int scroll = (int)(ft * 28);                                  // how far the view has followed it down
        if (2 - scroll + spriteH(SPR_DROPPER) > 0) m_.sprite(dx0, 2 - scroll, SPR_DROPPER);
        if (t < T_FLOOR) {
          for (int i = 0; i < 7; i++) {                                     // streaks rushing up past the drop
            const int sx = (i * 6 + 2) % MW;
            if (sx >= tipX - 3 && sx <= tipX + 3) continue;
            const int sy = MH - ((i * 13 + (int)(ft * 45)) % (MH + 8));
            m_.rect(sx, sy, 1, 3, SH_MID);
          }
          for (int c = 0; c < 2; c++) {                                     // two clouds float by
            const int cy0 = MH + 4 - (int)((ft - 0.4f - c * 1.3f) * 18);
            if (cy0 > -4 && cy0 < MH + 4) m_.sprite(c ? MW - 9 : 1, cy0, SPR_SKYCLOUD);
          }
        }
        const float k = t < T_FLOOR ? 0 : (t - T_FLOOR) / (T_LAND - T_FLOOR);   // the last stretch: down to the floor
        const int fallY = 11 + (t < T_FLOOR && ((int)(t * 5) & 1) ? 1 : 0);
        const int dy = (int)(fallY + k * k * (FLOOR - 3 - fallY));
        m_.sprite(bx, dy, ((int)(t * 7) & 1) ? SPR_DROPLONG : SPR_BLOBDROP);
      } else {
        // Splat! The goo spreads flat and flings a few droplets. It pulls itself back into a heap, springs up too
        // tall, squishes down, bounces and wobbles until it settles as the Lil Blob; the droplets slide back in.
        const float L = t - T_LAND;
        if (L < dt * 1.5f) sound(SND_POOF);
        struct Key { float at; int spr; };
        static const Key keys[] = {{0.00f, SPR_GOO_SPLAT}, {0.30f, SPR_GOO_POOL}, {0.50f, SPR_GOO_MOUND}, {0.65f, SPR_GOO_POOL},
                                   {0.85f, SPR_GOO_MOUND}, {1.10f, SPR_BLOB_RISE}, {1.35f, SPR_BLOB_SQUASH}, {1.55f, SPR_BLOB_TALLISH},
                                   {1.75f, SPR_BLOB_WIDE}, {1.92f, -1}, {2.10f, SPR_BLOB_WIDE}, {2.22f, -1}};
        int spr = -1;
        for (const Key& k : keys) if (L >= k.at) spr = k.spr;
        int bw = w;
        if (spr < 0) drawBlob((MW - w) / 2, cy, false, L > 2.4f && L < 2.55f ? 1 : 0, false);   // settled (and a blink)
        else { bw = spriteW(spr); m_.sprite((MW - bw) / 2, FLOOR - spriteH(spr), spr); }
        // the flung droplets: up and out, back down to the floor, then they slide back into the blob
        static const float vx[4] = {-15, -8, 9, 16}, vy[4] = {-20, -27, -26, -19};
        constexpr float G = 70.0f;
        for (int i = 0; i < 4; i++) {
          const float tl = -2 * vy[i] / G;                                   // time in the air
          float x = tipX + vx[i] * (L < tl ? L : tl), y = FLOOR - 1 + vy[i] * L + 0.5f * G * L * L;
          if (L >= tl) {
            y = FLOOR - 1;
            const float back = L - 1.1f;                                      // they creep back once it's a blob again
            if (back > 0) { const float edge = vx[i] < 0 ? (MW - bw) / 2.0f : (MW + bw) / 2.0f - 1; x += (edge - x) * (back * 1.6f > 1 ? 1 : back * 1.6f); }
            if (back > 0.62f) continue;                                       // absorbed
          }
          m_.set((int)x, (int)y, SH_MID);
        }
        if (L > 2.4f && ((int)(t * 8) & 1)) { m_.sprite(4, 6, SPR_SPARK); m_.sprite(MW - 8, 4, SPR_SPARK); }
      }
      break;
    }
    case AN_LEVELUP: {   // small, big, small, big, small, BIG: each swell a little bigger, more stars each time
      constexpr float BEAT = 0.36f;                      // one small-or-big beat; six of them, then a flash
      const int beat = (int)(t / BEAT);
      float sc = 1.0f;
      if (beat < 6) {
        static const float S[7] = {1.0f, 0.84f, 1.16f, 0.84f, 1.24f, 0.82f, 1.36f};
        const float k = fmodf(t, BEAT) / BEAT, e = k * k * (3 - 2 * k);
        sc = S[beat] + (S[beat + 1] - S[beat]) * e;
      } else sc = 1.36f;
      const int sw = (int)lroundf(w * sc), sh = (int)lroundf(h * sc);
      if (t >= 6 * BEAT + 0.12f && ((int)(t * 14) & 1)) m_.rect(0, 0, MW, MH, SH_INK);   // the flash: here it comes
      else {
        drawSquish((MW - sw) / 2, FLOOR - sh, idle, sw, sh, false);
        // stars around them: two to start, a pair more with every swell, twinkling
        static const int8_t ST[6][2] = {{3, 5}, {MW - 8, 8}, {8, 14}, {MW - 6, 17}, {1, 20}, {MW - 12, 2}};
        const int stars = 2 + 2 * ((beat + 1) / 2 > 2 ? 2 : (beat + 1) / 2);
        for (int i = 0; i < stars && i < 6; i++) if (((int)(t * 9) + i) % 3) m_.sprite(ST[i][0], ST[i][1], SPR_SPARK);
      }
      // sounds: a soft blip going small, a boing on each swell, a coin sparkle on the last and biggest
      for (int i = 0; i < 6; i++) {
        const float at = i * BEAT + BEAT * 0.5f;
        if (t >= at && t - dt < at) sound(i == 5 ? SND_COIN : (i & 1) ? SND_BOING : SND_TAP);
      }
      blobX_ = blobTx_ = (float)((MW - w) / 2);
      break;
    }
    case AN_GROW: {
      if (t >= 1.2f && t - dt < 1.2f) sound(SND_COO);   // the new look says hello
      if (t < 1.2f && ((int)(t * 6) & 1)) m_.rect(0, 0, MW, MH, SH_INK);
      else {
        drawBlob((MW - w) / 2, cy, false, 0, false);
        if ((int)(t * 8) & 1) { m_.sprite(2, 4, SPR_SPARK); m_.sprite(MW - 6, 8, SPR_SPARK); }
      }
      blobX_ = blobTx_ = (float)((MW - w) / 2);
      break;
    }
    default: break;
  }
  if (anT_ < anLen_) return;
  // finished: apply the effect
  switch (an_) {
    case AN_MEDS: {   // one dose cured one sickness (when it was given)
      const int left = anArg_;
      if (left) { char b[12]; snprintf(b, sizeof b, "%d MORE", left); say(b); sound(SND_COO_SOFT); }
      else { say("BETTER"); sound(SND_WIN); }
      break;
    }
    case AN_LOCK: say(anArg_ ? "LOCKED" : "UNLOCKED", 2.0f); break;
    case AN_SQUEEZE: case AN_GROW: say(lifeStageName(), 3.0f); sound(SND_LEVEL); break;   // "LIL BLOB!" etc.; the fanfare lands with the title
    case AN_LEVELUP:
      an_ = AN_NONE;
      if (pendingEvolution()) {   // now the picker
        evoSel_ = 0; sound(SND_WIN); setScreen(S_EVOLVE);
        if (in.down) swallow_ = true;   // a finger that was already down doesn't pick a look
        return;
      }
      break;
    default: break;
  }
  an_ = AN_NONE;
  setScreen(S_MAIN);
}

// ---------------------------------------------------------------------------------------------
// meter pages
// ---------------------------------------------------------------------------------------------
static void hearts(Matrix& m, int y, float v) {
  int n = v < 5 ? 0 : (int)ceilf(v / 25.0f);
  if (n > 4) n = 4;
  for (int i = 0; i < 4; i++) m.sprite(3 + i * 9, y, i < n ? SPR_HEART : SPR_HEART_E);
}

// LOCK (the padlock icon): asks before locking or unlocking.
//   locked: they stay at this stage and nothing ends their life (hunger, mess and sickness carry on)
void Game::screenLock() {
  const bool was = locked();
  if (was) { m_.smallCenter(1, "UNLOCK?"); m_.smallCenter(8, "THEY GROW"); m_.smallCenter(14, "AGAIN"); }
  else { m_.smallCenter(1, "LOCK?"); m_.smallCenter(8, "NO GROWING"); m_.smallCenter(14, "NO DYING"); }
  int dx, dy;
  const int hover = in.down && inMatrix(in.px, in.py, dx, dy) && dy >= 18 ? (dx < 20 ? 0 : 1) : -1;
  for (int x = 1; x < MW; x += 2) m_.set(x, 20, SH_MID);
  for (int y = 21; y < MH; y += 2) m_.set(20, y, SH_MID);
  m_.small(10 - Matrix::smallWidth("YES") / 2, 22, "YES"); m_.small(30 - Matrix::smallWidth("NO") / 2, 22, "NO");
  if (hover == 0) m_.rect(4, 27, 12, 1, SH_INK); else if (hover == 1) m_.rect(26, 27, 8, 1, SH_INK);
  if (scrT_ > 15) { setScreen(S_MAIN); return; }
  int ic;
  if (iconTapped(ic)) { if (ic == IC_LOCK) { sound(SND_BACK); setScreen(S_MAIN); } else doIcon(ic); return; }
  if (!in.tap || !inMatrix(in.px, in.py, dx, dy) || dy < 18) return;
  sound(SND_TAP);
  if (dx < 20) { setLocked(!was); startAnim(AN_LOCK, 1.8f, was ? 0 : 1); }   // (the padlock makes its own sound)
  else { sound(SND_BACK); setScreen(S_MAIN); }
}

static constexpr int METER_PAGES = 5;
void Game::screenMeter() {
  const PetData& d = pet.d;
  char b[16];
  switch (meterPage_) {
    case 0: {
      char nm[11]; strncpy(nm, d.name, 10); nm[10] = 0;
      for (char* p = nm; *p; p++) if (*p >= 'a' && *p <= 'z') *p = (char)(*p - 32);
      m_.smallCenter(2, nm);
      {   // "AGE 5 HRS": the unit sits a few dots away from the number
        const char* unit = d.age < DAY ? "HRS" : "D";
        snprintf(b, sizeof b, "AGE %d", d.age < DAY ? (int)(d.age / HOUR) : (int)(d.age / DAY));
        const int uw = Matrix::smallWidth(unit), gap = 4;
        const int tw = Matrix::smallWidth(b) + gap + uw;
        const int x0 = (MW - tw) / 2;
        m_.small(x0, 10, b);
        m_.small(x0 + tw - uw, 10, unit);
      }
      {   // the device's battery: the percentage, nothing else ("+" while it's charging)
        const int pct = plat->batteryPercent();
        if (pct < 0) m_.smallCenter(18, "BATT -");
        else {
          snprintf(b, sizeof b, "BATT %d%%%s", pct > 100 ? 100 : pct, plat->batteryCharging() ? "+" : "");
          m_.smallCenter(18, b);
        }
      }
      break;
    }
    case 1: m_.textCenter(2, "HUNGRY"); hearts(m_, 15, d.hunger); break;
    case 2: m_.textCenter(2, "HAPPY"); hearts(m_, 15, d.fun); break;
    case 3: m_.textCenter(2, "TRUST"); hearts(m_, 15, 100 - d.resent); break;
    default: {   // their treasures
      snprintf(b, sizeof b, "GIFTS %u", (unsigned)(d.giftCount > 999 ? 999 : d.giftCount));
      m_.smallCenter(2, b);
      snprintf(b, sizeof b, "SICK %u", (unsigned)sv.sickCount);
      m_.smallCenter(9, b);
      m_.smallCenter(17, lifeStageName(), SH_MID);   // (clear of the page dots on the bottom row)
      break;
    }
  }
  // page dots
  for (int i = 0; i < METER_PAGES; i++) m_.set(12 + i * 2, MH - 1, i == meterPage_ ? SH_INK : SH_MID);
  if (scrT_ > 20) { setScreen(S_MAIN); return; }
  int ic;
  if (iconTapped(ic)) {
    if (ic == IC_METER) { if (++meterPage_ >= METER_PAGES) { sound(SND_BACK); setScreen(S_MAIN); } else sound(SND_PAGE); }
    else doIcon(ic);
    return;
  }
  int dx, dy;
  if (in.tap && inMatrix(in.px, in.py, dx, dy)) { scrT_ = 0; if (++meterPage_ >= METER_PAGES) { sound(SND_BACK); setScreen(S_MAIN); } else sound(SND_PAGE); }
}


// ---------------------------------------------------------------------------------------------
// MOVE (it was DODGE). Things drop from the ceiling; tap or slide a finger anywhere to move them.
// One hit ends it. The score is how long they lasted: it speeds up until nobody can survive past ~30 s.
// ---------------------------------------------------------------------------------------------
static int dodgeHearts(float secs) { return secs >= 27 ? 4 : secs >= 20 ? 3 : secs >= 13 ? 2 : secs >= 6 ? 1 : 0; }
// What falls: the young (playing small) get the full mix, books included; for bigger looks the drops get
// smaller and lighter, so a wide fella still has a chance but has a harder time.
static const uint8_t DROP_KINDS[3][4] = {
  {SPR_BALL, SPR_BOOK, SPR_SOCK, SPR_BOOK},      // Lil Blob, Lil Blub
  {SPR_DROP, SPR_SOCK, SPR_PEBBLE, SPR_BALL},    // Lil Blunkus
  {SPR_PEBBLE, SPR_DROP, SPR_PEBBLE, SPR_PEBBLE},  // Lil Fella
};
void Game::screenDodge() {
  const int sp = gameSprite();
  const int w = spriteW(sp), h = spriteH(sp), cy = FLOOR - h;
  const int tier = stageOfForm(form()) <= 1 ? 0 : stageOfForm(form()) == 2 ? 1 : 2;
  const float roomy = 1.0f + 0.04f * (w > 10 ? w - 10 : 0);   // wider fellas get fewer drops (still harder, not hopeless)
  Dodge& g = dg_;
  int ic;
  // only the bottom icons work here (the top ones are hidden so the whole top can steer)
  if (g.phase < 2 && iconTapped(ic) && ic >= 4) { doIcon(ic); return; }
  char b[16];
  if (g.phase == 0) {
    memset(&g, 0, sizeof g);
    g.x = g.tx = (float)((MW - w) / 2); g.next = 1.2f; g.phase = 1;
  }
  g.t += dt;
  if (g.phase == 1) {
    static const float HEART_AT[4] = {6.0f, 13.0f, 20.0f, 27.0f};
    for (float th : HEART_AT) if (g.t >= th && g.t - dt < th) sound(SND_HEART);   // another heart earned
    // steer: they run toward the finger, anywhere on the screen
    if (in.down) g.tx = (float)((in.x - MX) / DOT - w / 2);
    if (g.tx < 0) g.tx = 0;
    if (g.tx > MW - w) g.tx = (float)(MW - w);
    const float walk = 34.0f * dt;
    if (g.x < g.tx) { g.x += walk; if (g.x > g.tx) g.x = g.tx; g.left = false; }
    else if (g.x > g.tx) { g.x -= walk; if (g.x < g.tx) g.x = g.tx; g.left = true; }
    // difficulty: gentle at first, then it climbs steeply: k = 1 at 30 s, where it's meant to be unbeatable
    const float k = g.t / 30.0f;
    g.next -= dt;
    if (g.next <= 0) {
      const int n = (k < 0.7f ? 1 : k < 0.9f ? 2 : 3) + (tier == 0 && k > 0.8f ? 1 : 0);   // the small young get one more at the end
      for (int m = 0; m < n; m++)
        for (int i = 0; i < 10; i++) if (!g.it[i].on) {
          auto& it = g.it[i];
          it.on = true; it.y = -4 - m * 3;
          it.kind = DROP_KINDS[tier][pet.rnd() % 4];
          it.w = (uint8_t)spriteW(it.kind); it.h = (uint8_t)spriteH(it.kind);
          const bool aim = k > 0.6f && pet.rndf() < 1.5f * (k - 0.6f) / roomy;   // drop it right on top of them
          it.x = aim ? (float)clampi((int)g.x + (int)pet.rndRange(-1, (float)w - 1), 0, MW - it.w) : (float)(int)pet.rndRange(0, MW - it.w - 0.01f);
          it.v = 8.0f + 30.0f * k * k * k + pet.rndRange(0, 2);
          break;
        }
      float gap = 1.0f - 0.8f * k * k;
      g.next = ((gap < 0.1f ? 0.1f : gap) + pet.rndRange(0, 0.25f * (1 - k > 0 ? 1 - k : 0))) * roomy;
    }
    const int cx = (int)g.x;
    for (int i = 0; i < 10; i++) {
      auto& it = g.it[i];
      if (!it.on) continue;
      it.y += it.v * dt;
      if (it.y + it.h - 1 >= cy + 2 && it.y < FLOOR && it.x + it.w > cx + 1 && it.x < cx + w - 1) {   // hit: game over
        g.phase = 2; g.score = g.t; g.t = 0; g.hitX = (int)it.x; it.on = false;
        sound(SND_GRUMBLE);
        const int hs = dodgeHearts(g.score);
        pet.exercise(4.0f + 6.0f * hs, 0.4f + 0.3f * hs);
        if (pet.d.evolving) pet.train(ACC_SPEED, 0.15f * hs);
        sv.lifeGames++;
        if (g.score > dodgeBest_) dodgeBest_ = g.score;
        break;
      }
      if (it.y > FLOOR) { it.on = false; sound(SND_PLOP); }   // a miss: it thuds on the floor
    }
    if (g.phase == 1) {
      drawGameBlob(cx, cy, g.left, 0, ((int)(g.t * 6) & 1) && g.x != g.tx);
      for (int i = 0; i < 10; i++) if (g.it[i].on) m_.sprite((int)g.it[i].x, (int)g.it[i].y, g.it[i].kind);
      snprintf(b, sizeof b, "%d", (int)g.t);
      m_.small(MW - Matrix::smallWidth(b), 0, b, SH_MID);
      return;
    }
  }
  if (g.phase == 2) {   // the hit: a moment to see it, then the score
    drawGameBlob((int)g.x, cy, g.left, 1, false);
    m_.sprite(clampi(g.hitX, 0, MW - 3), clampi(cy - 3, 0, MH), SPR_ANGRY);
    if (g.t > 1.0f) { g.phase = 3; g.t = 0; sound(dodgeHearts(g.score) >= 3 ? SND_WIN : SND_LOSE); }
    return;
  }
  // score: seconds lasted, hearts for how close to the 27-30 s "perfect" run
  m_.clear();   // (the score on a clear screen: nothing of the game behind it)
  snprintf(b, sizeof b, "%.1fS", (double)g.score);
  m_.textCenter(0, b);
  hearts(m_, 9, dodgeHearts(g.score) * 25.0f);
  m_.smallCenter(17, g.score >= 27 ? "PERFECT!" : g.score >= 20 ? "GREAT" : g.score >= 13 ? "GOOD" : g.score >= 6 ? "OK" : "OOPS");
  snprintf(b, sizeof b, "BEST %.1f", (double)dodgeBest_);
  m_.smallCenter(MH - 5, b, SH_MID);
  if (g.t > 0.6f && in.tap) setScreen(S_MAIN);
}

// ---------------------------------------------------------------------------------------------
// RUN: a side-scroller. They run on the spot, things come at them from the right, tap to jump.
// ---------------------------------------------------------------------------------------------
// 4 hearts = 300 m AND a double jump cleared over a tall wall
static int runHearts(float pts, bool wall) { return pts >= 300 && wall ? 4 : pts >= 65 ? 3 : pts >= 35 ? 2 : pts >= 12 ? 1 : 0; }

// kinds: 0 block, 1 crate, 2 spiky mat, 3 hole, 4 bird (stay low!), 5 tall pillar (needs a double jump), 6 power-up bolt
static constexpr float RUN_GRAV = 110.0f, RUN_JUMP = 49.0f, RUN_JUMP2 = 45.0f, RUN_BOOST = 1.6f, RUN_BOOST_SECS = 5.0f;
static constexpr int RUN_FX = 3;

void Game::runStep(float dtReal, bool press, bool sim) {
  const int sp = gameSprite();
  const int w = spriteW(sp), h = spriteH(sp);
  Run& r = rn_;
  const float dtw = dtReal * (r.boost > 0 ? RUN_BOOST : 1.0f);   // while boosted the whole world runs faster
  r.t += dtw;
  const float speed = 24.0f + 24.0f * (r.t > 40 ? 1.0f : r.t / 40.0f);
  if (r.boost > 0) r.boost -= dtReal;
  if (r.phase == 1) {
    r.dist += speed * dtw;
    if (press) {
      if (r.y <= 0.01f && r.vy <= 0) { r.vy = RUN_JUMP; r.air2 = 0; if (!sim) sound(SND_BOING); }
      else if (!r.air2 && r.y > 0.01f) { r.vy = RUN_JUMP2; r.air2 = 1; if (!sim) sound(SND_BOUNCE); }
    }
    if (r.y > 0 || r.vy > 0) { r.vy -= RUN_GRAV * dtw; r.y += r.vy * dtw; if (r.y <= 0) { r.y = 0; r.vy = 0; r.air2 = 0; } }
    r.next -= speed * dtw;
    if (!sim && r.next <= 0) {
      for (int i = 0; i < 6; i++) if (!r.ob[i].on) {
        auto& o = r.ob[i];
        int pool[8], n = 0;
        pool[n++] = 0; pool[n++] = 1; pool[n++] = 2;
        if (r.t > 6) pool[n++] = 3;
        if (r.t > 12) pool[n++] = 5;
        if (r.t > 16) { pool[n++] = 4; pool[n++] = 3; }
        int kind = pool[pet.rnd() % n];
        // power-ups: never in the same place twice. How many things go by before the next one is drawn fresh each
        // time (2-9, so the first can come within seconds or not for a while), and so are its height and how far
        // out it sits (below).
        if (r.nextP <= 0) r.nextP = 2 + (int)(pet.rnd() % 8);
        if (r.t > 3 && r.boost <= 0 && ++r.sinceP >= r.nextP) { kind = 6; r.sinceP = 0; r.nextP = 2 + (int)(pet.rnd() % 8); }
        if (!r.wallDone && r.dist / 4.0f > 120) {   // the 4th heart needs a tall wall: if none is on the way, send one
          bool wallComing = false;
          for (int k = 0; k < 6; k++) if (r.ob[k].on && r.ob[k].kind == 5) wallComing = true;
          if (!wallComing) kind = 5;
        }
        o.on = true; o.kind = (uint8_t)kind; o.x = (float)MW; o.by = 0; o.dj = 0;
        switch (kind) {
          case 0: o.w = 4; o.h = 4; break;
          case 1: o.w = 3; o.h = r.t < 15 ? 5 : 6; break;
          case 2: o.w = 7; o.h = 3; break;
          case 3: o.w = (uint8_t)(9 + pet.rnd() % 5); o.h = 0; break;
          case 4: o.w = 11; o.h = 6; o.by = (uint8_t)h; break;      // flies just over a standing head
          case 5: o.w = 4; o.h = 13; break;
          default: {   // the power-up: on the ground (run into it), a jump up, or high up (a well-timed jump, or a double)
            static const uint8_t TIER[6] = {0, 1, 1, 2, 2, 1};
            const int tier = TIER[pet.rnd() % 6], hi = h + 7 < 21 ? h + 7 : 21;
            o.w = 5; o.h = 5;
            o.by = (uint8_t)(tier == 0 ? 1 : tier == 1 ? h + 1 + (int)(pet.rnd() % 3) : hi - (int)(pet.rnd() % 2));
            o.x = (float)(MW + (int)(pet.rnd() % 13));   // and a random way further out
            break;
          }
        }
        // always room to land (a double jump takes longer) and see what comes next; later runs leave less spare room
        const float diff = r.t > 40 ? 1.0f : r.t / 40.0f;
        const float recover = kind == 5 ? 1.6f : 1.05f;
        r.next = speed * recover * (1.0f + pet.rndf() * 0.7f * (1.0f - 0.55f * diff)) + w + 10;
        break;
      }
    }
    for (int i = 0; i < 6; i++) {
      auto& o = r.ob[i];
      if (!o.on) continue;
      o.x -= speed * dtw;
      if (o.x + o.w < 0) { o.on = false; continue; }
      const bool over = o.x < RUN_FX + w - 3 && o.x + o.w > RUN_FX + 2;
      bool hit = false;
      if (o.kind == 5) {   // a tall wall cleared with a double jump
        if (over && r.air2) o.dj = 1;
        if (!over && o.dj && o.x + o.w <= RUN_FX + 2) r.wallDone = 1;
      }
      if (o.kind == 6) {
        if (over && r.y + h - 1 >= o.by && r.y < o.by + o.h) { o.on = false; r.boost = RUN_BOOST_SECS; if (!sim) sound(SND_WHOOSH); }
        continue;
      }
      if (o.kind == 3) {
        const float mx = RUN_FX + w * 0.5f;
        if (r.y <= 0.01f && mx > o.x + 1 && mx < o.x + o.w - 1) { hit = true; r.cause = 1; }
      } else if (over && r.boost <= 0) {
        hit = o.kind == 4 ? (r.y + h - 1 > o.by) : (r.y < o.h - 1);
        r.cause = 0;
      }
      if (hit) {
        r.phase = 2; r.score = r.dist / 4.0f; r.t = 0; r.hitX = (int)o.x; if (r.cause == 0) o.on = false; else r.vy = 0;
        if (!sim) {
          sound(SND_GRUMBLE);
          const int hs = runHearts(r.score, r.wallDone);
          pet.exercise(4.0f + 6.0f * hs, 0.4f + 0.3f * hs);
          if (pet.d.evolving) pet.train(ACC_SPEED, 0.15f * hs);
          sv.lifeGames++;
          if (r.score > runBest_) runBest_ = r.score;
        }
        break;
      }
    }
  } else if (r.phase == 2) {
    if (r.cause == 1) { r.vy -= RUN_GRAV * dtReal; r.y += r.vy * dtReal; }   // down the hole
  }
}

void Game::screenRun() {
  const int sp = gameSprite();
  const int w = spriteW(sp), h = spriteH(sp), cy = FLOOR - h, fx = RUN_FX;
  Run& r = rn_;
  int ic;
  if (r.phase < 2 && iconTapped(ic) && ic >= 4) { doIcon(ic); return; }
  char b[16];
  if (r.phase == 0) { memset(&r, 0, sizeof r); r.next = 40; r.phase = 1; r.card = RUN_CARD_SECS; }
  if (r.card > 0) {   // the card: how to play, before anything comes at them (a tap starts the run, or it starts by itself)
    m_.smallCenter(1, "TAP = JUMP");
    for (int x = 2; x < MW - 2; x += 2) m_.set(x, 8, SH_MID);
    m_.smallCenter(11, "TAP IN AIR");
    m_.smallCenter(17, "= DOUBLE");
    m_.smallCenter(23, "JUMP");
    r.card -= dt;
    if (in.tap && r.card < RUN_CARD_SECS - 0.3f) { r.card = 0; sound(SND_TAP); }
    return;
  }
  runStep(dt, in.pressed, false);
  const bool boosted = r.boost > 0;
  // the ground, scrolling, with holes cut out of it
  const int off = (int)r.dist;
  for (int x = 0; x < MW; x++) {
    bool gap = false;
    for (int i = 0; i < 6; i++) if (r.ob[i].on && r.ob[i].kind == 3 && x >= (int)r.ob[i].x && x < (int)r.ob[i].x + r.ob[i].w) gap = true;
    if (!gap && ((x + off) % 7) < 5) m_.set(x, FLOOR, SH_MID);
  }
  if (r.phase == 1) {
    const bool blink = boosted && ((int)(r.t * 14) & 1);
    if (!blink) drawGameBlob(fx, cy - (int)lroundf(r.y), false, 0, r.y <= 0 && ((int)(r.t * 10) & 1));
    if (boosted) for (int k = 0; k < 3; k++) { const int ly = cy + 2 + k * 3, lx = fx - 2 - (int)(r.dist * 2 + k * 5) % 5; m_.set(lx, ly - (int)lroundf(r.y), SH_MID); m_.set(lx - 1, ly - (int)lroundf(r.y), SH_MID); }
    for (int i = 0; i < 6; i++) {
      const auto& o = r.ob[i];
      if (!o.on) continue;
      const int ox = (int)o.x, oy = FLOOR - o.h;
      switch (o.kind) {
        case 3:   // the edges of a hole
          m_.set(ox, FLOOR - 1, SH_INK); m_.set(ox, FLOOR, SH_INK); m_.set(ox + o.w - 1, FLOOR - 1, SH_INK); m_.set(ox + o.w - 1, FLOOR, SH_INK);
          break;
        case 4: {   // a bird with both wings spread (beak to the left); it holds still, no flapping
          static const char* BIRD[6] = {"XX.....XX..", ".XXX..XXX..", "..XXXXXXXXX", "MXXXXXXXXX.", "..XXXXXX...", "....XX....."};
          const int by = FLOOR - o.by - 6;
          for (int yy = 0; yy < 6; yy++) for (int xx = 0; xx < 11; xx++) if (BIRD[yy][xx] != '.') m_.set(ox + xx, by + yy, BIRD[yy][xx] == 'M' ? SH_MID : SH_INK);
          break;
        }
        case 6: {   // power-up bolt
          const int by = FLOOR - o.by - 5;
          static const uint8_t bolt[5][3] = {{0, 0, 1}, {0, 1, 1}, {1, 1, 1}, {0, 1, 0}, {1, 0, 0}};
          for (int yy = 0; yy < 5; yy++) for (int xx = 0; xx < 3; xx++) if (bolt[yy][xx]) m_.set(ox + 1 + xx, by + yy, ((int)(r.t * 6) & 1) ? SH_INK : SH_MID);
          break;
        }
        default:
          m_.rect(ox, oy, o.w, o.h, SH_INK);
          if (o.kind == 1 && o.w > 2) m_.rect(ox + 1, oy + 1, o.w - 2, o.h - 2, SH_MID);   // a crate
          if (o.kind == 2) for (int k = 0; k < o.w; k += 2) m_.set(ox + k, oy, 0);         // a spiky mat
          if (o.kind == 5) for (int k = 2; k < o.h; k += 3) m_.set(ox + 1, oy + k, 0);      // a pillar
      }
    }
    snprintf(b, sizeof b, "%d", (int)(r.dist / 4.0f));
    m_.small(MW - Matrix::smallWidth(b), 0, b, SH_MID);
    if (boosted) { const int n = (int)(r.boost / RUN_BOOST_SECS * 14.0f + 0.9f); for (int k = 0; k < n; k++) m_.set(1 + k, 0, SH_INK); }
    return;
  }
  if (r.phase == 2) {
    if (r.cause == 1) drawGameBlob(fx, cy - (int)lroundf(r.y), false, 1, false);
    else {
      drawGameBlob(fx, cy - (int)lroundf(r.y), false, 1, false);
      m_.sprite(clampi(fx + w - 2, 0, MW - 3), clampi(cy - 3, 0, MH), SPR_ANGRY);
    }
    if (r.t > 1.0f) { r.phase = 3; r.t = 0; sound(runHearts(r.score, r.wallDone) >= 3 ? SND_WIN : SND_LOSE); }
    return;
  }
  m_.clear();
  snprintf(b, sizeof b, "%dM", (int)r.score);
  m_.textCenter(0, b);
  hearts(m_, 9, runHearts(r.score, r.wallDone) * 25.0f);
  m_.smallCenter(17, runHearts(r.score, r.wallDone) >= 4 ? "PERFECT!" : r.score >= 65 ? "GREAT" : r.score >= 35 ? "GOOD" : r.score >= 12 ? "OK" : "OOPS");
  snprintf(b, sizeof b, "BEST %d", (int)runBest_);
  m_.smallCenter(MH - 5, b, SH_MID);
  if (r.t > 0.6f && in.tap) setScreen(S_MAIN);
}

// ---------------------------------------------------------------------------------------------
// ORBIT CATCH (Big Blorbus's game, in the Feed slot): the fella, as a little round chomper, circles a sun on one of two
// orbits; tap to hop to the other one. Eat the planets, but not the spiky ones (3 tummy aches and it's over). 30 s.
// Spiky planets never sit in the same slot on both orbits and always leave time to hop, so nothing is unavoidable.
// (o.stars counts the planets eaten, o.hits the tummy aches.)
// ---------------------------------------------------------------------------------------------
static int orbitHearts(int stars) { return stars >= 26 ? 4 : stars >= 18 ? 3 : stars >= 11 ? 2 : stars >= 5 ? 1 : 0; }
static constexpr float ORB_SLOT = 0.5236f;   // 30 degrees
static constexpr int ORB_CX = 19, ORB_CY = 14;
static constexpr float ORB_RX[2] = {12.0f, 17.0f}, ORB_RY[2] = {6.0f, 10.0f};   // (room for the chomper at its fattest)

void Game::orbitStep(float dtReal, bool tap, bool sim) {
  Orbit& o = orb_;
  o.t += dtReal;
  const float diff = o.t > 25 ? 1.0f : o.t / 25.0f;
  const float w = 1.5f + 1.1f * diff;
  o.th += w * dtReal;
  if (tap) { o.lane ^= 1; if (!sim) sound(SND_BOUNCE); }
  o.lanePos += ((float)o.lane - o.lanePos) * (dtReal * 16.0f > 1 ? 1 : dtReal * 16.0f);
  if (o.flash > 0) o.flash -= dtReal;
  if (o.gulp > 0) o.gulp -= dtReal;
  // a new slot comes round: set up the one half a lap ahead
  const int slot = (int)(o.th / ORB_SLOT);
  while (!sim && o.slot < slot) {
    o.slot++;
    const int target = o.slot + 6;
    const int gap = (int)ceilf(0.65f * w / ORB_SLOT);
    const int r = (int)(pet.rnd() % 100);
    int kind = -1;
    if (target - o.lastAst >= (gap < 2 ? 2 : gap) && r < 18 + (int)(17 * diff)) kind = 1;
    else if (r < 46) kind = 0;
    if (kind < 0) continue;
    for (int i = 0; i < 14; i++) if (!o.it[i].on) {
      o.it[i].on = true; o.it[i].kind = (uint8_t)kind; o.it[i].lane = (uint8_t)(pet.rnd() & 1); o.it[i].a = target * ORB_SLOT;
      if (kind == 1) o.lastAst = target;
      break;
    }
  }
  for (int i = 0; i < 14; i++) {
    auto& it = o.it[i];
    if (!it.on) continue;
    const float d = o.th - it.a;
    if (d > 0.36f) { it.on = false; continue; }           // sailed past
    if (it.lane == o.lane && d > -0.2f && d < 0.2f) {
      it.on = false;
      if (it.kind == 0) { o.stars++; o.gulp = 0.3f; if (!sim) sound(SND_EAT); }   // chomp
      else { o.hits++; o.flash = 0.6f; if (!sim) sound(SND_GRUMBLE); }               // a spiky one: tummy ache
      o.gx = ORB_CX + cosf(it.a) * ORB_RX[it.lane]; o.gy = ORB_CY + sinf(it.a) * ORB_RY[it.lane]; o.gk = it.kind;
    }
  }
  if (o.phase == 1 && (o.t >= 30.0f || o.hits >= 3)) {
    o.phase = 3; o.t = 0;
    if (!sim) {
      const int hs = orbitHearts(o.stars);
      pet.exercise(5.0f + 5.0f * hs, 0.2f * hs);
      sv.lifeGames++;
      if (o.stars > orbitBest_) orbitBest_ = o.stars;
      sound(hs >= 3 ? SND_WIN : SND_LOSE);
    }
  }
}

void Game::screenOrbit() {
  Orbit& o = orb_;
  int ic;
  if (o.phase < 3 && iconTapped(ic) && ic >= 4) { doIcon(ic); return; }
  char b[16];
  if (o.phase == 0) { memset(&o, 0, sizeof o); o.phase = 1; o.lastAst = -9; o.slot = -1; o.th = -1.0f; }
  if (o.phase == 3) o.t += dt;   // the result screen's own clock (it used to stand still, so a tap couldn't leave it)
  // a little starfield behind
  for (int i = 0; i < 14; i++) {
    const int x = (i * 17 + 5) % MW, y = (i * 11 + 3) % MH;
    if (((int)(scrT_ * 1.5f) + i) % 5) continue;
    m_.set(x, y, SH_MID);
  }
  if (o.phase == 1) {
    // first the sky is empty for a moment so the first star isn't a surprise
    orbitStep(dt, in.pressed, false);
  }
  if (o.phase == 1 || o.phase == 3) {
    // the two orbits, dotted
    for (int l = 0; l < 2; l++) for (int k = 0; k < 30; k++) {
      const float a = k * 6.2832f / 30;
      m_.set(ORB_CX + (int)lroundf(cosf(a) * ORB_RX[l]), ORB_CY + (int)lroundf(sinf(a) * ORB_RY[l]), SH_MID);
    }
  }
  // the sun in the middle: a hollow ring with rays that twinkle (nothing like the chomper, which is filled in)
  for (int dy = -2; dy <= 2; dy++) for (int dx = -2; dx <= 2; dx++) {
    const int d2 = dx * dx + dy * dy;
    if (d2 >= 3 && d2 <= 5) m_.set(ORB_CX + dx, ORB_CY + dy, SH_INK);
  }
  {
    const bool diag = ((int)(scrT_ * 3)) & 1;
    static const int8_t RAY[2][4][2] = {{{4, 0}, {-4, 0}, {0, 4}, {0, -4}}, {{3, 3}, {-3, 3}, {3, -3}, {-3, -3}}};
    for (const auto& r : RAY[diag]) m_.set(ORB_CX + r[0], ORB_CY + r[1], SH_INK);
  }
  if (o.phase == 1) {
    // the planets: tasty ones (ringed, or with a moon) and spiky ones
    for (int i = 0; i < 14; i++) {
      const auto& it = o.it[i];
      if (!it.on) continue;
      const int x = ORB_CX + (int)lroundf(cosf(it.a) * ORB_RX[it.lane]), y = ORB_CY + (int)lroundf(sinf(it.a) * ORB_RY[it.lane]);
      if (it.kind == 0) {
        m_.set(x, y, SH_MID); m_.set(x - 1, y, SH_INK); m_.set(x + 1, y, SH_INK); m_.set(x, y - 1, SH_INK); m_.set(x, y + 1, SH_INK);
        if (i & 1) { m_.set(x - 2, y, SH_INK); m_.set(x + 2, y, SH_INK); }   // a ring round it
        else m_.set(x + 2, y - 2, SH_MID);                                   // a little moon
      } else {   // spiky: don't eat
        static const int8_t SPK[12][2] = {{-2, -2}, {0, -2}, {2, -2}, {-1, -1}, {1, -1}, {-2, 0}, {2, 0}, {-1, 1}, {1, 1}, {-2, 2}, {0, 2}, {2, 2}};
        for (const auto& s : SPK) m_.set(x + s[0], y + s[1], SH_INK);
        m_.set(x, y - 1, SH_INK); m_.set(x - 1, y, SH_INK); m_.set(x + 1, y, SH_INK); m_.set(x, y + 1, SH_INK);
      }
    }
    // crumbs where the last one was eaten
    if (o.gulp > 0) {
      const float k = 1 - o.gulp / 0.3f;
      static const int8_t CR[4][2] = {{1, -1}, {-1, -1}, {1, 1}, {-1, 1}};
      for (const auto& c : CR) m_.set((int)lroundf(o.gx + c[0] * (1 + k * 3)), (int)lroundf(o.gy + c[1] * (1 + k * 3)), SH_MID);
    }
    // the chomper: round, facing the way it's going, mouth opening as a tasty planet comes up, fatter with every one eaten
    const float rx = ORB_RX[0] + (ORB_RX[1] - ORB_RX[0]) * o.lanePos, ry = ORB_RY[0] + (ORB_RY[1] - ORB_RY[0]) * o.lanePos;
    const float cx = ORB_CX + cosf(o.th) * rx, cy = ORB_CY + sinf(o.th) * ry;
    const float face = atan2f(cosf(o.th) * ry, -sinf(o.th) * rx);   // along the orbit
    float near = 9;
    for (int i = 0; i < 14; i++) if (o.it[i].on && o.it[i].kind == 0 && o.it[i].lane == o.lane) { const float d = o.it[i].a - o.th; if (d > -0.2f && d < near) near = d; }
    float mouth = near < 0.7f ? 0.9f * (1 - near / 0.7f) + 0.25f : 0.25f + 0.2f * (((int)(o.t * 6)) & 1);   // wide open as one comes up, munching otherwise
    if (o.gulp > 0.15f) mouth = 0;                                                                         // gulp
    const float r = 2.1f + (o.stars > 24 ? 24 : o.stars) * 0.035f + (o.gulp > 0 ? 0.35f * o.gulp / 0.3f : 0);   // 2.1 to about 3
    const bool sick = o.flash > 0, show = !(sick && ((int)(o.t * 16) & 1));
    if (show) {
      const int R = (int)ceilf(r + 1);
      for (int dy = -R; dy <= R; dy++) for (int dx = -R; dx <= R; dx++) {
        const float fx = (float)(int)lroundf(cx) + dx - cx, fy = (float)(int)lroundf(cy) + dy - cy, d2 = fx * fx + fy * fy;
        if (d2 >= (r + 0.5f) * (r + 0.5f)) continue;
        float rel = atan2f(fy, fx) - face;
        while (rel > 3.14159f) rel -= 6.28318f;
        while (rel < -3.14159f) rel += 6.28318f;
        if (mouth > 0 && fabsf(rel) < mouth && d2 > 0.6f) continue;                 // the open mouth
        m_.set((int)lroundf(cx) + dx, (int)lroundf(cy) + dy, d2 >= (r - 0.5f) * (r - 0.5f) ? SH_INK : sick ? SH_MID : 0);   // (green-grey inside with a tummy ache)
      }
      // the eye: above the middle, a little forward; a tummy ache screws it shut
      // (placed on the side away from the mouth's top jaw so the open mouth never swallows it)
      const float ux = cosf(face), uy = sinf(face);
      const float ex0 = cx + ux * r * 0.15f + uy * r * 0.55f, ey0 = cy + uy * r * 0.15f - ux * r * 0.55f;   // a quarter turn back from the mouth
      const int ex = (int)lroundf(ex0), ey = (int)lroundf(ey0);
      m_.set(ex, ey, SH_INK);
      if (sick) { m_.set(ex - 1, ey - 1, SH_INK); m_.set(ex + 1, ey + 1, SH_INK); m_.set(ex + 1, ey - 1, SH_INK); m_.set(ex - 1, ey + 1, SH_INK); }   // x_x
      // a hint of who they are
      const int ix = (int)lroundf(cx), iy = (int)lroundf(cy), ri = (int)lroundf(r);
      switch (form()) {
        case F_BB_BALLBOI: for (int k = 0; k < 16; k += 2) { const float a = k * 0.3927f + o.t; m_.set((int)lroundf(cx + cosf(a) * (r + 1.6f)), (int)lroundf(cy + sinf(a) * (r + 1.6f)), SH_MID); } break;   // his hamster ball
        case F_BB_ALIEN: m_.set(ix - ri + 1, iy - ri - 1, SH_INK); m_.set(ix + ri - 1, iy - ri - 1, SH_INK); break;   // ears
        case F_BB_BLOBBERT: m_.set(ix - 1, iy + ri + 1 + (((int)(o.t * 3)) & 1), SH_MID); break;                       // a drip
        default: break;
      }
    }
    for (int k = 0; k < 3 - o.hits; k++) m_.rect(1 + k * 3, 1, 2, 2, SH_INK);   // lives
    snprintf(b, sizeof b, "%d", o.stars);
    m_.small(MW - Matrix::smallWidth(b), 0, b, SH_MID);
    return;
  }
  m_.clear();
  snprintf(b, sizeof b, "%d", o.stars);
  m_.textCenter(0, b);
  hearts(m_, 9, orbitHearts(o.stars) * 25.0f);
  m_.smallCenter(17, o.stars >= 26 ? "PERFECT!" : o.stars >= 18 ? "GREAT" : o.stars >= 11 ? "GOOD" : o.stars >= 5 ? "OK" : "OOPS");
  const bool snack = orbitHearts(o.stars) >= 3 && eatRoutine() != 0;   // a good round earns a snack
  snprintf(b, sizeof b, "BEST %d", orbitBest_);
  m_.smallCenter(MH - 5, snack && ((int)(o.t * 1.5f) & 1) ? "SNACK!" : b, SH_MID);
  if (o.t > 0.6f && in.tap) {
    if (snack) startAnim(AN_EAT_SNACK, eatSecs());
    else setScreen(S_MAIN);
  }
}

// ---------------------------------------------------------------------------------------------
// evolution: at each new stage the player flips through the looks and taps the one they want
// ---------------------------------------------------------------------------------------------
void Game::screenEvolve() {
  const int stage = pendingEvolution();
  if (!stage) { setScreen(S_MAIN); return; }
  const int n = evoCount(stage);
  if (evoSel_ < 0 || evoSel_ >= n) evoSel_ = 0;
  const EvoChoice& opt = evoOption(stage, evoSel_);
  int idle, alt, sl;
  blobSprite(opt.form == F_COUNT ? lilFellaForm() : opt.form, idle, alt, sl);   // (F_COUNT: turning Big Blorbus down)
  const int w = spriteW(idle), h = spriteH(idle);
  m_.smallCenter(2, opt.name);   // ("Pick new form:" is written above the LCD, where the top icons usually are)
  const bool bob = ((int)(scrT_ * 2)) & 1;
  m_.sprite((MW - w) / 2, FLOOR - h - (bob ? 1 : 0), idle);
  m_.sprite(0, 12, SPR_ARROW_R, true);
  m_.sprite(MW - 3, 12, SPR_ARROW_R);
  int dx, dy;
  if (scrT_ < 0.5f) return;   // a moment to see it before a tap counts
  if (!in.tap || !inMatrix(in.px, in.py, dx, dy)) return;
  if (dx < 12) { evoSel_ = (evoSel_ + n - 1) % n; sound(SND_PAGE); return; }   // left / right thirds flip, middle picks
  if (dx >= MW - 12) { evoSel_ = (evoSel_ + 1) % n; sound(SND_PAGE); return; }
  // pick this one
  evoSetChoice(stage, stage == 4 ? (evoSel_ == 0 ? 1 : 7) : evoSel_ + 1);
  lastForm_ = form();
  { int i2, a2, s2; blobSprite(form(), i2, a2, s2); blobX_ = blobTx_ = (float)((MW - spriteW(i2)) / 2); }
  sound(SND_WIN);
  autosave(true);
  startAnim(AN_GROW, 2.0f);
}

// ---------------------------------------------------------------------------------------------
// endings
// ---------------------------------------------------------------------------------------------
void Game::screenEnding() {
  // Every life ends the same way: they melt into a puddle of goo and the dropper comes down over it. Then it's up to
  // you: squeeze the bulb and let go, three times, to suck the puddle up. The dropper rises and the name page opens;
  // "Let's goo!" leads to the opening scene, where one tap squeezes out the new blob.
  int idle, alt, sl;
  blobSprite(form(), idle, alt, sl);
  const int w = spriteW(idle), h = spriteH(idle), cy = FLOOR - h;
  const float t = scrT_;
  const bool recycled = anArg_ == END_RECYCLED;
  const int cx = (MW - w) / 2;
  const int dw = spriteW(SPR_DROPPER), dh = spriteH(SPR_DROPPER), dx0 = (MW - dw) / 2, tipX = dx0 + dw / 2;
  const int lowY = FLOOR - 1 - dh;                                         // dropper's tip just above the puddle
  constexpr float T_MELT = 0.8f, T_POOL = 3.2f, T_SUCK = 2.0f, T_UP = ENDING_UP_AT;   // T_SUCK / T_UP: after the last squeeze
  const bool waiting = t >= ENDING_WAIT_AT && suckAt_ < 0;                 // the dropper is down, waiting for you

  // your finger: press squeezes the bulb, letting go sucks some goo up
  int dx, dy;
  if (waiting) {
    if (in.pressed && inMatrix(in.px, in.py, dx, dy)) { squeezing_ = true; sound(SND_STRETCH); }
    if (squeezing_ && !in.down) {
      squeezing_ = false; endPulls_++; pullAt_ = t; sound(SND_POOF);
      if (endPulls_ >= ENDING_PULLS) { suckAt_ = t; sound(SND_BLOW); }   // the last of it goes up the tube
    }
  }
  if (t >= T_MELT && t - dt < T_MELT) sound(SND_MELT);                     // they start to melt
  const float s = suckAt_ < 0 ? -1 : t - suckAt_;                          // seconds since the last squeeze
  // The dropper is back up top with the goo inside: on to the name page. It waits for a finger still on the glass to
  // lift first, so a press that began here can't land on a key there.
  if (s >= T_UP && !in.down) { pickName(); setScreen(S_SETUP_NAME); return; }

  int pool = 0;                                                            // puddle half-width
  const int full = w / 2 + 4;
  if (t < T_POOL) {
    const float k = t < T_MELT ? 0 : (t - T_MELT) / (T_POOL - T_MELT);
    drawBlob(cx, cy, false, 1, false);
    m_.rect(0, 0, MW, cy + (int)(k * h), 0);                               // the top sinks away
    for (int i = 0; i < 3 && k > 0.1f; i++) {                              // drips down the sides
      const int dyy = cy + (int)(k * h) + (int)(fmodf(t * 6 + i * 2.3f, 6.0f));
      m_.set(i == 1 ? cx + w : cx - 1, clampi(dyy, 0, FLOOR), SH_MID);
    }
    if (recycled && k < 0.9f) for (int i = 0; i < 2; i++) {               // a recycled fella goes out sparkling
      const int sy = cy + (int)(k * h) - 3 - (int)(fmodf(t * 5 + i * 2.5f, 5.0f));
      if (sy > 5) m_.sprite(i ? cx + w - 2 : cx - 2, sy, SPR_SPARK);
    }
    if (anArg_ == END_SICK && t < T_MELT && ((int)(t * 4) & 1)) m_.sprite(clampi(cx + w, 0, MW - 7), clampi(cy - 2, 0, MH), SPR_SKULL);
    pool = (int)(k * full);
    const char* why = anArg_ == END_DISSOLVED ? "OVERFED" : anArg_ == END_SICK ? "TOO SICK" : anArg_ == END_SAD ? "TOO SAD"
                    : anArg_ == END_RAN_AWAY ? "NO TRUST" : "THANK YOU";
    m_.smallCenter(0, t < 1.6f ? (recycled ? "BYE BYE" : "OH NO") : why);
  } else if (s < 0) {                                                      // each squeeze takes a third of it
    float rem = 1.0f;
    if (endPulls_ > 0) rem = 1.0f - ((endPulls_ - 1) + (t - pullAt_ >= 0.5f ? 1.0f : (t - pullAt_) / 0.5f)) / (float)ENDING_PULLS;
    pool = (int)(full * rem + 0.5f);
  } else if (s < T_SUCK) {
    pool = (int)(full * (1.0f / ENDING_PULLS) * (1 - s / T_SUCK));         // the last third, shrinking as it's sucked up
  }
  if (pool > 0) {                                                          // the puddle of goo
    m_.rect(MW / 2 - pool, FLOOR, pool * 2, 1, SH_MID);
    if (pool > 2) m_.rect(MW / 2 - pool + 2, FLOOR - 1, pool * 2 - 4, 1, SH_MID);
  }
  if (t >= T_POOL) {
    int y = lowY, x = dx0, spr = SPR_DROPPER;
    if (t < ENDING_WAIT_AT) y = -dh + (int)((t - T_POOL) / (ENDING_WAIT_AT - T_POOL) * (lowY + dh));   // coming down
    else if (s < 0) {                                                      // waiting for your squeezes
      if (squeezing_) spr = SPR_DROPPER_SQ;
      for (int i = 0; i < endPulls_; i++) m_.set(tipX, y + dh - 2 - i, SH_MID);   // goo already in the glass
      if (fmodf(t, 1.0f) < 0.7f) m_.small(0, 3, "PUSH");                    // blinks beside the bulb
      for (int i = 0; i < ENDING_PULLS; i++) m_.rect(i * 3, 9, 2, 2, i < endPulls_ ? SH_INK : SH_MID);   // squeezes so far
    } else if (s < T_SUCK) {                                               // the goo climbs up the glass
      const int up = ENDING_PULLS - 1 + (int)(s / T_SUCK * 3);
      for (int i = 0; i < up; i++) m_.set(tipX, y + dh - 2 - i, SH_MID);
    } else if (s < T_UP) y = lowY - (int)((s - T_SUCK) / (T_UP - T_SUCK) * (lowY - 2));
    else y = 2;                                                            // up top (a finger is still down: see above)
    if (s >= T_SUCK) for (int i = 0; i < 4; i++) m_.set(tipX, y + dh - 2 - i, SH_MID);   // full of goo
    m_.sprite(x, y, spr);
  }
}

// ---------------------------------------------------------------------------------------------
// tests / web page shortcuts
// ---------------------------------------------------------------------------------------------
void Game::debugAct(int which) {
  if (!sv.hasBlob) return;
  if (scr == S_ANIM && an_ == AN_MEDS && anT_ < anLen_) return;   // (not even the web page's buttons cut the medicine short)
  switch (which) {
    case 0: case 1: { bool o; pet.d.hunger = 30; pet.feed(which ? FOOD_TREAT : FOOD_KIBBLE, o); startAnim(which ? AN_EAT_SNACK : AN_EAT_MEAL, eatSecs()); break; }
    case 2: startScrub(); break;
    case 13: startPop(); break;
    case 14: startYeet(); break;
    case 3: if (an_ != AN_MEDS) { pet.vetVisit(); startAnim(AN_MEDS, MEDS_SECS, pet.sickStack()); } break;
    case 5: memset(&dg_, 0, sizeof dg_); setScreen(S_DODGE); break;
    case 11: memset(&rn_, 0, sizeof rn_); setScreen(S_RUN); break;
    case 12: memset(&orb_, 0, sizeof orb_); setScreen(S_ORBIT); break;
    case 6: meterPage_ = 0; setScreen(S_METER); break;
    case 7: setFrom_ = S_MAIN; setPage_ = 0; setScreen(S_SETTINGS); break;
    case 8: if (pet.d.age < DROP_AGE) squeezeOut(); break;   // squeeze the dropper
    case -1: setScreen(S_MAIN); break;
    default: break;
  }
}

}  // namespace lf
