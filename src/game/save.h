// Persistent game data. One flat POD blob with magic/version/CRC.
#pragma once
#include "pet.h"
#include <stddef.h>

namespace lf {

constexpr uint32_t SAVE_MAGIC = 0xC47F00D5u;
constexpr uint16_t SAVE_VERSION = 7;
constexpr int HALL_MAX = 8;

// The layout is fixed so saves from every earlier build keep loading. Bytes no longer used (left over from an
// earlier game) are kept as reserved space; static_asserts below pin every field to its place.
struct SaveData {
  uint32_t magic;
  uint16_t version;
  uint16_t size;
  uint32_t crc;            // CRC32 of everything after this field

  uint8_t mode;            // Mode (MODE_UNSET = a brand new device: the clock page comes first)
  uint8_t volume;          // 0..5 (0 = off)
  uint8_t brightness;      // 1..5
  uint8_t reserved0[3];
  uint8_t clockSet;        // user confirmed the clock
  uint8_t hallCount;       // entries in hall[] (the Old friends log)
  uint8_t reserved1[4];
  uint32_t unused0;        // (was the bubbles currency, removed in v0.07)
  uint8_t reserved2[5];
  uint8_t hasBlob;         // 0 until the dropper is filled (a name picked)
  uint8_t reserved3[22];
  uint8_t eq[4];           // [0] palette (1 = night), [1] touch debug, [2] auto off (see AUTO_OFF_MS)
  uint8_t reserved4[8];

  uint32_t lastSeen;       // epoch seconds (local) at last save
  uint32_t fallbackClock;  // used if the RTC is unavailable

  uint32_t lifeFed, lifeGames, lifeBubbles, lifeGifts, lifeBlobs;   // lifetime counts

  HallEntry hall[HALL_MAX];   // the Old friends log, newest first
  PetData pet;
  uint32_t journal;        // the Journal: bit f set = that look (Form f) has been raised (Lil Blob always counts)
  uint16_t yeetBest;       // YEET: the farthest launch ever (metres)
  uint8_t sickCount;       // this fella's sicknesses so far (the starter bug doesn't count)
  uint8_t fixes;           // one-time save upgrades done (FIX_ bits)
  uint8_t reserved5[24];
};
constexpr uint8_t FIX_SICK = 1;   // v0.07: sickness reworked, bubbles gone
constexpr uint8_t FIX_LOCK = 2;   // v0.08: training gone; its save byte (pad0[1]) is the LOCK now
constexpr uint8_t FIX_AUTOOFF = 4;   // v1.0.1: Auto off no longer waits for the device to lie flat; the old default (5 Sec) becomes 30 Sec
static_assert(offsetof(SaveData, mode) == 12 && offsetof(SaveData, clockSet) == 18 && offsetof(SaveData, unused0) == 24 &&
              offsetof(SaveData, hasBlob) == 33 && offsetof(SaveData, eq) == 56 && offsetof(SaveData, lastSeen) == 68 &&
              offsetof(SaveData, lifeFed) == 76 && offsetof(SaveData, hall) == 96 && offsetof(SaveData, pet) == 256 &&
              offsetof(SaveData, journal) == 376 && offsetof(SaveData, yeetBest) == 380 && sizeof(SaveData) == 408, "the save layout must not change (old saves would be lost)");

void saveDefaults(SaveData& s);
void saveSeal(SaveData& s);
bool saveValid(const SaveData& s);
uint32_t crc32(const uint8_t* p, uint32_t n);

}  // namespace lf
