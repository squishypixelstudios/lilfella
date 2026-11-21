#include "game.h"
#include "breeds.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

namespace lf {

// Packed save fields (the SaveData layout never changes, so an existing fella carries over):
//   sv.eq[0]          screen palette: 0 classic LCD, 1 night (inverted)
//   sv.eq[1]          touch debug overlay on/off
//   sv.eq[2]          auto off when untouched: 0 after 5 s, 2 after 15 s, 3 after 30 s (default), 4 after 1 min, 1 never
//   pet.d.pad0[0]     the looks the player picked: bits0-1 Lil Blub, bits2-4 Lil Blunkus, bits5-7 Lil Fella (0 = not picked, else
//                     index+1 in the list the path so far allows, see the evolution tree below)
//   pet.d.pad0[1]     LOCK (1 = locked)
//   pet.d.pad1[0]     care mistakes
//   pet.d.pad1[1]     bit0 light off, bit1 looks packed in the evolution-tree layout, bits2-4 the Big Blorbus pick
//   pet.d.pad2        first-hour lessons (see game.h)

static const char* const kNames[] = {   // a new blob's random name (11 characters at most)
  "Pickles", "Pumpkin", "Muffin", "Tofu", "Ziggy", "Olive", "Bean", "Mochi",
  "Nugget", "Sprout", "Clover", "Peanut", "Misty", "Gizmo", "Luna", "Squeegee",
  "Wunkus", "Fluffy", "Pip", "Bleeb", "Blorp", "Blippy", "Squishface", "Mushy",
  "Puddles", "Blotto", "Yurp", "Manny", "Plinky", "Blinky", "Spraynard"};
constexpr int kNameCount = sizeof(kNames) / sizeof(kNames[0]);

// ---------------------------------------------------------------------------------------------
// The evolution tree. Each stage's list depends on what was picked before.
//   Lil Blub (3)  ->  Lil Blunkus  ->  Lil Fella
// ---------------------------------------------------------------------------------------------
#define EC(f, n) {f, n}
static const EvoChoice EVO_CHILD[3] = {EC(F_S0, "BLOBBERT"), EC(F_S0B, "MOCHI"), EC(F_S0C, "CHEEKS")};
static const EvoChoice TEEN_BLOBBERT[4] = {EC(F_TN_SPROUT, "SPROUT"), EC(F_TN_BUN, "BUN"), EC(F_TN_CURL, "CURLY"), EC(F_TN_SLOSH, "SLOSH")};
static const EvoChoice TEEN_MOCHI[3] = {EC(F_TN_WOBBLES, "WOBBLES"), EC(F_TN_MALLOW, "MALLOW"), EC(F_TN_TOASTY, "TOASTY")};
static const EvoChoice TEEN_CHEEKS[3] = {EC(F_TN_BEAN, "BEAN"), EC(F_TN_BUN, "BUN"), EC(F_TN_SLOSH, "SLOSH")};
// final forms, by the Lil Blunkus (and for Bun and Slosh, which Lil Blub it came from)
static const EvoChoice FIN_SPROUT[3] = {EC(F_AD_JELLY, "JELLY"), EC(F_AD_DANGO, "SPIKYBOI"), EC(F_AD_MARSHY, "MARSHY")};
static const EvoChoice FIN_BUN_B[3] = {EC(F_AD_PUDDLE, "PUDDLE"), EC(F_AD_JELLY, "JELLY"), EC(F_AD_PUFF, "PUFF")};
static const EvoChoice FIN_CURLY[3] = {EC(F_AD_FUBZ, "FUBZ"), EC(F_AD_BOOP, "BOOP"), EC(F_AD_DANGO, "SPIKYBOI")};
static const EvoChoice FIN_SLOSH_B[3] = {EC(F_AD_NIMBUS, "NIMBUS"), EC(F_AD_BOOP, "BOOP"), EC(F_AD_BALLBOI, "BALLBOI")};
static const EvoChoice FIN_WOBBLES[3] = {EC(F_AD_JELLY, "JELLY"), EC(F_AD_PUDDLE, "PUDDLE"), EC(F_AD_NIMBUS, "NIMBUS")};
static const EvoChoice FIN_MALLOW[3] = {EC(F_AD_LOAF, "LOAF"), EC(F_AD_PUFF, "PUFF"), EC(F_AD_BOOP, "BOOP")};
static const EvoChoice FIN_TOASTY[2] = {EC(F_AD_LOAF, "LOAF"), EC(F_AD_MARSHY, "MARSHY")};
static const EvoChoice FIN_BEAN[4] = {EC(F_S1C, "DAPPER"), EC(F_AD_DANGO, "SPIKYBOI"), EC(F_AD_MARSHY, "MARSHY"), EC(F_AD_FUBZ, "FUBZ")};
static const EvoChoice FIN_BUN_C[3] = {EC(F_S1C, "DAPPER"), EC(F_AD_PUFF, "PUFF"), EC(F_AD_FUBZ, "FUBZ")};
static const EvoChoice FIN_SLOSH_C[5] = {EC(F_AD_NIMBUS, "NIMBUS"), EC(F_AD_BOOP, "BOOP"), EC(F_AD_BALLBOI, "BALLBOI"), EC(F_S1C, "DAPPER"), EC(F_AD_FUBZ, "FUBZ")};
#undef EC

struct EvoList { const EvoChoice* o; int n; };
#define LIST(a) EvoList{a, (int)(sizeof(a) / sizeof(a[0]))}
static EvoList teenList(int childPick) {   // childPick = Lil Blub index + 1 (0 = unknown: treated as the first)
  return childPick == 2 ? LIST(TEEN_MOCHI) : childPick == 3 ? LIST(TEEN_CHEEKS) : LIST(TEEN_BLOBBERT);
}
static EvoList finalList(int childPick, Form teen) {
  const bool cheeks = childPick == 3;
  switch (teen) {
    case F_TN_SPROUT: return LIST(FIN_SPROUT);
    case F_TN_BUN: return cheeks ? LIST(FIN_BUN_C) : LIST(FIN_BUN_B);
    case F_TN_CURL: return LIST(FIN_CURLY);
    case F_TN_SLOSH: return cheeks ? LIST(FIN_SLOSH_C) : LIST(FIN_SLOSH_B);
    case F_TN_WOBBLES: return LIST(FIN_WOBBLES);
    case F_TN_MALLOW: return LIST(FIN_MALLOW);
    case F_TN_TOASTY: return LIST(FIN_TOASTY);
    case F_TN_BEAN: return LIST(FIN_BEAN);
    default: return LIST(FIN_SPROUT);
  }
}
static EvoList evoListFor(int stage, int c, int tp) {   // c, tp: the Lil Blub / Lil Blunkus picks so far (index + 1)
  if (stage == 1) return LIST(EVO_CHILD);
  const EvoList t = teenList(c);
  if (stage == 2) return t;
  return finalList(c, t.o[(tp >= 1 && tp <= t.n) ? tp - 1 : 0].form);
}
static const EvoChoice BIG_PICK[3] = {{F_BB_BLOBBERT, "BIG BLOB"}, {F_BB_BALLBOI, "BIG BALL"}, {F_BB_ALIEN, "ALIEN CAT"}};
static const EvoChoice BIG_STAY = {F_COUNT, "STAY"};   // (turn Big Blorbus down: the form stays what it is)
Form Game::bigOf(Form f, int child) {   // child: the Lil Blub pick (index + 1); Big Blobbert is for the Blobbert line, Big Ballboi for the Cheeks line
  switch (f) {
    case F_AD_JELLY: case F_AD_PUFF: return child == 1 ? F_BB_BLOBBERT : F_COUNT;   // Big Blobbert: the Blobbert line only
    case F_AD_BALLBOI: return child == 3 ? F_BB_BALLBOI : F_COUNT;
    case F_AD_NIMBUS: case F_S1C: return F_BB_ALIEN;   // Nimbus or Dapper (Loaf could until v1.0.2)
    default: return F_COUNT;
  }
}
int Game::evoCount(int stage) const { if (stage == 4) return 2; return evoListFor(stage, evoChoice(1), evoChoice(2)).n; }
const EvoChoice& Game::evoOption(int stage, int i) const {
  if (stage == 4) { if (i == 1) return BIG_STAY; const Form b = bigOf(lilFellaForm(), evoChoice(1)); return BIG_PICK[b == F_BB_BALLBOI ? 1 : b == F_BB_ALIEN ? 2 : 0]; }
  const EvoList l = evoListFor(stage, evoChoice(1), evoChoice(2)); return l.o[i < 0 || i >= l.n ? 0 : i]; }

int stageOfForm(Form f) {
  switch (f) {
    case F_DROPPER: case F_BABY: return 0;
    case F_S0: case F_S0B: case F_S0C: return 1;
    case F_TN_BEAN: case F_TN_BUN: case F_TN_CURL: case F_TN_SPROUT: case F_TN_SLOSH: case F_TN_WOBBLES:
    case F_TN_MALLOW: case F_TN_TOASTY: return 2;
    case F_BB_BLOBBERT: case F_BB_BALLBOI: case F_BB_ALIEN: return 4;
    default: return 3;
  }
}

// Saves from before the tree packed the picks as bits0-1 child, bits2-3 teen, bits4-6 adult, indexes into the old lists.
// Re-pack them, keeping each look where the new tree still has it (otherwise the first option).
void Game::migrateLooks() {
  if (pet.d.pad1[1] & 2) return;
  pet.d.pad1[1] |= 2;
  const uint8_t old = pet.d.pad0[0];
  static const Form OT[3] = {F_TN_BEAN, F_TN_BUN, F_TN_CURL};
  static const Form OA[7] = {F_AD_PUDDLE, F_AD_JELLY, F_S1C, F_TN_SPROUT, F_AD_DANGO, F_AD_PUFF, F_AD_LOAF};
  const int oc = old & 3, ot = (old >> 2) & 3, oa = (old >> 4) & 7;
  int nt = 0, na = 0;
  if (ot >= 1 && ot <= 3) {
    const EvoList t = teenList(oc);
    nt = 1;
    for (int i = 0; i < t.n; i++) if (t.o[i].form == OT[ot - 1]) nt = i + 1;
    if (oa >= 1 && oa <= 7) {
      const EvoList f = finalList(oc, t.o[nt - 1].form);
      na = 1;
      for (int i = 0; i < f.n; i++) if (f.o[i].form == OA[oa - 1]) na = i + 1;
    }
  }
  pet.d.pad0[0] = packLooks(oc, nt, na);
}

constexpr float NEGLECT_LIMIT = 15 * 60.0f;   // a call left unanswered this long is a care mistake

Game::Game(Platform* p) : plat(p), pet(sv.pet) { saveDefaults(sv); m_.clear(); }

const char* Game::randomName() {
  nameSeed_ = (nameSeed_ + 1 + (int)(pet.rnd() % 7)) % kNameCount;
  return kNames[nameSeed_];
}
void Game::pickName() { memset(nameBuf_, 0, sizeof nameBuf_); strncpy(nameBuf_, randomName(), sizeof(nameBuf_) - 1); nameFresh_ = true; }

// ---------------------------------------------------------------------------------------------
// lifecycle
// ---------------------------------------------------------------------------------------------
void Game::begin() {
  static SaveData tmp;
  memset(&tmp, 0, sizeof(tmp));
  if (plat->loadSave(&tmp, sizeof(tmp)) && saveValid(tmp)) sv = tmp;
  else saveDefaults(sv);
  pet.rng = (plat->randomSeed() ^ 0xA5A5A5A5u) | 1u;
  nowMs = plat->millis();
  if (sv.hasBlob) migrateLooks();
  if (sv.volume > 5) sv.volume = 2;
  if (sv.brightness < 1 || sv.brightness > 5) sv.brightness = 4;
  if (!sv.fallbackClock) sv.fallbackClock = toEpoch({2026, 1, 1, 12, 0, 0});
  showTouch = sv.eq[1] != 0;
  updateClock();
  nameSeed_ = (int)(pet.rnd() % kNameCount);
  pickName();   // every new blob starts with a random name (NEW NAME for another)
  {
    DateTime d = fromEpoch(epoch);
    clockSel_[0] = d.year; clockSel_[1] = d.month; clockSel_[2] = d.day; clockSel_[3] = d.hour; clockSel_[4] = d.minute;
  }
  // one mode only (old saves made in the retired hard mode are switched over)
  const bool firstStart = sv.mode == MODE_UNSET;
  sv.mode = MODE_EASY;
  if (firstStart) { clockNext_ = S_SETUP_NAME; scr = S_SETUP_CLOCK; }
  else if (!sv.hasBlob) scr = S_SETUP_NAME;
  else {
    applyOfflineCatchUp();
    scr = S_MAIN;
    if (!clockOk) { clockNext_ = S_MAIN; scr = S_SETUP_CLOCK; }
  }
  // the Journal: a save from before it had one starts with the looks this fella has had, and the old friends' looks
  if (!sv.journal) {
    for (int i = 0; i < HALL_MAX && i < sv.hallCount; i++) if (sv.hall[i].look) markSeen((Form)(sv.hall[i].look - 1));
    if (sv.hasBlob) for (int s = 1; s <= 3; s++) { const int c = evoChoice(s); if (c && c <= evoCount(s)) markSeen(evoOption(s, c - 1).form); }
    if (sv.hasBlob) markSeen(form());
    sv.journal |= 1u << F_BABY;
  }
  // v0.07 upgrade: sickness is a thing they catch now (bytes once used by the bubbles currency are cleared)
  if (!(sv.fixes & FIX_SICK)) {
    sv.unused0 = 0;
    PetData& d = pet.d;
    memset(d.sick, 0, sizeof d.sick); d.nightSleep = 0; d.nightLight = 0; d.bubblesOnFloor = 0;
    if (d.age >= SICK_BUG_BY) d.sick[3] |= SF_BUG_DONE;
    if (sv.hasBlob && d.health < 35) d.sick[SICK_BUG] = 1;   // sick under the old rules: one dose sorts it
    sv.fixes |= FIX_SICK;
  }
  if (!(sv.fixes & FIX_LOCK)) { pet.d.pad0[1] = 0; sv.fixes |= FIX_LOCK; }   // (what was the training bar: start unlocked)
  // v1.0.1 upgrade: Auto off is a plain timer now, and its default is 30 Sec. A save still on the old default
  // (5 Sec, which only applied laid flat) moves to 30 Sec once; after that whatever the player picks is kept.
  if (!(sv.fixes & FIX_AUTOOFF)) { if (sv.eq[2] == 0) sv.eq[2] = 3; sv.fixes |= FIX_AUTOOFF; }
  lastForm_ = form();
  if (sv.hasBlob && form() == F_BABY && pet.d.age < DROP_AGE + SQUEEZE_SECS + NEWBORN_AWAKE)   // restarted in the first minutes
    newbornT_ = DROP_AGE + SQUEEZE_SECS + NEWBORN_AWAKE - pet.d.age;
  wake();
  updatePower();
}

void Game::applyOfflineCatchUp() {
  if (!sv.hasBlob || !sv.lastSeen || !clockOk || epoch <= sv.lastSeen) return;
  uint32_t el = epoch - sv.lastSeen;
  if (el < 120) return;
  if (el > 400u * 86400u) el = 400u * 86400u;
  const uint32_t cap = 10u * 3600u;
  uint32_t simSecs = el < cap ? el : cap;
  for (uint32_t t = 0; t < simSecs; t += 60) {
    SimCtx c;
    const DateTime when = fromEpoch(sv.lastSeen + t);
    c.night = isNight(when.hour, when.minute);
    c.lightOff = lightOff();
    c.away = true;
    pet.tick(60.0f, c);
    if (stageOfForm(form()) == 4) pet.d.hunger = 100;
    Event e;
    while (pet.pop(e)) if (e.type == EV_SICK && e.arg != SICK_BUG && sv.sickCount < 255) sv.sickCount++;
  }
  if (el > simSecs) pet.d.age += (float)(el - simSecs);
  pet.d.exploring = 0;
  pet.sleeping = false;
}

void Game::autosave(bool force) {
  if (!force && saveCool_ > 0) return;
  sv.lastSeen = epoch;
  saveSeal(sv);
  plat->storeSave(&sv, sizeof(sv));
  saveCool_ = 15;
  saveT_ = 0;
}

void Game::newPet() {
  pet.newPet(plat->randomSeed() ^ nowMs ^ epoch, 0, nameBuf_);
  pet.d.evolving = 1;   // the breed (weight gain, bubble rate, coat) is decided by how they're raised
  pet.d.pad2 = 0x8000;  // first-hour lessons on
  sv.mode = MODE_EASY;  // set up (after Erase everything the save is MODE_UNSET until the next blob)
  tutWant_ = -1; tutNext_ = DROP_AGE;   // (the first "HELLO!" waits for the drop to land, see lessonTick)
  pet.d.nextSick = 0; pet.d.sleepyMin = 0;   // scheduled on the first tick
  sv.sickCount = 0;
  sv.hasBlob = 1;
  pet.d.pad1[1] |= 2;   // looks use the evolution-tree packing
  sv.lifeBlobs++;
  blobX_ = blobTx_ = 14;
  neglectT_ = 0;
  memset(&sc_, 0, sizeof sc_);   // (no half-done scrub carried over from the last fella)
  lastForm_ = F_DROPPER;
  sv.lastSeen = epoch;
  autosave(true);
}

// The friends log: the last HALL_MAX fellas, newest first (only ones that made it out of the dropper).
void Game::logFriend(int ending) {
  if (!sv.hasBlob || pet.d.age < DROP_AGE) return;
  for (int i = HALL_MAX - 1; i > 0; i--) sv.hall[i] = sv.hall[i - 1];
  HallEntry& h = sv.hall[0];
  memset(&h, 0, sizeof(h));
  memcpy(h.name, pet.d.name, sizeof(h.name) - 1);   // the last byte stays 0 (memset)
  h.breed = pet.d.breed;
  h.ending = (uint8_t)ending;
  const float hours = pet.d.age / HOUR;
  h.age = (uint16_t)(hours > 65535 ? 65535 : hours);
  h.gift = pet.d.giftCount;
  h.look = (uint8_t)(form() + 1);
  h.flags = HE_HOURS;
  if (sv.hallCount < HALL_MAX) sv.hallCount++;
  sv.lifeGifts += pet.d.giftCount;
}

void Game::endLife(int ending) {
  logFriend(ending);
  sv.hasBlob = 0;
  pet.d.exploring = 0;
  anArg_ = ending; suckAt_ = -1; squeezing_ = false; endPulls_ = 0; pullAt_ = -1;
  sound(ending == END_RECYCLED ? SND_WIN : SND_SAD);   // the caption; the melt has its own sound (screenEnding)
  stayAwake();
  setScreen(S_ENDING);
  autosave(true);
}

void Game::say(const char* s, float secs) {
  strncpy(msg_, s, sizeof(msg_) - 1);
  msg_[sizeof(msg_) - 1] = 0;
  msgT_ = secs;
}

// ---------------------------------------------------------------------------------------------
// what they look like
// ---------------------------------------------------------------------------------------------
Form Game::form() const {
  const Form f = lilFellaForm();
  if (pet.d.age >= AGE_BLORBUS && evoChoice(4) == 1) {
    const Form b = bigOf(f, evoChoice(1));
    if (b != F_COUNT) return b;
    if (f == F_AD_LOAF) return F_BB_ALIEN;   // a Loaf that already took the Alien Cat (before Dapper replaced Loaf) stays one
  }
  return f;
}

Form Game::lilFellaForm() const {
  const float a = pet.d.age;
  if (a < DROP_AGE) return F_DROPPER;
  const int stage = a < BABY_END ? 0 : a < CHILD_END ? 1 : a < TEEN_END ? 2 : 3;
  // they keep their current look until the player picks the next one
  for (int s = stage; s >= 1; s--) {
    const int c = evoChoice(s);
    if (c && c <= evoCount(s)) return evoOption(s, c - 1).form;
  }
  return F_BABY;
}

// Developer option (Settings page 3): jump straight to one of the three Big Blorbus looks, wherever the fella is in life.
// Each tap goes on to the next: Big Blob, Big Ball, Alien Cat. It makes them 8 days old, skips the first-hour lessons and
// sets the looks picked on the way (whatever was picked before is replaced) so that look is the one they grow into.
void Game::debugJumpBig() {
  if (!sv.hasBlob || form() == F_DROPPER) return;
  // the picks (index + 1 at Lil Blub, Lil Blunkus, Lil Fella) that lead to each: Jelly > Big Blob, Ballboi > Big Ball, Nimbus > Alien Cat
  static const uint8_t PATH[3][3] = {{1, 2, 2}, {3, 3, 3}, {1, 4, 1}};
  const Form now = form();
  const int next = now == F_BB_BLOBBERT ? 1 : now == F_BB_BALLBOI ? 2 : 0;   // from anything else: start at Big Blob
  PetData& d = pet.d;
  for (int s = 1; s <= 3; s++) evoSetChoice(s, PATH[next][s - 1]);
  evoSetChoice(4, 1);
  if (d.age < AGE_BLORBUS) d.age = AGE_BLORBUS;
  d.pad2 |= 0x40FF;   // lessons finished
  tutWant_ = -1; newbornT_ = 0;
  say(lookName(form()), 3.0f);
  autosave(true);
}

// The first stage they've reached without the player having picked its look (0 = none).
int Game::pendingEvolution() const {
  const float a = pet.d.age;
  const float starts[4] = {0, BABY_END, CHILD_END, TEEN_END};
  for (int s = 1; s <= 3; s++) {
    if (locked()) return 0;                                    // LOCK: they stay as they are
    if (s == 1 && lessonsOn() && !lessonsDone()) return 0;   // a Lil Blob grows up only after trying every icon
    if (a >= starts[s] && !evoChoice(s)) return s;
  }
  // Big Blorbus: old enough, and a Lil Fella that has one to grow into
  if (!locked() && a >= AGE_BLORBUS && !evoChoice(4) && bigOf(lilFellaForm(), evoChoice(1)) != F_COUNT) return 4;
  return 0;
}

// Called every frame: the squeeze out of the dropper, the "grow" flash, and the level-up that opens the evolution picker.
void Game::checkGrowth() {
  if (!sv.hasBlob) return;
  const Form now = form();
  markSeen(now);   // the Journal
  if (lastForm_ != F_COUNT && now != lastForm_) {
    if (lastForm_ == F_DROPPER && now == F_BABY) { if (scr == S_MAIN) startAnim(AN_SQUEEZE, SQUEEZE_SECS); }
    else if (scr == S_MAIN || scr == S_ANIM) startAnim(AN_GROW, 2.0f);
    else sound(SND_LEVEL);   // (with an animation, the fanfare plays when its title appears)
    stayAwake();
  }
  lastForm_ = now;
  // A new stage waits until they're awake (and the light is on): growing up never happens in their sleep.
  if (pendingEvolution() && scr == S_MAIN && screenOn_ && !pet.sleeping && !lightOff() && !pet.d.exploring) {
    startAnim(AN_LEVELUP, LEVELUP_SECS);   // something's happening... (the picker opens when it ends, see screenAnim)
  }
}

// ---------------------------------------------------------------------------------------------
// attention + care rules
// ---------------------------------------------------------------------------------------------
CallReason Game::callReason() const {
  if (!sv.hasBlob || form() == F_DROPPER || pet.d.exploring) return CR_NONE;
  const PetData& d = pet.d;
  if (d.hunger < 15) return CR_HUNGRY;
  if (d.fun < 15 && !pet.sleeping) return CR_BORED;
  if (pet.sick()) return CR_SICK;
  if (d.floorMess || d.droppings >= 3) return CR_MESS;
  if (pet.sleeping && !lightOff()) return CR_LIGHT;   // asleep with the light on: they want it off
  if (d.resent >= 60) return CR_UPSET;
  // a first-hour lesson (real needs above come first, so a lesson never hides one)
  if (tutWant_ >= 0 && lessonsOn() && !(d.pad2 & (1u << tutWant_))) {
    static const CallReason lesson[IC_COUNT] = {CR_HUNGRY, CR_LIGHT, CR_BORED, CR_SICK, CR_MESS, CR_CHECK, CR_NONE, CR_HELLO};
    return lesson[tutWant_];
  }
  return CR_NONE;
}

// First-hour lessons: every few minutes the Lil Blob asks for the next kind of care it hasn't had yet.
// Medicine and the light are taught by the timed sickness and the sleepy nap (scheduledTick), not asked for here.
void Game::lessonTick() {
  if (!lessonsOn()) return;
  if (lessonsDone()) {   // all 7 icons with a lesson used: lessons are over
    pet.d.pad2 |= 0x4000; tutWant_ = -1;
    sound(SND_WIN); say("ALL LEARNED", 3.0f);
    return;
  }
  const float a = pet.d.age;
  if (tutWant_ >= 0) {
    if (!(pet.d.pad2 & (1u << tutWant_))) return;          // still waiting for that icon
    tutWant_ = -1; tutNext_ = a + pet.rndRange(150, 270);   // answered: the next one in a few minutes
    return;
  }
  if (newbornT_ > 0 && SQUEEZE_SECS + NEWBORN_AWAKE - newbornT_ < 6.1f) return;   // the first HELLO! comes 1-2 s after the drop splats (5.0 s into the squeeze)
  if (tutNext_ <= 0) tutNext_ = a + 60;
  if (a < tutNext_ || pet.d.exploring || pet.sleeping) return;   // asked only while they're awake
  static const uint8_t order[5] = {IC_CALL, IC_FEED, IC_LITTER, IC_PLAY, IC_METER};
  int want = -1;
  for (int i = 0; i < 5; i++) if (!(pet.d.pad2 & (1u << order[i]))) { want = order[i]; break; }
  if (want < 0) return;
  // set the scene so the icon really does something
  switch (want) {
    case IC_FEED: if (pet.d.hunger > 10) pet.d.hunger = 10; break;
    case IC_PLAY: if (pet.d.fun > 10) pet.d.fun = 10; break;
    case IC_LITTER: if (!pet.d.droppings && !pet.d.floorMess) return; break;   // waits for a real poop
    default: break;
  }
  tutWant_ = want;
  sound(SND_COO);
}

// Their sleep hours: the young sleep longest. Lil Blob 8pm-10am, Lil Blub 9pm-9am, Lil Blunkus 9:30pm-8:30am,
// Lil Fella 10pm-8am.
void Game::bedtime(int& startMin, int& endMin) const {
  const int st = stageOfForm(form());
  if (st == 0) { startMin = 20 * 60; endMin = 10 * 60; }
  else if (st == 1) { startMin = 21 * 60; endMin = 9 * 60; }
  else if (st == 2) { startMin = 21 * 60 + 30; endMin = 8 * 60 + 30; }
  else { startMin = 22 * 60; endMin = 8 * 60; }
}
bool Game::isNight(int h, int m) const {
  int st, en; bedtime(st, en);
  const int t = h * 60 + m;
  return t >= st || t < en;
}

// Timed events of growing up (game seconds, so debug speed works). They only start while they're awake.
void Game::scheduledTick() {
  PetData& d = pet.d;
  const float a = d.age;
  const bool awake = !pet.sleeping && !d.exploring && form() != F_DROPPER;
  // the starter bug: once, inside the first 5 minutes (the medicine lesson; it doesn't count as one of their sicknesses).
  // After that they only get sick from going hungry or a night with the light on (pet.cpp).
  // While the lessons are on, both tests have to be passed for real before "ALL LEARNED": the bug cured with
  // medicine, the light turned off while they nap (doIcon marks them). So neither is skipped: each waits until
  // they're awake, and comes round again if it was missed.
  const bool learning = lessonsOn();
  const bool needMeds = learning && !(d.pad2 & (1u << IC_MEDS)), needLight = learning && !(d.pad2 & (1u << IC_LIGHT));
  if (!(d.sick[3] & SF_BUG_DONE)) {
    if (d.nextSick <= 0) d.nextSick = pet.rndRange(150, SICK_BUG_BY);
    if ((a >= d.nextSick && awake) || (!needMeds && a >= SICK_BUG_BY + 60)) {   // (not learning, and asleep or out at the time: it's skipped)
      d.sick[3] |= SF_BUG_DONE;
      if (awake && (needMeds || a < SICK_BUG_BY + 60)) pet.getSick(SICK_BUG);
    }
  } else if (needMeds && awake && !pet.sick()) {
    pet.getSick(SICK_BUG);   // the bug went by without a dose of medicine (an older save, or it was skipped): here's another
  }
  // the sleepy nap, a few minutes after the bug (6-9 minutes old): they want the light off (the light lesson).
  // A nap slept through with the light left on doesn't teach it: another comes 3 minutes after they wake.
  if (!d.sleepyMin) d.sleepyMin = (uint16_t)pet.rndRange(6, 9.99f);
  if (learning && !needLight) d.pad2 |= 0x2000;   // already learned (the light turned off on a real night's sleep): no test nap needed
  if (needLight && !(d.pad2 & 0x2000) && d.sleepyMin > 10 && a < d.sleepyMin * 60.0f)   // (a save from when the nap came at 45-58 minutes)
    d.sleepyMin = (uint16_t)(a / 60.0f + 2);
  if (!(d.pad2 & 0x2000) && a >= d.sleepyMin * 60.0f && awake) {
    d.pad2 |= 0x2000;
    pet.nap = TEST_NAP_SECS;
    sound(SND_YAWN);
  } else if (needLight && (d.pad2 & 0x2000) && awake && pet.nap <= 0 && a >= d.sleepyMin * 60.0f + 60.0f) {
    d.pad2 &= (uint16_t)~0x2000u;
    d.sleepyMin = (uint16_t)(a / 60.0f + 3);
  }
}

// Runs once per game second (so debug fast-forward works).
void Game::careTick(float s) {
  if (!sv.hasBlob || form() == F_DROPPER) return;
  lessonTick();
  scheduledTick();
  // a call left alone for 15 minutes counts as a care mistake (once per call)
  const CallReason r = callReason();
  const bool bugOnly = r == CR_SICK && pet.sickStack() == pet.d.sick[SICK_BUG];   // the starter bug is a lesson, never a mistake
  const bool neglect = (r == CR_HUNGRY || r == CR_BORED || r == CR_MESS || r == CR_LIGHT || r == CR_SICK) && !bugOnly;
  if (neglect && r != lastCall_) neglectT_ = 0;   // a new kind of call starts its own clock
  if (neglect) {
    neglectT_ += s;
    // the first mistake after 15 minutes, then one more for every further hour they're left like this
    if (neglectT_ >= NEGLECT_LIMIT) { neglectT_ -= HOUR; if (pet.d.pad1[0] < 250) pet.d.pad1[0]++; }
  } else neglectT_ = 0;
  if (r != lastCall_) {
    if (r != CR_NONE) sound(SND_COO_SOFT);
    lastCall_ = r;
  }
}

// ---------------------------------------------------------------------------------------------
// input / clock / power
// ---------------------------------------------------------------------------------------------
// A tap is a press + release that stays within TOUCH_TAP_SLOP (or TOUCH_QUICK_SLOP for a quick tap). Hit tests use
// where the finger went down. Holding still for TOUCH_LONG_MS is a long press (fires once).
void Game::updateInput(bool down, int x, int y) {
  in.pressed = in.released = in.tap = in.longPress = false;
  if (down && !in.down) { in.pressed = true; in.px = x; in.py = y; in.downMs = nowMs; in.longDone = false; in.maxMove = 0; in.sumX = in.sumY = 0; in.nSamp = 0; }
  if (down) {
    in.x = x; in.y = y; in.heldMs = (int)(nowMs - in.downMs);
    in.sumX += x; in.sumY += y; in.nSamp++;
    in.ax = (int)(in.sumX / in.nSamp); in.ay = (int)(in.sumY / in.nSamp);
    const int mv = abs(x - in.px) > abs(y - in.py) ? abs(x - in.px) : abs(y - in.py);
    if (mv > in.maxMove) in.maxMove = mv;   // the whole path counts: a scrub that comes back to the start is not a tap
  }
  const bool still = in.maxMove <= TOUCH_TAP_SLOP;
  if (down && still && !in.longDone && in.heldMs >= TOUCH_LONG_MS) { in.longPress = true; in.longDone = true; }
  if (!down && in.down) {
    in.released = true;
    in.tap = !in.longDone && (still || (in.heldMs < TOUCH_QUICK_MS && in.maxMove <= TOUCH_QUICK_SLOP));
  }
  in.down = down;
}

void Game::updateClock() {
  DateTime d;
  if (plat->rtcGet(d)) { clockOk = true; epoch = toEpoch(d) + turboOffset_; }
  else { clockOk = false; epoch = sv.fallbackClock + (nowMs / 1000) + turboOffset_; }
  DateTime n = fromEpoch(epoch);
  hour = n.hour;
  minute = n.minute;
}

void Game::backHomeForSleep() {
  if (!sv.hasBlob) return;
  if (scr == S_MENU || scr == S_METER || scr == S_DODGE || scr == S_RUN || scr == S_ORBIT || scr == S_POP || scr == S_YEET || scr == S_SCRUB || scr == S_LOCK || scr == S_SETTINGS || scr == S_CONFIRM) setScreen(S_MAIN);
}

void Game::updatePower() {
  // left untouched for a few seconds: the screen goes off (unless Settings says never). The motion sensor is not
  // used any more (it is switched off, see hal), so this no longer waits for the device to be laid flat.
  // Not during setup, a game or the look picker; a tap or the button turns it on again.
  const uint32_t offMs = autoOffMs();
  if (screenOn_ && scr == S_ANIM) idleSec = 0;   // an animation counts as something happening: it is never cut off part-way
  if (screenOn_ && offMs && plat->idleAutoOff() && !in.down && idleSec * 1000 >= offMs &&
      (scr == S_MAIN || scr == S_MENU || scr == S_METER || (scr == S_ANIM && an_ != AN_MEDS))) {
    backHomeForSleep();
    screenOn_ = false;
  }
  static const uint8_t kLevels[5] = {40, 80, 130, 190, 255};
  uint8_t base = kLevels[(sv.brightness < 1 ? 1 : (sv.brightness > 5 ? 5 : sv.brightness)) - 1];
  const bool setup = scr == S_SETUP_CLOCK || scr == S_SETUP_NAME || scr == S_TITLE || scr == S_ENDING;
  // games keep the screen bright while they're played (an untouched game goes home after GAME_IDLE_SECS)
  bool keepOn = setup || scr == S_DODGE || scr == S_RUN || scr == S_ORBIT || scr == S_EVOLVE || scr == S_YEET || scr == S_POP || scr == S_SCRUB;
  if (keepOn && !setup && idleSec > 300.0f) keepOn = false;   // (the picker)
  if (!keepOn && idleSec > 180.0f && offMs) {   // untouched for 3 minutes: off (unless Auto off is Never; it stays dimmed)
    if (screenOn_) backHomeForSleep();
    screenOn_ = false;
  } else if (!keepOn && idleSec > 45.0f) {
    bright_ = (uint8_t)(base * 15 / 100 < 8 ? 8 : base * 15 / 100);
  } else {
    bright_ = base;
  }
  if (!screenOn_) bright_ = 0;
}

// ---------------------------------------------------------------------------------------------
// simulation
// ---------------------------------------------------------------------------------------------
SimCtx Game::ctx(bool away) const {
  SimCtx c;
  c.night = isNight(hour, minute);
  c.lightOff = lightOff();
  c.away = away;
  c.locked = locked();
  c.newborn = newbornT_ > 0;   // the first 30 minutes after the drop: bedtime doesn't count yet (naps do)
  return c;
}

void Game::stepSim(float seconds, bool away) {
  if (form() == F_DROPPER) return;   // the dropper waits for you to push it
  pet.tick(seconds, ctx(away));
  if (stageOfForm(form()) == 4) pet.d.hunger = 100;   // Big Blorbus never gets hungry
  if (newbornT_ > 0) newbornT_ -= seconds;
  careTick(seconds);
}

void Game::simulate(float seconds, bool away) {
  int n = (int)seconds;
  for (int i = 0; i < n; i++) {
    turboOffset_++;
    epoch++;
    DateTime d = fromEpoch(epoch);
    hour = d.hour; minute = d.minute;
    stepSim(1.0f, away);
    handleEvents();
    if (!sv.hasBlob) break;
    checkGrowth();
  }
}

void Game::updateSim() {
  simAccum_ += dt;
  clockT_ += dt;
  if (clockT_ >= 1.0f) { clockT_ = 0; updateClock(); }
  int guard = 0;
  while (simAccum_ >= 1.0f && guard++ < 8) {
    simAccum_ -= 1.0f;
    if (!sv.hasBlob) continue;
    for (int k = 0; k < timeScale; k++) {
      if (k > 0) {
        turboOffset_++;
        epoch++;
        DateTime d = fromEpoch(epoch);
        hour = d.hour; minute = d.minute;
      }
      stepSim(1.0f, false);
      if (timeScale > 1) handleEvents();
      if (pet.ended || !sv.hasBlob) break;
    }
  }
  saveT_ += dt;
  if (saveCool_ > 0) saveCool_ -= dt;
  if (saveT_ > 180.0f) autosave(false);
}

// A plain tap's bloop waits a frame: if the tap did something with a sound of its own (a menu opening, a
// refusal, a pop), that sound plays instead of both on top of each other.
void Game::sound(Sound s) {
  if (s == SND_TAP) { tapPending_ = true; return; }
  tapPending_ = false;
  if (sv.volume > 0 && timeScale == 1) plat->playSound(s, sv.volume);
}

void Game::setScreen(Screen s) {
  prev_ = scr;
  scr = s;
  scrT_ = 0;
  in.tap = false;
  in.pressed = false;
  in.longPress = false;
  menuPress_ = -1;
  if (s == S_MAIN) selIcon_ = -1;
}

void Game::handleEvents() {
  Event e;
  while (pet.pop(e)) {
    switch (e.type) {
      case EV_ENDING: endLife(e.arg); return;
      case EV_POOP:
        if (scr != S_MAIN) sound(SND_PLOP);   // (on the home screen you see them go: the sounds are in the animation)
        // they trot over to the box (or protests on the floor) if you're watching
        pottyPending_ = e.arg; pottyAge_ = 0;   // shown as soon as you're on the home screen
        break;
      case EV_SICK:
        if (e.arg != SICK_BUG && sv.sickCount < 255) sv.sickCount++;
        sound(SND_SICK); stayAwake();   // the sick icon shows: they need medicine
        break;
      case EV_GIFT_BACK: if (e.arg2) { sound(SND_BOING); say("GIFT!"); } break;
      case EV_EXPLORE_GO: sound(SND_COO_SOFT); say("EXPLORING", 3); break;
      case EV_WARN: sound(SND_SAD); break;
      case EV_WOKE_ANGRY: sound(SND_GRUMBLE); say(e.arg ? "YAWN" : "GRR!"); break;
      // morning: the light comes on (woken in the night - fed, say - it stays off); a newborn up from its test nap at night
      // gets the light too, they're not in bed yet
      case EV_WAKE: if (!isNight(hour, minute) || newbornT_ > 0) { setLight(false); sound(SND_COO_SOFT); } break;
      case EV_SLEEP: if (isNight(hour, minute)) sound(SND_YAWN); break;   // bedtime
      default: break;
    }
  }
}

// ---------------------------------------------------------------------------------------------
// main frame
// ---------------------------------------------------------------------------------------------
bool Game::frame(Frame& f, bool touchDown, int physX, int physY) {
  uint32_t now = plat->millis();
  float d = nowMs ? (now - nowMs) / 1000.0f : 0.033f;
  if (d < 0.001f) d = 0.001f;
  if (d > 0.1f) d = 0.1f;
  dt = d;
  nowMs = now;
  if (tapPending_) { tapPending_ = false; if (sv.volume > 0 && timeScale == 1) plat->playSound(SND_TAP, sv.volume); }
  anim_ += dt;
  scrT_ += dt;
  idleSec += dt;
  if (msgT_ > 0) msgT_ -= dt;
  if (clockShowT_ > 0) clockShowT_ -= dt;
  if (petCd_ > 0) petCd_ -= dt;

  // PWR / BOOT button: screen off / on
  bool justOff = false;
  if (plat->powerButton()) {
    if (screenOn_) {
      backHomeForSleep();
      screenOn_ = false; idleSec = 200.0f;
      in.down = false; swallow_ = touchDown;
      justOff = true;
      autosave(true);
    } else { wake(); swallow_ = true; }
  }
  // the touch that wakes the screen only wakes it (it doesn't also press whatever is under the finger)
  if (!justOff && !screenOn_ && touchDown && !in.down) { wake(); swallow_ = true; }
  if (swallow_) { if (!touchDown) swallow_ = false; touchDown = false; }
  updateInput(touchDown, physX / 2, physY / 2);
  if (in.down || in.released) wake();
  // a game left untouched: after GAME_IDLE_SECS it's put away (and the screen can rest)
  {
    const bool game = scr == S_DODGE || scr == S_RUN || scr == S_ORBIT || scr == S_POP || scr == S_YEET || scr == S_SCRUB;
    if (!game || in.down || scr != gameScr_) gameIdle_ = 0;
    else if ((gameIdle_ += dt) >= GAME_IDLE_SECS) { gameIdle_ = 0; setScreen(S_MAIN); }
    gameScr_ = scr;
  }

  updateSim();
  handleEvents();
  checkGrowth();
  if (pottyPending_) {
    pottyAge_ += dt;
    if (pottyAge_ > 600 || !sv.hasBlob) pottyPending_ = 0;   // too long ago to show
    else if (pet.sleeping) pottyPending_ = 0;   // never show an awake trip to the box while they sleep
    else if (screenOn_ && scr == S_MAIN && !lightOff() && !pet.d.exploring && form() != F_DROPPER) { startAnim(AN_POTTY, 2.8f, pottyPending_); pottyPending_ = 0; }
  }
  updatePower();

  if (!screenOn_) {
    if (dirty_) { autosave(true); dirty_ = false; }
    return false;
  }
  dirty_ = true;

  // a long press on the LCD opens Settings in the middle of an animation or an ending too (the home screen's own
  // long press is in mainInput); see openSettings
  if ((scr == S_ANIM || scr == S_ENDING) && in.longPress) { int dx, dy; if (inMatrix(in.px, in.py, dx, dy)) openSettings(); }

  const bool lcdScreen = scr >= S_MAIN && scr <= S_ENDING;
  if (lcdScreen) {
    const bool popView = scr == S_POP;   // POP has the whole screen: a taller LCD, no icons, a BACK button
    m_.h = popView ? POP_MH : MH; m_.y0 = popView ? POP_MY : MY;
    m_.clear();
    switch (scr) {
      case S_MAIN: drawMain(true); break;
      case S_MENU: screenMenu(); break;
      case S_ANIM: screenAnim(); break;
      case S_METER: screenMeter(); break;
      case S_DODGE: screenDodge(); break;
      case S_RUN: screenRun(); break;
      case S_ORBIT: screenOrbit(); break;
      case S_EVOLVE: screenEvolve(); break;
      case S_POP: screenPop(); break;
      case S_YEET: screenYeet(); break;
      case S_SCRUB: screenScrub(); break;
      case S_LOCK: screenLock(); break;
      case S_ENDING: screenEnding(); break;
      default: break;
    }
    f.clear(SH_BG);
    const bool tall = popView && scr == S_POP;   // (not on the frame it is left on: that one is the ordinary screen again)
    if (!tall) { m_.h = MH; m_.y0 = MY; }
    if (tall) drawPopBack(f); else drawIcons(f);
    const bool dark = lightOff() && sv.hasBlob && (scr == S_MAIN || scr == S_ANIM);
    m_.render(f, dark ? SH_MID : SH_FAINT, dark ? SH_MID : SH_BG);
    f.roundFrame(MX - 4, m_.y0 - 4, MW * DOT + 7, m_.h * DOT + 7, SH_FAINT);
  } else {
    f.clear(SH_BG);
    switch (scr) {
      case S_SETUP_CLOCK: pageClock(f); break;
      case S_SETUP_NAME: pageName(f); break;
      case S_TITLE: pageTitle(f); break;
      case S_SETTINGS: pageSettings(f); break;
      case S_CONFIRM: pageConfirm(f); break;
      case S_FRIENDS: pageFriends(f); break;
      case S_JOURNAL: pageJournal(f); break;
      case S_ABOUT: pageAbout(f); break;
      default: scr = S_MAIN; break;
    }
  }
  if (showTouch && in.down) {   // touch check: a ring where the controller says the finger is, plus its coordinates
    f.ring(in.x, in.y, 5, SH_INK);
    f.ring(in.x, in.y, 7, SH_BG);
    f.set(in.x, in.y, SH_INK);
    char b[24];
    snprintf(b, sizeof b, "%d,%d", in.x * 2, in.y * 2);
    f.rect(W / 2 - 30, 2, 60, 10, SH_BG);
    f.textCenter(W / 2, 3, b, SH_INK);
  }
  return true;
}

// LOCK: on or off. (Unlocking just lets time run on again from where it stopped.)
void Game::setLocked(bool on) {
  PetData& d = pet.d;
  d.pad0[1] = on ? 1 : 0;
  if (!on) {   // unlocking a neglected fella: a little room to put things right before anything can end
    if (d.health < 30) d.health = 30;
    if (d.resent > 80) d.resent = 80;
    if (d.chonk > CHONK_LIMIT - 10) d.chonk = CHONK_LIMIT - 10;
    d.sadness = 0;
  }
  autosave(true);
}

}  // namespace lf
