// Blob sounds (synthesised by src/game/synth.cpp) -> ES8311 codec over I2S (16 kHz, 16-bit stereo).
// Runs in its own FreeRTOS task so the game never waits on audio. Compiled only with LF_AUDIO 1 (hal.h).
#include "hal.h"
#if !LF_AUDIO
namespace hal {
void audioBegin() {}
void audioPlay(lf::Sound, uint8_t) {}
}  // namespace hal
#else
#include <Arduino.h>
#include <Wire.h>
#include <ESP_I2S.h>
#include <math.h>
#include "board.h"
#include "hal.h"
#include "../game/synth.h"

extern "C" {
#include "es8311/es8311.h"
}

namespace hal {

static I2SClass i2s;
static QueueHandle_t q = nullptr;
static bool ready = false;
static constexpr int SR = 16000;

struct Msg { uint8_t snd; uint8_t vol; };

// The sounds themselves (wet bloops, squelches, bubbles) live in the game core: src/game/synth.cpp.
// A few can play at once (a tap over a coo, the scrub over a pop); the oldest gives way.
static constexpr int VOICES = 4;
struct Playing { lf::SynthVoice v; float gain = 0; uint32_t born = 0; };

static void audioTask(void*) {
  static Playing voice[VOICES];
  static int16_t buf[256 * 2];
  uint32_t quietBlocks = 0, serial = 0;
  bool paOn = false;
  for (;;) {
    Msg m;
    bool any = false;
    for (auto& p : voice) if (!p.v.done()) any = true;
    // Nothing playing and the tail has run out: amp off, then sleep until the next sound is queued (no polling,
    // so a silent game costs the audio task nothing).
    const bool idle = !any && quietBlocks >= 20;
    if (idle && paOn) { digitalWrite(PA_PIN, LOW); paOn = false; }
    TickType_t wait = idle ? portMAX_DELAY : 0;
    while (xQueueReceive(q, &m, wait) == pdTRUE) {
      wait = 0;
      if (m.snd < lf::SND_COUNT) {
        int pick = 0;
        for (int i = 0; i < VOICES; i++) { if (voice[i].v.done()) { pick = i; break; } if (voice[i].born < voice[pick].born) pick = i; }
        // settings 0..5: every step is half of what it was up to v0.09 (0.05, 0.11, 0.22, 0.45, 0.85), which was too
        // loud all the way up. Speaker power goes with the square of this, so each setting draws about a quarter of
        // what it did.
        static const float g[6] = {0.0f, 0.025f, 0.055f, 0.11f, 0.225f, 0.425f};
        voice[pick].gain = g[m.vol > 5 ? 5 : m.vol];
        voice[pick].born = ++serial;
        voice[pick].v.start((lf::Sound)m.snd, SR, esp_random());
        any = true;
      }
      if (!paOn) {   // the amp needs a moment to wake: feed it silence first so short sounds (taps) are not swallowed
        digitalWrite(PA_PIN, HIGH); paOn = true;
        static int16_t z[256 * 2];
        for (int k = 0; k < 3; k++) i2s.write((uint8_t*)z, sizeof(z));
      }
      quietBlocks = 0;
    }
    if (!any && quietBlocks >= 20) continue;      // idle: block on the queue (above)
    for (int i = 0; i < 256; i++) {
      float s = 0;
      for (auto& p : voice) if (!p.v.done()) s += p.v.next() * p.gain;
      if (s > 0.6f) s = 0.6f + (s - 0.6f) / (1.0f + (s - 0.6f) * 2.5f);          // soft knee: stacked sounds squash, never crack
      else if (s < -0.6f) s = -0.6f + (s + 0.6f) / (1.0f - (s + 0.6f) * 2.5f);
      int16_t o = (int16_t)(s * 30000);
      buf[2 * i] = o;
      buf[2 * i + 1] = o;
    }
    if (!any) quietBlocks++;
    i2s.write((uint8_t*)buf, sizeof(buf));
  }
}

void audioBegin() {
  pinMode(PA_PIN, OUTPUT);
  digitalWrite(PA_PIN, LOW);
  i2s.setPins(I2S_BCK_IO, I2S_WS_IO, I2S_DO_IO, I2S_DI_IO, I2S_MCK_IO);
  if (!i2s.begin(I2S_MODE_STD, SR, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO, I2S_STD_SLOT_BOTH)) {
    Serial.println("[audio] i2s begin failed");
    return;
  }
  es8311_handle_t h = es8311_create(0, ES8311_ADDRRES_0);
  if (!h) { Serial.println("[audio] es8311 create failed"); return; }
  const es8311_clock_config_t clk = {
    .mclk_inverted = false,
    .sclk_inverted = false,
    .mclk_from_mclk_pin = true,
    .mclk_frequency = SR * 256,
    .sample_frequency = SR};
  if (es8311_init(h, &clk, ES8311_RESOLUTION_16, ES8311_RESOLUTION_16) != ESP_OK ||
      es8311_sample_frequency_config(h, clk.mclk_frequency, clk.sample_frequency) != ESP_OK ||
      es8311_voice_volume_set(h, 85, NULL) != ESP_OK) {
    Serial.println("[audio] es8311 config failed");
    return;
  }
  q = xQueueCreate(12, sizeof(Msg));
  xTaskCreatePinnedToCore(audioTask, "audio", 6144, nullptr, 2, nullptr, 0);
  ready = true;
  Serial.println("[audio] ok");
}

void audioPlay(lf::Sound s, uint8_t volume) {
  if (!ready || volume == 0) return;
  Msg m = {(uint8_t)s, volume};
  if (xQueueSend(q, &m, 0) != pdTRUE) {   // never block the game; if full, drop the oldest so the newest still plays
    Msg old; xQueueReceive(q, &old, 0); xQueueSend(q, &m, 0);
  }
}

}  // namespace hal
#endif  // LF_AUDIO
