// Lil Fella: a classic 2-bit virtual pet, driven by Game::frame() ~30 times a second.
// The fella's needs, ageing, moods and endings live in pet.cpp;
// this layer is the classic presentation: a 40x28 dot-matrix LCD with 8 icons, all in 4 shades.
//   game.cpp    loop, clock, power, events, care rules, save
//   view.cpp    the dot-matrix scenes, icons, animations, mini-games (POP, the bubble shooter, is in pop.cpp)
//   pages.cpp   full-screen pages: first-time setup, settings, confirm
#pragma once
#include "platform.h"
#include "lcd.h"
#include "pet.h"
#include "save.h"

namespace lf {

constexpr const char* LF_VERSION = "1.0.1";   // shown in Settings and the boot log

// ---- touch (defaults; tweak here when re-testing on the device) ----
// Screen scale: ~12.7 physical px per mm, so 1 logical px = ~0.16 mm and one LCD dot (4 px) = ~0.63 mm. A fingertip
// covers 7-10 mm and lands 1-2 mm off, so every target on the LCD is hit-tested with a ~3 mm reach (TOUCH_REACH dots).
constexpr int TOUCH_TAP_SLOP = 24;      // logical px (~4 mm) the finger may drift between press and release and still be a tap
constexpr int TOUCH_QUICK_MS = 300;     // a quick tap may drift further (fingers roll as they lift)
constexpr int TOUCH_QUICK_SLOP = 40;
constexpr int TOUCH_REACH = 5;          // dots around a small LCD target that still count as touching it
constexpr int SETTINGS_Y0 = 20;        // Settings rows: y of the first row
constexpr int SETTINGS_STEP = 30;      // distance between rows
constexpr int SETTINGS_ROW_H = 28;     // row height: 56 physical px, about 4.5 mm
constexpr uint32_t AUTO_OFF_MS[5] = {5000, 0, 15000, 30000, 60000};   // untouched this long: the screen turns off
                                                     // (Settings: Auto off; indexed by sv.eq[2]: 5 Sec, Never, 15 Sec, 30 Sec, 1 Min)
constexpr int TOUCH_LONG_MS = 1200;     // hold this long on the LCD to open Settings

constexpr float DROP_AGE = 10.0f;       // a new Lil Blob's starting age (s), once it's squeezed out of the dropper
// Stages: Lil Blub at 1 hour, Lil Blunkus at 1 day, Lil Fella at 3 days (see pet.h).
constexpr float BABY_END = AGE_BLUB;
constexpr float CHILD_END = AGE_JUNIOR;
constexpr float TEEN_END = AGE_ADULT;
constexpr float SQUEEZE_SECS = 7.6f;    // the squeeze out of the dropper: the long fall from the sky, then the gooey landing
constexpr float NEWBORN_AWAKE = 1800.0f;  // a fresh Lil Blob's bedtime doesn't start until this long after landing: one made during
                                          // sleep time falls asleep 30 minutes afterwards. Naps aren't held back (the test nap that
                                          // teaches the light comes at 6-9 minutes), and one still going at 30 minutes runs on into the night
constexpr float ENDING_WAIT_AT = 4.6f;  // an ending: they melt, and the dropper is down over the puddle, waiting for your push
constexpr int ENDING_PULLS = 3;       // an ending: squeezes needed to suck the puddle up
constexpr float ENDING_UP_AT = 3.0f;   // seconds after the last squeeze when the dropper is back up top: the name page opens
constexpr float EAT_SECS = 2.6f;        // eating: bites (Lil Blunkus and Lil Fella)
constexpr float ABSORB_SECS = 6.0f;     // eating: walk over, stretch round the food, zoom in on it dissolving (Lil Blob, Lil Blub)
constexpr float GAME_IDLE_SECS = 30.0f; // a game (or the scrub) left untouched this long goes back to the home screen
constexpr float RUN_CARD_SECS = 4.0f;    // RUN: the how-to card (tap = jump, tap again = double jump) goes on its own after this long, or at a tap
// ---- POP: the bubble shooter (pop.cpp) ----
constexpr int POP_COLS = 8;             // bubbles across (the odd rows hold one fewer, half a bubble along)
constexpr int POP_MH = 43;              // POP has the whole screen: its LCD is this many rows of dots (the icons are put away)...
constexpr int POP_MY = 30;              // ...and starts here in the frame (logical px), under the BACK button
constexpr int POP_ROWS = 9;             // rows the field keeps (more than fit on screen: the ceiling's drops use them up)
constexpr int POP_SAFE_ROWS = 7;        // rows that fit above the line; a bubble below it ends the game
constexpr int POP_START_ROWS = 3;       // full rows to start with...
constexpr int POP_START_EXTRA = 4;      // ...and up to this many more hanging under them
constexpr int POP_DROP_EVERY = 8;       // shots between the ceiling coming down a row
constexpr float POP_CARD_SECS = 4.0f;   // the how-to card goes on its own after this long, or at a tap
constexpr float POP_MAX_ANG = 1.31f;    // aim: up to 75 degrees either side of straight up
// The five kinds of bubble: no colours on this LCD, so they differ by what's inside them.
enum PopKind : uint8_t { PK_NONE, PK_X, PK_TRIANGLE, PK_DIAMOND, PK_EMPTY, PK_BLACK, PK_COUNT };

// ---- litter box (right-hand side of the LCD) ----
constexpr int BOX_X = 29;               // left dot column of the litter box (floor mess is laid out leftward from here)
constexpr int BOX_W = 11;               // the box: on screen only while there's poop in it waiting to be cleaned

// ---- icons: 4 on top, 4 at the bottom, like the original ----
enum IconId : uint8_t { IC_FEED, IC_LIGHT, IC_PLAY, IC_MEDS, IC_LITTER, IC_METER, IC_LOCK, IC_CALL, IC_COUNT };   // (IC_LOCK took the training icon's place)
constexpr int ICON_TOP_Y = 20, ICON_BOT_Y = 188;   // top-left y of the 16x16 icons
constexpr int ICON_X[4] = {28, 65, 103, 140};      // icon left x (kept clear of the rounded screen corners)
// Where each icon sits: slots 0-3 along the top, 4-7 along the bottom. (Icon ids stay fixed: the first-hour
// lessons save one bit per id.) The litter scoop is top right, the meter (stats) bottom left, medicine next to it.
constexpr IconId ICON_AT[IC_COUNT] = {IC_FEED, IC_LIGHT, IC_PLAY, IC_LITTER, IC_METER, IC_MEDS, IC_LOCK, IC_CALL};
constexpr int iconSlot(int ic) { for (int s = 0; s < IC_COUNT; s++) if (ICON_AT[s] == ic) return s; return 0; }
constexpr int iconX(int ic) { return ICON_X[iconSlot(ic) % 4]; }
constexpr int iconY(int ic) { return iconSlot(ic) < 4 ? ICON_TOP_Y : ICON_BOT_Y; }

enum Screen : uint8_t {
  S_SETUP_CLOCK, S_SETUP_NAME, S_TITLE,
  S_MAIN, S_MENU, S_ANIM, S_METER, S_DODGE, S_RUN, S_ORBIT, S_EVOLVE, S_POP, S_YEET, S_SCRUB, S_LOCK, S_ENDING,
  S_SETTINGS, S_CONFIRM, S_FRIENDS, S_JOURNAL, S_ABOUT, S_COUNT
};
enum AnimId : uint8_t {
  AN_NONE, AN_EAT_MEAL, AN_EAT_SNACK, AN_EAT_DIET, AN_REFUSE, AN_MEDS, AN_LOCK,
  AN_SQUEEZE, AN_GROW, AN_PET, AN_POTTY,
  AN_LEVELUP   // about to grow up: they pulse small-big three times among stars, then the look picker opens
};
constexpr float LEVELUP_SECS = 2.5f;
constexpr float MEDS_SECS = 2.2f;   // the medicine: the syringe, then the sparkle (nothing can cut it short)
enum ConfirmId : uint8_t { CF_NONE, CF_NEWBLOB, CF_ERASE };

// what the fella looks like right now
enum Form : uint8_t { F_DROPPER, F_BABY, F_S0, F_S0B, F_S0C, F_TN_BEAN, F_TN_BUN, F_TN_CURL, F_S1C, F_AD_LOAF,
                    F_AD_PUDDLE, F_AD_JELLY, F_TN_SPROUT, F_AD_DANGO, F_AD_PUFF,
                    F_TN_SLOSH, F_TN_WOBBLES, F_TN_MALLOW, F_TN_TOASTY, F_AD_MARSHY, F_AD_BOOP, F_AD_FUBZ, F_AD_NIMBUS,
                    F_AD_BALLBOI, F_BB_BLOBBERT, F_BB_BALLBOI, F_BB_ALIEN, F_COUNT };
// (Form values are saved in the friends log: new looks are only ever added at the end.)
// The evolution tree: at each new stage the player picks one of the looks the path so far allows
// (stage 1 Lil Blub, 2 Lil Blunkus, 3 Lil Fella).
struct EvoChoice { Form form; const char* name; };
int stageOfForm(Form f);   // 0 Lil Blob (and the dropper), 1 Lil Blub, 2 Lil Blunkus, 3 Lil Fella, 4 Big Blorbus

// why the attention icon is lit (highest priority first)
enum CallReason : uint8_t { CR_NONE, CR_HUNGRY, CR_BORED, CR_SICK, CR_MESS, CR_LIGHT, CR_UPSET, CR_HELLO, CR_CHECK };

class Game {
 public:
  explicit Game(Platform* plat);
  Game(const Game&) = delete;
  Game& operator=(const Game&) = delete;

  void begin();
  // One frame. Touch in physical px (0..367, 0..447). Returns true if the display should be refreshed.
  bool frame(Frame& f, bool touchDown, int physX, int physY);
  uint8_t brightness() const { return bright_; }
  bool displayOn() const { return screenOn_; }
  const Palette& palette() const { return sv.eq[0] == 1 ? PAL_NIGHT : PAL_LCD; }
  void saveNow() { autosave(true); }

  // ---- state (public: handy for the simulator & tests) ----
  Platform* plat;
  SaveData sv;
  Pet pet;
  Screen scr = S_SETUP_CLOCK;
  uint32_t nowMs = 0;
  float dt = 0.033f;
  int hour = 12, minute = 0;
  uint32_t epoch = 0;
  float idleSec = 0;
  bool clockOk = false;
  int timeScale = 1;   // debug fast-forward
  bool showTouch = false;

  void simulate(float seconds, bool away);
  const char* nameBuf() const { return nameBuf_; }
  void endNewborn() { newbornT_ = 0; }   // tests: skip the first 30 awake minutes
  int tutWant() const { return tutWant_; }
  int anim() const { return an_; }   // tests: the animation playing
  float blobX() const { return blobX_; }   // tests: where the fella stands (dots)
  Form form() const;
  CallReason callReason() const;
  // the Journal (Settings page 2): every look ever raised on this device
  void markSeen(Form f) { if (f >= F_BABY && f < F_COUNT) sv.journal |= 1u << f; }
  bool seenLook(int f) const { return f == F_BABY || (sv.journal >> f & 1u); }   // (Lil Blob: everyone starts as one)
  int journalCount(int stage, int* total) const;   // looks raised in a stage (0 Lil Blob .. 4 Big Blorbus; -1 all), total = how many there are
  void debugAct(int which);   // tests / web page: 0 feed meal, 1 snack, 2 clean, 3 meds, 5 dodge, 6 meter, 7 settings, 8 squeeze the dropper now

 private:
  struct In {
    bool down = false, pressed = false, released = false, tap = false, longPress = false, longDone = false;
    int x = 0, y = 0, px = 0, py = 0;
    int ax = 0, ay = 0;         // where the finger rested: the average of every reading in this press
    long sumX = 0, sumY = 0; int nSamp = 0;
    int maxMove = 0;            // farthest the finger got from where it went down (px), over the whole press
    uint32_t downMs = 0;
    int heldMs = 0;
  } in;
  bool swallow_ = false;

  // timing / power
  float simAccum_ = 0, clockT_ = 0, saveT_ = 0, saveCool_ = 0;
  uint32_t turboOffset_ = 0;
  bool screenOn_ = true;
  uint8_t bright_ = 200;
  float anim_ = 0, scrT_ = 0;
  bool dirty_ = false;

  // screens
  Screen prev_ = S_MAIN;
  int menuPress_ = -1;
  AnimId an_ = AN_NONE; float anT_ = 0, anLen_ = 0; int anArg_ = 0;
  int meterPage_ = 0;
  ConfirmId confirm_ = CF_NONE;
  int setPage_ = 0;           // Settings page: 0, 1, 2
  Screen setFrom_ = S_MAIN;   // where Settings was opened from: an animation or an ending carries on after Back
  float setFromT_ = 0;        // ... and how far into that screen it was (scrT_)
  void openSettings();
  void leaveSettings();
  int friendsPage_ = 0;       // the friends log: 4 to a page
  int journalSel_ = -1;       // the Journal: the look tapped (its name shows), -1 = none
  float suckAt_ = -1;         // ending: when you pushed the dropper (scrT_), -1 = not yet
  bool squeezing_ = false;    // ending: your finger is squeezing the bulb
  int endPulls_ = 0;          // ending: squeezes done so far (ENDING_PULLS needed to suck it all up)
  float pullAt_ = -1;         // ending: when the last squeeze was let go
  bool nameFresh_ = true;       // the name on the page is still a random one: the first key you type replaces it
  char nameBuf_[12] = "";       // the name on the name page (random, see pickName)
  int nameSeed_ = 0;
  int clockSel_[5] = {2026, 1, 1, 12, 0};
  Screen clockNext_ = S_SETUP_NAME;
  int selIcon_ = -1;          // icon highlighted (the menu/action in use)
  char msg_[12] = ""; float msgT_ = 0;
  int titleBar_ = -1;         // title: the bar of the tune that is playing
  float clockHoldT_ = 0;      // set-clock: how long a finger has been held on an arrow
  int clockHoldDir_ = 0;      // +1 / -1 / 0
  int clockHoldFld_ = -1;
  float clockShowT_ = 0;     // tap on the empty LCD shows the time

  // the fella on the LCD
  float blobX_ = 12, blobTx_ = 12;
  bool faceLeft_ = false;
  float stepT_ = 0.6f, blinkT_ = 3;
  int stepPhase_ = 0;
  Form lastForm_ = F_COUNT;
  int evoSel_ = 0;
public:   // (the simulator tests the evolution tree)
  // picks are packed in pad0[0]: bits 0-1 Lil Blub, bits 2-4 Lil Blunkus, bits 5-7 Lil Fella (0 = not picked yet, else index+1)
  int evoChoice(int stage) const { return stage == 4 ? ((pet.d.pad1[1] >> 2) & 7) : stage == 1 ? (pet.d.pad0[0] & 3) : stage == 2 ? ((pet.d.pad0[0] >> 2) & 7) : ((pet.d.pad0[0] >> 5) & 7); }
  void evoSetChoice(int stage, int n) {   // n = index + 1
    if (stage == 4) { pet.d.pad1[1] = (uint8_t)((pet.d.pad1[1] & ~(7 << 2)) | ((n & 7) << 2)); return; }   // Big Blorbus: 1 taken, 7 turned down
    const int shift = stage == 1 ? 0 : stage == 2 ? 2 : 5, mask = stage == 1 ? 3 : 7;
    pet.d.pad0[0] = (uint8_t)((pet.d.pad0[0] & ~(mask << shift)) | ((n & mask) << shift));
  }
  static uint8_t packLooks(int child, int teen, int adult) { return (uint8_t)(child | teen << 2 | adult << 5); }
  Form lilFellaForm() const;                            // the look before Big Blorbus
  static Form bigOf(Form f, int child);                            // the Big Blorbus a Lil Fella can grow into (F_COUNT = none)
  int evoCount(int stage) const;                       // how many looks the path so far allows at this stage
  const EvoChoice& evoOption(int stage, int i) const;
  int pendingEvolution() const;
  void blobSprite(Form f, int& idle, int& alt, int& sleep) const;
  float ballHx_ = 0;
  // RUN: a side-scroller, tap to jump the things coming at you
  struct Run { int phase, hitX, air2, cause, sinceP, wallDone, nextP; float t, dist, y, vy, next, score, boost, card; struct { float x; uint8_t w, h, kind, by; bool on; uint8_t dj; } ob[6]; } rn_{};   // wallDone: double-jumped over a tall wall (needed for the 4th heart); nextP: things to come before the next power-up; card: seconds the how-to card has left (0 = playing)
  void runStep(float dtReal, bool press, bool sim);   // one tick of RUN (sim = look-ahead for tests: no spawning, sounds or rewards)
  float runBest_ = 0;
  // ORBIT CATCH: the fella circles a sun on one of two orbits eating planets, tap to hop between them (Big Blorbus's game, in the Feed slot)
  struct Orbit { int phase, lane, stars, hits, slot, lastAst, gk; float t, th, lanePos, flash, gulp, gx, gy; struct { float a; uint8_t lane, kind; bool on; } it[14]; } orb_{};   // (stars: planets eaten, hits: tummy aches; gulp/gx/gy/gk: the last bite)
  int orbitBest_ = 0;
  void orbitStep(float dtReal, bool tap, bool sim);
  void migrateLooks();                                  // saves from before the evolution tree: re-pack the picked looks
private:
  float petCd_ = 0;

  // care
  float neglectT_ = 0;
  CallReason lastCall_ = CR_NONE;

  // first-hour lessons (like the 2nd-generation Tamagotchi): the Lil Blob asks for each kind of care in turn,
  // and only grows into a Lil Blub once every icon has been used. pet.d.pad2: bits 0-7 icons used,
  // bit 14 lessons finished (reward given), bit 15 lessons switched on for this fella.
  int tutWant_ = -1;          // icon the Lil Blob is asking for right now
  float tutNext_ = 0;         // age (s) of the next request
  float newbornT_ = 0;        // game seconds a fresh Lil Blob is kept awake (from the push: the landing + NEWBORN_AWAKE)
  void squeezeOut() { pet.d.age = DROP_AGE; newbornT_ = SQUEEZE_SECS + NEWBORN_AWAKE; }   // the dropper is pushed
  void scheduledTick();       // the timed events of growing up: the starter bug, the sleepy nap
  void bedtime(int& startMin, int& endMin) const;   // their sleep hours (minutes of the day) by life stage
  bool isNight(int h, int m) const;
  bool lessonsOn() const { return (pet.d.pad2 & 0x8000) && !(pet.d.pad2 & 0x4000); }
  static constexpr uint16_t LESSON_MASK = 0xFF & ~(1u << IC_LOCK);   // the 7 icons that have a lesson (LOCK has none)
  bool lessonsDone() const { return (pet.d.pad2 & LESSON_MASK) == LESSON_MASK; }
  void markIcon(int ic) { if (pet.d.pad2 & 0x8000) pet.d.pad2 |= (uint16_t)(1u << ic); }
  void lessonTick();


public:
  // POP (Play menu): a bubble shooter after Puzzle Bobble. Aim, fire, match three of a kind (pop.cpp).
  struct Pop {
    int phase;                    // 1 aiming, 2 a bubble in flight, 3 over (a moment to see how), 4 the result
    float t, card, overT, ang, bx, by, vx, vy, shake, carry;   // card: seconds the how-to card has left; ang: the aim (0 = straight up)
    int drops, sinceDrop, shots, popped, start, hearts;  // drops: rows the ceiling has come down; start: bubbles at the start
    bool aiming, won;
    uint8_t cur, next;            // the bubble in the launcher, and the one after it (PopKind)
    uint8_t g[POP_ROWS][POP_COLS];   // the field: g[row][col], row 0 against the ceiling
    struct { float x, y, vy, t; uint8_t kind, fall; bool on; } fx[40];   // bubbles popping (fall 0) or dropping off (1)
  } pop_{};
  static int popCols(int r) { return (r & 1) ? POP_COLS - 1 : POP_COLS; }
  static int popCellX(int r, int c) { return c * 5 + ((r & 1) ? 2 : 0); }   // a bubble's left dot column...
  int popCellY(int r) const { return (r + pop_.drops) * 5; }                 // ...and its top row
  static int popNeighbours(int r, int c, int nr[6], int nc[6]);
  static float popAngleAt(int px);                    // the aim for a finger at this logical x
  bool popTrace(float ang, int& r, int& c, int* bounces = nullptr) const;   // where a shot at this angle would land
  int popCount() const;                               // bubbles on the field
  void popFire(float ang);
  void startPop();
private:
  bool popAdvance(float& x, float& y, float& vx, float& vy, float dist, int& r, int& c, bool* bounced) const;
  void popLand(int r, int c);
  void popEnd(bool won);
  uint8_t popPick();
  void screenPop();
  static bool popBackAt(int px, int py);   // is this spot (logical px) on POP's BACK button?
  void drawPopBack(Frame& f);
  float gameIdle_ = 0;        // seconds a game screen has gone untouched (GAME_IDLE_SECS: back home)
  Screen gameScr_ = S_MAIN;

public:
  // Clean (soap icon): scrub the mess away - rub a finger back and forth, the suds build up, it wipes clean
  // Every poop is different: its shape, where it lies, and how stuck-on it is (tough 1-5: up to twice the rubbing).
  // Every poop since the last clean is in it, and each is more to scrub.
  struct Scrub {
    float t, prog, need, base, doneT; int lx, ly, box, floor, ns, np; bool had, done;
    struct { int8_t x, y; uint8_t k; float born; } suds[48];   // born: S.t when it was made (they fade after SUDS_SECS)
    struct { int8_t x, y; uint8_t shape, tough; float hp, hp0; } poo[BOX_MAX + 4];   // (the box, and up to 4 on the floor)
  } sc_{};
  void startScrub();
  void screenScrub();
public:
  // YEET: pull the fella back in the sling and let go; whatever is out past the edge bounces them around
  struct YeetOb { float x, y; uint8_t kind; bool on; float t; };
  struct Yeet {
    int phase;               // 0 the card (how to + best), 1 aiming, 2 flying, 3 landed, 4 result
    float t, x, y, vx, vy, camX, camY, rest, spin, pullX, pullY, ax0, ay0, maxX, hitT, squash, landT, gustT;
    int rot, meters, lastHit, hearts, drawBot, gyScr;
    bool aiming, onGround, best, inMud, splatDone;
    float nextOb;
    YeetOb ob[10];
    struct { float x, y, vx, vy, life; uint8_t k; } pt[16];
  } yt_{};
  void yeetLaunch(float pullX, float pullY);          // let go with the band pulled back this far (dots; tests)
  void yeetStep(float dtReal);                         // one tick of the flight
  static constexpr float YEET_PULL = 9.0f;             // furthest the band stretches (dots)
private:
  void startYeet();
  void screenYeet();
  void drawYeetFella(float cx, float cy, int rot, float sx, float sy, bool flip, int mode);

  // mini-games
  struct Dodge { int phase, hitX; float t, x, tx, next, score; bool left; struct { float x, y, v; uint8_t kind, w, h; bool on; } it[10]; } dg_{};
  float dodgeBest_ = 0;   // best Dodge time since power-on
  void screenRun();
  void screenOrbit();
  friend struct DodgeBot;   // sim: a bot that plays Dodge to check the difficulty curve
  friend struct Reporter;   // sim: scripted playthroughs for the balance report
  friend struct TouchMap;   // sim: where every touch lands
  friend struct JournalTest;   // sim: the Journal
  friend struct NewTests;      // sim: v0.07 (sickness, scrub, POOP UP, YEET)
  friend struct NewShots;
  friend struct V010;          // sim: v0.10 (POP, scrub resume, RUN card, wiki pictures)

  // ---- game.cpp ----
  void updateInput(bool down, int x, int y);
  void updateClock();
  void updateSim();
  void stepSim(float seconds, bool away);
  SimCtx ctx(bool away) const;
  void careTick(float seconds);
  void handleEvents();
  void updatePower();
  void backHomeForSleep();
  void wake() { idleSec = 0; screenOn_ = true; }      // only for a tap, the button or power-on
  void stayAwake() { if (screenOn_) idleSec = 0; }   // something happened: keep a lit screen lit, never switch it on
  void setScreen(Screen s);
  void sound(Sound s);
  bool tapPending_ = false;   // a plain tap sounds only if nothing else did that frame (see sound())
  void autosave(bool force);
  void applyOfflineCatchUp();
  void newPet();
  void endLife(int ending);
  void logFriend(int ending);   // add the current fella to the friends log
  const char* randomName();
  void pickName();            // a random name onto the name page
  void say(const char* s, float secs = 2.0f);
  uint32_t autoOffMs() const { return AUTO_OFF_MS[sv.eq[2] < 5 ? sv.eq[2] : 0]; }   // 0 = never
  bool lightOff() const { return pet.d.pad1[1] & 1; }
  void setLight(bool off) { if (off) pet.d.pad1[1] |= 1; else pet.d.pad1[1] &= (uint8_t)~1u; }
public:
  // LOCK: they stay at this stage and can't die; hunger, mess and sickness carry on. Unlock any time.
  bool locked() const { return pet.d.pad0[1] == 1; }
  void setLocked(bool on);
private:
  void screenLock();
  int lockSel_ = -1;
  void checkGrowth();

  // ---- view.cpp ----
  Matrix m_;
  void drawIcons(Frame& f);
  void drawMain(bool interactive);
  void drawBlob(int x, int y, bool flip, int mode, bool alt);
  void drawScene(bool withBlob);
  void drawZzz(int cx, int cy, int w, uint8_t shade);
  void drawLitter(int boxPoos, int floorPoos, bool box = false);   // box: draw the box even when it's empty (they're using it)
  int pottyPending_ = 0; float pottyAge_ = 0;   // a poop they did while you weren't looking at the home screen
  void mainInput();
  bool iconTapped(int& which) const;
  bool inMatrix(int x, int y, int& dx, int& dy) const;
  void doIcon(int ic);
  void startAnim(AnimId a, float len, int arg = 0);
  void screenMenu();
  void feedMeal();
  void screenAnim();
  void screenMeter();
  void screenDodge();
  void screenEvolve();
  void screenEnding();
  uint8_t coatFill() const;
  uint8_t sickShade() const;   // the sickly speckle shade (0 = not sick)
  void drawSickIcon(int cx, int cy, int w);
  int eatSprite(int idle) const;
  bool absorbsFood() const { return form() == F_BABY; }   // Lil Blob absorbs; every other look has its own eating routine (eatfx)
public:
  int eatRoutine() const;     // that routine for the current look (eatfx::Routine), 0 = the plain three bites
  float eatSecs() const;
  uint8_t eatVar_ = 0;        // the routine's random variant, picked every feed (Marshy: which wall)
  int eatFromX_ = 0;          // where they stood when the food came (the Lil Blob's meal is laid out from it: see drawAbsorb)
  const uint8_t* dots() const { return m_.d; }   // the LCD dots (tests)
private:
  float ballHy_ = 0, ballHv_ = 0, ballSq_ = 0, ballPrevX_ = 0; int ballRun_ = 1; bool ballWasMoving_ = false;   // Big Ballboi's hamster
  void drawBallboi(int x, int y, bool still);
  void updateBall();
  void drawSquish(int x, int y, int id, int w, int h, bool flip, uint8_t sick = 0);   // a sprite stretched to w x h dots (the body engulfing food)
  void drawBelly(int x, int y, int id, int ew, int eh, bool flip);   // a sprite with columns let into its side and rows into its belly
  void drawAbsorb(float t);                         // the Lil Blob eats by absorbing its food
  int gameSprite() const;     // the look used in the games (a smaller one for the young)
  void drawGameBlob(int x, int y, bool flip, int mode, bool alt);
public:
  const char* lifeStageName() const;
private:
public:
  static const char* lookName(int form);   // "Lil Blob", "Mochi", ...
private:
  int lookSprite(int form) const;          // a look's standing sprite
  void updateBlobWalk();

  // ---- pages.cpp ----
  void pageClock(Frame& f);
  void pageName(Frame& f);
  void pageTitle(Frame& f);
  void pageSettings(Frame& f);
  void pageConfirm(Frame& f);
  void pageFriends(Frame& f);
  void pageJournal(Frame& f);
  void pageAbout(Frame& f);
  void eraseAll();            // Settings → Erase everything
  bool pageButton(Frame& f, int x, int y, int w, int h, const char* label, bool enabled = true);
  bool tapIn(int x, int y, int w, int h) const;
};

}  // namespace lf
