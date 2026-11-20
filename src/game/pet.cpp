#include "pet.h"
#include "breeds.h"
#include <string.h>
#include <math.h>

namespace lf {

static constexpr float NEGLECT_K = 0.06f;   // tuned with sim/balance (total neglect -> ~3 days)

static inline float clampf(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }

// piecewise-linear table over age in hours
static float byAge(float age, const float (*pts)[2], int n) {
  const float h = age / HOUR;
  if (h <= pts[0][0]) return pts[0][1];
  for (int i = 1; i < n; i++)
    if (h < pts[i][0]) return pts[i - 1][1] + (pts[i][1] - pts[i - 1][1]) * (h - pts[i - 1][0]) / (pts[i][0] - pts[i - 1][0]);
  return pts[n - 1][1];
}
// How often they poop, by stage (minutes): Lil Blob 3, Lil Blub 6, Lil Blunkus 10, Lil Fella 15, Big Blorbus / last stages 20.
float poopInterval(float age) {
  return 60.0f * (age < AGE_BLUB ? 3.0f : age < AGE_JUNIOR ? 6.0f : age < AGE_ADULT ? 10.0f : age < AGE_BLORBUS ? 15.0f : 20.0f);
}
// Fullness and Happy: about an hour from full to empty for a Lil Blob, ~2-3 h for a Lil Blub,
// ~5 h for a Lil Blunkus, ~9 h for a grown Lil Fella.
float drainScale(float age, bool fun) {
  static const float h[][2] = {{0, 9.5f}, {1, 9.5f}, {2, 6}, {12, 4}, {24, 3}, {72, 1.5f}, {96, 1}};
  static const float f[][2] = {{0, 13}, {1, 13}, {2, 8}, {12, 5}, {24, 4}, {72, 2}, {96, 1}};
  return fun ? byAge(age, f, 7) : byAge(age, h, 7);
}

int giftValue(uint8_t p) {
  switch (p) {
    case GIFT_FLOWER: return 2;
    case GIFT_SHELL: return 3;
    case GIFT_PEBBLE: return 3;
    case GIFT_BERRY: return 5;
    case GIFT_GEM: return 12;
    default: return 0;
  }
}
const char* endingName(uint8_t e) {
  switch (e) {
    case END_RECYCLED: return "Recycled";
    case END_RAN_AWAY: return "Lost trust";
    case END_DISSOLVED: return "Overfed";   // (weight is hidden from the player: no word for it on screen)
    case END_SICK: return "Too sick";
    case END_SAD: return "Too sad";
    case END_RESET: return "Started over";
    default: return "-";
  }
}

void Pet::newPet(uint32_t seed, int breed, const char* name) {
  memset(&d, 0, sizeof(d));
  strncpy(d.name, name, sizeof(d.name) - 1);
  d.breed = (uint8_t)breed;
  d.hunger = 20; d.fun = 20; d.energy = 85; d.health = 100; d.chonk = 34; d.resent = 0;   // a new Lil Blob arrives hungry and bored: lots to do
  rng = seed * 2654435761u + 0x9E3779B9u;
  if (!rng) rng = 1;
  for (int i = 0; i < 5; i++) rnd();
  d.digestTimer = rndRange(90.0f, 180.0f);   // the first one comes within 3 minutes
  d.exploreTimer = rndRange(3 * HOUR, 6 * HOUR);
  sleeping = false; napping = false; ended = false; sadAcc = 0;
  grump = 0; disturbCd = 0; lastWarn = 0;
  qh_ = qt_ = 0;
}

void Pet::push(EventType t, int a, int b) {
  q_[qt_] = {t, (int16_t)a, (int16_t)b};
  qt_ = (uint8_t)((qt_ + 1) % 12);
  if (qt_ == qh_) qh_ = (uint8_t)((qh_ + 1) % 12);
}
bool Pet::pop(Event& e) {
  if (qh_ == qt_) return false;
  e = q_[qh_];
  qh_ = (uint8_t)((qh_ + 1) % 12);
  return true;
}

Stage Pet::stage() const {
  if (d.age < AGE_JUNIOR) return STAGE_YOUNG;
  if (d.age < AGE_ADULT) return STAGE_JUNIOR;
  if (d.age < AGE_SENIOR) return STAGE_ADULT;
  return STAGE_SENIOR;
}
int Pet::evolveChoice() const {
  float h = d.age / HOUR; if (h < 1) h = 1;
  auto cl = [](float v) { return v < 0 ? 0.0f : (v > 1 ? 1.0f : v); };
  float speed = cl(d.acc[ACC_SPEED] / 4.0f), strength = cl(d.acc[ACC_STRENGTH] / 4.0f);
  float clean = cl(d.acc[ACC_CLEAN] / (0.9f * h)) * 0.7f + cl(d.cleanActs / 5.0f) * 0.3f;
  float wavg = d.acc[ACC_WEIGHT] / h, weight = cl((wavg - 0.35f) / 0.25f);
  float neglect = cl(d.acc[ACC_NEGLECT] / h * 2.5f), love = cl(d.acc[ACC_LOVE] / (0.8f * h));
  float sc[7];
  sc[SP_PALE] = speed; sc[SP_BIG] = strength; sc[SP_PATCHY] = weight;
  sc[SP_SMOOTH] = (clean > 0.75f && wavg < 0.27f) ? clean * 0.95f : clean * 0.4f;
  sc[SP_SPECKLE] = (love + clean) * 0.5f * 0.85f;
  sc[SP_VOID] = neglect >= 0.5f ? 1.5f + neglect : neglect * 0.5f;
  sc[SP_PLAIN] = 0.42f;
  int best = SP_PLAIN;
  for (int i = 0; i < 7; i++) if (sc[i] > sc[best]) best = i;
  return best;
}

void Pet::evolve() {
  d.breed = (uint8_t)evolveChoice();
  d.evolving = 0;
}

float Pet::idealWeight() const {
  if (d.age < AGE_JUNIOR) return 35.0f;
  if (d.age >= AGE_ADULT) return 50.0f;
  return 35.0f + 15.0f * (d.age - AGE_JUNIOR) / (AGE_ADULT - AGE_JUNIOR);   // growing up
}

void Pet::clampAll() {
  d.hunger = clampf(d.hunger, 0, 100);
  d.fun = clampf(d.fun, 0, 100);
  d.energy = clampf(d.energy, 0, 100);
  d.health = clampf(d.health, 0, 100);
  d.chonk = clampf(d.chonk, 3, 130);
  d.resent = clampf(d.resent, 0, 100);
  if (d.droppings > 6) d.droppings = 6;
  if (d.floorMess > 4) d.floorMess = 4;
  d.bubblesOnFloor = 0;
  for (int i = 0; i < 3; i++) if (d.sick[i] > SICK_MAX) d.sick[i] = SICK_MAX;
}

float Pet::feed(Food f, bool& overfed) {
  overfed = false;
  const float before = d.hunger;
  const Breed& b = BREEDS[d.breed < NUM_BREEDS ? d.breed : 0];
  float gained = 0;
  switch (f) {
    case FOOD_KIBBLE:
      d.hunger += 30;
      // a proper meal offsets the daily burn and gently pulls toward the ideal weight (so the young grow up)
      gained = b.gain * (1.0f + clampf((idealWeight() - d.chonk) * 0.1f, -0.9f, 2.0f));
      overfed = before >= 85;
      if (overfed) gained += 4.0f * b.gain;   // overfeeding: about 5 of these and they're visibly chonky
      d.fun += 1;
      break;
    case FOOD_TREAT:
      d.hunger += 10;
      d.fun += 20;
      gained = (3.5f + (before > 85 ? 1.5f : 0.0f)) * b.gain;
      overfed = before >= 90;
      d.resent -= 3;
      break;
    case FOOD_DIET:
      d.hunger += 24;
      gained = -0.8f;
      d.fun -= 2;
      d.health += 1;
      overfed = before >= 95;
      break;
  }
  if (overfed) { d.health -= 1.5f; d.fun -= 2; }
  d.chonk += gained;
  clampAll();
  return gained;
}

void Pet::getSick(int kind) {
  if (kind < 0 || kind >= SICK_KINDS || sickStack() >= SICK_MAX) return;
  d.sick[kind]++;
  d.health -= SICK_HIT;
  if (d.health < 1) d.health = 1;   // (the sickness itself never finishes them off on the spot)
  push(EV_SICK, kind);
}

bool Pet::vetVisit() {
  int k = d.sick[SICK_BUG] ? SICK_BUG : d.sick[SICK_HUNGER] ? SICK_HUNGER : d.sick[SICK_LIGHT] ? SICK_LIGHT : -1;
  if (k < 0) return false;
  d.sick[k]--;
  d.health = d.health + SICK_CURE > 100 ? 100 : d.health + SICK_CURE;
  d.resent = d.resent - 5 < 0 ? 0 : d.resent - 5;
  return true;
}

void Pet::petIt(float amt) {
  d.fun += amt;
  d.resent -= amt * 0.3f;
  clampAll();
}

void Pet::exercise(float funGain, float energyCost, float chonkLoss) {
  d.fun += funGain;
  d.energy -= energyCost;
  d.chonk -= chonkLoss;
  d.resent -= funGain * 0.2f;
  clampAll();
}

void Pet::disturb() {
  if (!sleeping || disturbCd > 0) return;
  sleeping = false;
  disturbCd = 60;
  grump = 20;
  d.fun -= 2;
  push(EV_WOKE_ANGRY, 1);
  clampAll();
}

void Pet::wakeGently() {
  if (!sleeping) return;
  sleeping = false;
  push(EV_WAKE);
}

float Pet::scoop(int n) {
  int removed = 0;
  while (n-- > 0 && d.droppings > 0) { d.droppings--; removed++; if (d.cleanActs < 60000) d.cleanActs++; }
  return (float)removed;
}

void Pet::poop() {
  // A content fella always uses the box, even a dirty one. Only a really grumpy fella protests on the floor.
  const bool protest = (d.resent >= 60.0f && d.droppings >= 2) || d.resent >= 85.0f;
  if (!protest) { d.droppings++; push(EV_POOP, 1); }
  else { if (d.floorMess < 4) d.floorMess++; push(EV_POOP, 2); }
  d.digestTimer = poopInterval(d.age) * rndRange(0.9f, 1.1f);
}

void Pet::tick(float dt, const SimCtx& cx) {
  const bool away = cx.away;
  const float hr = dt / HOUR;

  // ---- sleeping: when you put the device down (or they run out of energy) ----
  if (d.energy < 12.0f) napping = true;
  if (d.energy >= 50.0f) napping = false;
  if (grump > 0) grump -= dt;
  if (disturbCd > 0) disturbCd -= dt;
  // asleep at bedtime, when worn out, or for a short nap with the light off in the day
  // (a newborn's first 30 minutes: bedtime doesn't count yet, so one made at night stays up to be looked after. Naps
  // still happen: the test nap that teaches the light comes inside those 30 minutes.)
  const bool night = cx.night && !cx.newborn;
  const bool want = !d.exploring && (night || napping || (cx.lightOff && d.energy < 50.0f));
  if (!sleeping && want && grump <= 0) { sleeping = true; push(EV_SLEEP); }
  else if (sleeping && !want) { sleeping = false; push(EV_WAKE); }

  // ---- needs ----
  float hungerRate, funRate, energyRate;
  if (sleeping) { hungerRate = -2.5f; funRate = -1.5f; energyRate = 120.0f * sleepMul; }   // a night's sleep costs little
  else { hungerRate = -14; funRate = -10; energyRate = -5; }
  const float mul = away ? 0.35f : 0.75f;
  // awake, the young need you often; it eases off as they grow
  const float kH = sleeping ? 1.0f : drainScale(d.age, false), kF = sleeping ? 1.0f : drainScale(d.age, true);
  d.hunger += hungerRate * kH * mul * hr;
  d.fun += funRate * kF * mul * hr;
  d.energy += energyRate * (energyRate > 0 ? 1.0f : mul) * hr;

  // ---- weight ----
  d.chonk -= hr * (sleeping ? 0.06f : 0.15f);
  if (d.hunger <= 0) d.chonk -= hr * 0.8f;

  // ---- litter / mess ----
  // on a timer that eases off with age; at night they hold it until morning, and an empty tummy has nothing to give
  if (!sleeping) d.digestTimer -= night ? dt * 0.05f : dt;   // asleep, they hold it until they wake
  if (d.digestTimer <= 0) {
    if (d.hunger > 5 && (!d.exploring || away)) poop();
    else d.digestTimer = 60;   // try again in a minute
  }
  if (d.floorMess) d.messAge += dt; else d.messAge = 0;

  // ---- sickness ----
  // hungry right down to 0: sick (once, until they've been fed back up)
  if (d.hunger <= 0 && !(d.sick[3] & SF_HUNGER_LATCH)) { d.sick[3] |= SF_HUNGER_LATCH; getSick(SICK_HUNGER); }
  else if (d.hunger >= 25) d.sick[3] &= (uint8_t)~SF_HUNGER_LATCH;
  // a night's sleep with the light left on: count it, and judge it when the night is over
  if (sleeping && night) { d.nightSleep += dt; if (!cx.lightOff) d.nightLight += dt; }   // (a newborn's test nap isn't a night's sleep)
  else if (!cx.night && d.nightSleep > 0) {
    if (d.nightSleep >= 600.0f && d.nightLight > SICK_LIGHT_FRAC * d.nightSleep) getSick(SICK_LIGHT);
    d.nightSleep = 0; d.nightLight = 0;
  }

  // ---- exploring: now and then they slip out and bring back a gift ----
  if (!away) {
    if (d.exploring) {
      d.exploreLeft -= dt;
      if (d.exploreLeft <= 0) {
        d.exploring = 0;
        const float luck = d.health / 100.0f;
        uint8_t type = GIFT_NONE;
        if (rndf() < 0.55f + 0.4f * luck) {
          const float r = rndf();
          type = r < 0.45f ? GIFT_FLOWER : r < 0.70f ? GIFT_SHELL : r < 0.85f ? GIFT_PEBBLE : r < 0.95f ? GIFT_BERRY : GIFT_GEM;
        }
        if (type) { d.giftOnFloor = type; d.giftAge = 0; }
        push(EV_GIFT_BACK, 0, type);
      }
    } else if (!sleeping && d.giftOnFloor == GIFT_NONE && stage() != STAGE_YOUNG &&
               d.fun >= 30 && d.hunger >= 25 && d.health >= 30) {
      d.exploreTimer -= dt;
      if (d.exploreTimer <= 0) {
        d.exploring = 1;
        d.exploreLeft = rndRange(120, 240);
        d.exploreTimer = rndRange(1.5f * HOUR, 4 * HOUR);
        push(EV_EXPLORE_GO);
      }
    }
  } else if (d.giftOnFloor == GIFT_NONE && !sleeping && stage() != STAGE_YOUNG) {
    d.exploreTimer -= dt;   // while you are away they may leave a gift
    if (d.exploreTimer <= 0) {
      const float r = rndf();
      d.giftOnFloor = r < 0.5f ? GIFT_FLOWER : r < 0.8f ? GIFT_SHELL : r < 0.95f ? GIFT_PEBBLE : GIFT_BERRY;
      d.giftAge = 0;
      d.exploreTimer = rndRange(2 * HOUR, 5 * HOUR);
    }
  }
  if (d.giftOnFloor) {
    d.giftAge += dt;
    if (d.giftAge > 900.0f && !away) { d.giftOnFloor = GIFT_NONE; d.giftAge = 0; }
  }

  // ---- resentment & health ----
  // Neglect builds resentment slowly: a totally ignored fella lasts about 3 days before leaving.
  // Health only suffers while they're awake (a night's sleep is safe); at 0 they dissolve (END_SICK).
  const float NK = NEGLECT_K;
  const float hurt = sleeping ? 0.0f : hr;
  float loss = 0;   // health lost this tick
  if (d.hunger <= 0) { if (!d.sick[SICK_HUNGER]) loss += 2 * hurt; d.resent += 12 * hr * NK; }   // (once it's made them sick, the sickness is the damage)
  else if (d.hunger < 20) d.resent += 3 * hr * NK;
  if (d.fun < 10) d.resent += 6 * hr * NK;
  if (d.floorMess && d.messAge > 1200.0f) {
    loss += 1 * hurt * (d.floorMess > 2 ? 2 : d.floorMess);
    d.resent += 4 * hr * (1 + 0.5f * d.floorMess) * NK;
  }
  if (d.droppings >= 4) { loss += 1 * hurt; d.resent += 3 * hr * NK; }   // a day's worth (3) is tolerated
  const float ideal = idealWeight();
  if (d.chonk > ideal + 45) loss += 3 * hurt;
  else if (d.chonk > ideal + 30) loss += 1 * hurt;
  if (d.health < 10) d.resent += 10 * hr * NK;
  const int stack = sickStack();
  const bool sick = stack > 0;
  if (sick) {   // a sickness gets worse until it's treated (one dose each): ~30 awake hours for one alone, faster when they stack
    loss += SICK_DRAIN * stack * hurt;
    if (loss > SICK_MAX_LOSS * stack * hurt) loss = SICK_MAX_LOSS * stack * hurt;
  }
  d.health -= loss;
  const bool good = d.hunger >= 35 && d.fun >= 30 && d.floorMess == 0 && d.droppings < 3;
  if (good) {
    d.resent -= 3.5f * hr;
    if (d.chonk < ideal + 30 && !sick) d.health += 5 * hr;   // good care keeps them well, but only medicine cures
  }

  // ---- sadness: each 10 awake minutes in a row at zero happiness; cheering them up (half happy) clears it ----
  if (d.fun >= 50) { d.sadness = 0; sadAcc = 0; }
  else if (!sleeping && d.fun <= 0.5f) {
    sadAcc += dt;
    while (sadAcc >= SAD_UNIT) {
      sadAcc -= SAD_UNIT;
      if (d.sadness < 255 && ++d.sadness == SAD_WARN) push(EV_WARN, 3);   // halfway: a sad sound
    }
  }

  // ---- ageing: faster when unwell, overweight, grumpy or neglected; only great care reaches 31 days ----
  float rate = 1.0f;
  if (d.health < 70) rate += (70 - d.health) / 70.0f * 0.6f;
  if (d.chonk > ideal + 25) rate += (d.chonk - ideal - 25) / 40.0f * 0.3f;
  rate += clampf(d.resent, 0, 100) / 100.0f * 0.8f;
  if (d.hunger < 20 || d.fun < 20) rate += 0.35f;
  if (!cx.locked) d.age += dt * rate;   // (locked: time stands still for them)
  if (d.evolving) {
    if (d.chonk > 0) d.acc[ACC_WEIGHT] += hr * (d.chonk / 100.0f);
    if (d.droppings == 0 && d.floorMess == 0) d.acc[ACC_CLEAN] += hr;
    if (d.hunger < 25 || d.fun < 20 || d.resent > 40 || d.droppings >= 3 || d.floorMess > 0) d.acc[ACC_NEGLECT] += hr;
    if (d.fun > 70 && d.resent < 10) d.acc[ACC_LOVE] += hr;
  }
  if (d.evolving && d.age >= AGE_BREED) evolve();

  clampAll();

  // ---- warnings & endings ----
  if (d.resent < 50) lastWarn = 0;
  if (d.resent >= 85 && lastWarn < 2) { lastWarn = 2; push(EV_WARN, 2); }
  else if (d.resent >= 60 && lastWarn < 1) { lastWarn = 1; push(EV_WARN, 1); }

  if (cx.locked) {   // LOCK: nothing ends their life (it all still weighs on them, just never to the end)
    if (d.resent > 99) d.resent = 99;
    if (d.chonk > CHONK_LIMIT - 1) d.chonk = CHONK_LIMIT - 1;
    if (d.health < 1) d.health = 1;
    if (d.sadness > SAD_LIMIT - 1) d.sadness = SAD_LIMIT - 1;
  }
  if (away) {   // offline catch-up is gentler and never ends a life
    if (d.resent > 90) d.resent = 90;
    if (d.chonk > 99) d.chonk = 99;
    if (d.health < 5) d.health = 5;
    if (d.sadness > SAD_LIMIT - 6) d.sadness = SAD_LIMIT - 6;
  } else if (!ended && !cx.locked) {
    if (d.chonk >= CHONK_LIMIT) { ended = true; push(EV_ENDING, END_DISSOLVED); }
    else if (d.health <= 0) { ended = true; push(EV_ENDING, END_SICK); }
    else if (d.sadness >= SAD_LIMIT) { ended = true; push(EV_ENDING, END_SAD); }
    else if (d.resent >= 100) { ended = true; push(EV_ENDING, END_RAN_AWAY); }
    else if (d.age >= AGE_RETIRE) { ended = true; push(EV_ENDING, END_RECYCLED); }
  }
}

}  // namespace lf
