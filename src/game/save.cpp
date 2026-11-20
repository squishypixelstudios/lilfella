#include "save.h"
#include <string.h>

namespace lf {

uint32_t crc32(const uint8_t* p, uint32_t n) {
  uint32_t c = 0xFFFFFFFFu;
  for (uint32_t i = 0; i < n; i++) {
    c ^= p[i];
    for (int k = 0; k < 8; k++) c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1u)));
  }
  return ~c;
}

void saveDefaults(SaveData& s) {
  memset(&s, 0, sizeof(s));
  s.magic = SAVE_MAGIC;
  s.version = SAVE_VERSION;
  s.size = sizeof(SaveData);
  s.mode = MODE_UNSET;
  s.volume = 2;   // quiet by default (0..5): the first-run clock screen was far too loud
  s.brightness = 4;
  s.eq[2] = 3;              // Auto off: 30 Sec until the player changes it (see AUTO_OFF_MS)
  s.fixes = FIX_AUTOOFF;    // (a fresh save needs no move to the new default: a later pick of 5 Sec must stick)
}

void saveSeal(SaveData& s) {
  s.magic = SAVE_MAGIC;
  s.version = SAVE_VERSION;
  s.size = sizeof(SaveData);
  s.crc = crc32(reinterpret_cast<const uint8_t*>(&s) + 12, sizeof(SaveData) - 12);
}

bool saveValid(const SaveData& s) {
  if (s.magic != SAVE_MAGIC || s.version != SAVE_VERSION || s.size != sizeof(SaveData)) return false;
  return s.crc == crc32(reinterpret_cast<const uint8_t*>(&s) + 12, sizeof(SaveData) - 12);
}

}  // namespace lf
