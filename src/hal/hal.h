#pragma once
#include <stdint.h>
#include "../game/platform.h"

namespace hal {

void begin();                                   // bring up every peripheral
bool readTouch(int& x, int& y);                 // px inside the game's 368x448 picture (0..367 x 0..447); false = finger up
void flush(const uint8_t* frame184x224, const uint16_t* palette4);   // 2-bit shades -> RGB565, 2x upscale, centred on the AMOLED
void setBrightness(uint8_t level);              // 0..255, only sends on change

// audio (runs on its own task; the codec shares the I2C port). platformio.ini builds with -DLF_AUDIO=1.
// With LF_AUDIO 0 the audio code compiles to nothing (smaller firmware, silent).
#ifndef LF_AUDIO
#define LF_AUDIO 0
#endif
void audioBegin();
void audioPlay(lf::Sound s, uint8_t volume);

// The lf::Platform implementation used by the firmware.
class EspPlatform : public lf::Platform {
 public:
  uint32_t millis() override;
  bool rtcGet(lf::DateTime& dt) override;
  void rtcSet(const lf::DateTime& dt) override;
  bool loadSave(void* buf, uint32_t len) override;
  bool storeSave(const void* buf, uint32_t len) override;
  void playSound(lf::Sound s, uint8_t volume) override;
  uint32_t randomSeed() override;
  bool powerButton() override;
  int batteryPercent() override;
  bool batteryCharging() override;
};

}  // namespace hal
