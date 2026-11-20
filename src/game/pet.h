// The fella itself: needs, weight ("chonk"), growth, moods and the events that happen to it.
// Pure logic, no drawing, no hardware. Time is advanced in "sim seconds".
#pragma once
#include <stdint.h>

namespace lf {

enum Mode : uint8_t { MODE_EASY = 0, MODE_UNSET = 255 };   // saved; one mode (old saves may hold 1, the retired hard mode)
enum Stage : uint8_t { STAGE_YOUNG, STAGE_JUNIOR, STAGE_ADULT, STAGE_SENIOR };   // drain/weight phases (the looks use their own stages)
// How a fella's life ended (saved in the friends log; values never change). Every ending plays the same way: they
// dissolve into a puddle and you push the dropper to suck it back up, to squeeze out a new blob.
enum Ending : uint8_t {
  END_NONE, END_RECYCLED,   // a long, well-kept life (about a month): recycled into a new blob
  END_RAN_AWAY,             // trust ran out (once they ran away; now they dissolve like the rest)
  END_DISSOLVED,            // died: too chonky
  END_SICK,                 // died: health ran out (untreated sickness, starving, mess)
  END_SAD,                  // died: left at zero happiness too long
  END_RESET                 // Settings → New blob
};
enum Food : uint8_t { FOOD_KIBBLE, FOOD_TREAT, FOOD_DIET };
enum Gift : uint8_t { GIFT_NONE, GIFT_FLOWER, GIFT_SHELL, GIFT_PEBBLE, GIFT_BERRY, GIFT_GEM };

enum EventType : uint8_t {
  EV_NONE, EV_POOP, EV_UNUSED, EV_EXPLORE_GO, EV_GIFT_BACK, EV_ENDING, EV_WOKE_ANGRY, EV_SLEEP, EV_WAKE,
  EV_WARN, EV_SICK   // sick: arg = SickKind
};
// What made them sick. Sicknesses stack: each one needs its own dose of medicine.
enum SickKind : uint8_t { SICK_BUG, SICK_HUNGER, SICK_LIGHT, SICK_KINDS };
struct Event {
  EventType type;
  int16_t arg;   // poop: 1 box / 2 floor ; ending: Ending ; warn: 1/2
  int16_t arg2;  // gift: type
};

// Tuning (seconds). Kept in one place so balance is easy to change.
constexpr float HOUR = 3600.0f;
constexpr float DAY = 86400.0f;
// Life stages (the looks): Lil Blob, then Lil Blub at 90 min, Lil Blunkus at 1 day, Lil Fella at 3 days.
constexpr float AGE_BLUB = 15 * 60.0f;   // Lil Blob lasts 15 minutes
constexpr float AGE_JUNIOR = 1 * DAY;    // Lil Blunkus: starts exploring, starts filling out
constexpr float AGE_ADULT = 3 * DAY;     // Lil Fella: full-grown weight
constexpr float AGE_BLORBUS = 8 * DAY;   // Big Blorbus: offered from here on (unless they are locked)
constexpr float AGE_BREED = 1 * DAY;     // the hidden breed is decided by the care so far
constexpr float AGE_SENIOR = 26 * DAY;
constexpr float AGE_RETIRE = 31 * DAY;   // the best-kept fellas live a full month; poor care ages them faster
constexpr float CHONK_LIMIT = 100.0f;    // this chonky and they can't go on: they dissolve
constexpr float SICK_DRAIN = 2.5f;       // health lost per awake hour for each untreated sickness
constexpr float SICK_MAX_LOSS = 3.5f;    // while sick, the most health they lose an hour from everything together (per sickness)
constexpr float SICK_HIT = 25.0f;        // health lost the moment a sickness starts
constexpr float SICK_CURE = 25.0f;       // health back from each dose of medicine
constexpr float SICK_LIGHT_FRAC = 0.15f; // the light left on for more than this share of a night's sleep: sick on waking
constexpr float SICK_BUG_BY = 270.0f;    // the one starter bug (the medicine lesson): by this age (s), inside the first 5 minutes
constexpr int SICK_MAX = 9;              // most sicknesses at once
constexpr float SAD_UNIT = 600.0f;       // sadness: 10-minute units awake at zero happiness, cleared at half happy
constexpr int SAD_LIMIT = 72;            // 12 awake hours in a row at zero happiness: they dissolve
constexpr int SAD_WARN = 36;             // 6 hours: a warning

// One past fella in the friends log (save layout fixed: 20 bytes).
struct HallEntry {
  char name[12];
  uint8_t breed;
  uint8_t ending;             // Ending
  uint16_t age;               // hours (HE_HOURS set) or days (entries from older builds)
  uint16_t gift;
  uint8_t look;               // Form + 1 (0 = not recorded)
  uint8_t flags;              // HE_HOURS
};
constexpr uint8_t HE_HOURS = 1;

// Saved as-is (layout must not change).
struct PetData {
  char name[12];
  uint8_t breed;
  uint8_t evolving;           // 1 = the hidden breed is decided at 1 day old (AGE_BREED)
  uint8_t pad0[2];            // [0] picked looks, [1] LOCK (see game.cpp)
  float age;                  // fella-seconds
  float hunger, fun, energy;  // 0..100
  float health;               // 0..100
  float chonk;                // 0..100 (35 = ideal); 100 -> they dissolve
  float resent;               // 0..100; 100 -> runs away
  uint8_t sick[4];            // sicknesses now: [0] starter bug, [1] from hunger, [2] from the light; [3] SF_ flags
  uint8_t droppings;          // in the litter box
  uint8_t floorMess;          // on the floor
  uint8_t bubblesOnFloor;     // unused (the bubbles currency is gone)
  uint8_t giftOnFloor;        // Gift enum
  uint8_t sadness;            // 10-minute units spent awake at zero happiness (SAD_LIMIT -> they dissolve)
  uint8_t exploring;
  uint8_t pad1[2];            // [0] care mistakes, [1] light off
  float digestTimer, nightSleep, exploreTimer, exploreLeft;   // nightSleep: seconds asleep this night
  float nextSick, messAge, nightLight, giftAge;    // nextSick: age (s) of the starter bug (0 = not set); nightLight: of nightSleep, seconds with the light on
  uint16_t giftCount;         // this fella's trophies
  uint16_t pad2;              // first-hour lessons
  float acc[6];               // care habits while evolving: speed, clean, weight, strength, neglect, love
  uint16_t cleanActs;
  uint16_t sleepyMin;         // age in minutes of the one daytime sleepy nap (the light lesson); 0 = not set
};
constexpr uint8_t SF_BUG_DONE = 1, SF_HUNGER_LATCH = 2;   // the starter bug has happened; hunger hit 0 (re-armed once fed)
enum EvoAcc { ACC_SPEED, ACC_CLEAN, ACC_WEIGHT, ACC_STRENGTH, ACC_NEGLECT, ACC_LOVE };

struct SimCtx {
  bool night;             // their bedtime (depends on the life stage)
  bool lightOff;          // lights off: they sleep (in the day, until rested)
  bool away;              // offline catch-up: gentler, never ends the game
  bool newborn = false;   // just out of the dropper: stays awake (even at bedtime) so you can look after them
  bool locked = false;    // LOCK: they don't age (so never grow up) and nothing ends their life; every need carries on
};

// Upkeep that eases off as they grow up (all by age in seconds)
float poopInterval(float age);     // seconds between poops
float drainScale(float age, bool fun);   // how fast Fullness / Happy fall while awake (1 = adult)

class Pet {
 public:
  explicit Pet(PetData& data) : d(data) {}
  PetData& d;
  float sleepMul = 1.0f;   // > 1: sleep restores energy faster (lights off)

  // runtime-only state
  bool sleeping = false;
  float grump = 0;            // seconds of post-disturbance grumpiness
  float disturbCd = 0;
  uint8_t lastWarn = 0;
  bool napping = false;       // asleep because of low energy
  bool ended = false;         // an ending event has been emitted
  float sadAcc = 0;           // seconds toward the next sadness unit (or back off one)
  uint32_t rng = 12345;

  void newPet(uint32_t seed, int breed, const char* name);
  void tick(float dt, const SimCtx& cx);
  bool pop(Event& e);

  // player actions
  bool canFeed() const { return !sleeping && !d.exploring && d.hunger <= 98.0f; }   // a completely stuffed fella refuses food
  float feed(Food f, bool& overfed);       // returns chonk gained
  void petIt(float amt);                  // affection
  void exercise(float funGain, float energyCost, float chonkLoss);
  void disturb();                         // shaken while napping
  void wakeGently();
  void cleanFloor() { if (d.cleanActs < 60000) d.cleanActs++; if (d.floorMess) d.floorMess--; if (!d.floorMess) d.messAge = 0; }
  void takeGift() { d.giftOnFloor = GIFT_NONE; d.giftAge = 0; d.giftCount++; }
  int sickStack() const { return d.sick[0] + d.sick[1] + d.sick[2]; }   // how many sicknesses right now
  bool sick() const { return sickStack() > 0; }
  void getSick(int kind);                 // a new sickness (stacks)
  bool vetVisit();                        // one dose of medicine: cures one sickness (false = nothing to cure)
  float scoop(int n);                     // remove n droppings from the box, returns how many removed

  // derived values
  Stage stage() const;
  float idealWeight() const;              // 35 for the young, grows to 50 by 1 day old
  void train(int acc, float amt) { if (d.evolving) d.acc[acc] += amt; }
  int evolveChoice() const;               // which breed the care habits point at
  void evolve();
  uint32_t rnd() { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return rng; }
  float rndf() { return (rnd() & 0xFFFFFF) / 16777216.0f; }
  float rndRange(float a, float b) { return a + (b - a) * rndf(); }
  void clampAll();

 private:
  Event q_[12];
  uint8_t qh_ = 0, qt_ = 0;
  void push(EventType t, int a = 0, int b = 0);
  void poop();
};

int giftValue(uint8_t p);
const char* endingName(uint8_t e);

}  // namespace lf
