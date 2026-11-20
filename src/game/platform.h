// Everything the portable game core needs from the outside world.
// The ESP32 firmware (src/hal) and the PC simulator (sim/) each implement this.
#pragma once
#include <stdint.h>

namespace lf {

enum Sound : uint8_t {
  SND_TAP, SND_COO_SOFT, SND_COO, SND_COO_LOUD, SND_PET, SND_EAT, SND_COIN,
  SND_SCOOP, SND_BLOW, SND_BOING, SND_WIN, SND_LOSE, SND_SAD, SND_MELT,
  SND_SCRATCH, SND_YAWN, SND_POOF, SND_GRUMBLE,
  SND_KEY, SND_MENU, SND_BACK, SND_PAGE, SND_NOPE, SND_LIGHT_ON, SND_LIGHT_OFF,
  SND_SICK, SND_GULP, SND_MEDS, SND_HEART, SND_PLOP, SND_SCRUB,
  SND_BUBBLE, SND_SPLAT, SND_SPARKLE, SND_BOUNCE, SND_STRETCH,
  SND_WHOOSH, SND_LOCK, SND_UNLOCK, SND_LEVEL,
  SND_TITLE1, SND_TITLE2, SND_TITLE3, SND_TITLE4, SND_TITLE5, SND_TITLE6, SND_TITLE7, SND_TITLE8, SND_COUNT   // (what each one sounds like: synth.cpp)
};

struct DateTime {
  uint16_t year;   // 2000..2099
  uint8_t month;   // 1..12
  uint8_t day;     // 1..31
  uint8_t hour;    // 0..23
  uint8_t minute;
  uint8_t second;
};

class Platform {
 public:
  virtual ~Platform() {}
  virtual uint32_t millis() = 0;
  // True once per press of the device's PWR or BOOT button (screen on/off).
  virtual bool powerButton() { return false; }
  // The device's battery: charge left in percent (0..100), or -1 if it can't be read / there is none.
  virtual int batteryPercent() { return -1; }
  virtual bool batteryCharging() { return false; }
  // Battery-backed clock. Returns false if the clock lost power / was never set.
  virtual bool rtcGet(DateTime& dt) = 0;
  virtual void rtcSet(const DateTime& dt) = 0;
  // Accelerometer in g. Returns false if no fresh sample.
  virtual bool imuRead(float& ax, float& ay, float& az) = 0;
  // Persistent storage of one blob.
  virtual bool loadSave(void* buf, uint32_t len) = 0;   // true only if exactly len bytes were read
  virtual bool storeSave(const void* buf, uint32_t len) = 0;
  virtual void playSound(Sound s, uint8_t volume) = 0;  // volume 1..5 (0 is never sent)
  virtual uint32_t randomSeed() = 0;
};

// Seconds since 2000-01-01 00:00:00 (local wall-clock time; no time zones needed).
uint32_t toEpoch(const DateTime& dt);
DateTime fromEpoch(uint32_t e);

}  // namespace lf
