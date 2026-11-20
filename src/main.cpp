// Lil Fella - firmware entry point (Waveshare ESP32-S3-Touch-AMOLED-2.16 and -1.8; hal/board.h says which).
#include <Arduino.h>
#include "hal/hal.h"
#include "game/game.h"

static lf::Frame frame;           // 184x224 shades (one byte each, ~41 KB)
static hal::EspPlatform platform;
static lf::Game game(&platform);

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.printf("\n[boot] Lil Fella v%s\n", lf::LF_VERSION);
  hal::begin();
  game.begin();
  game.frame(frame, false, 0, 0);
  hal::flush(frame.px, game.palette().c);
  hal::setBrightness(game.brightness());
}

// Touch is also polled while the loop waits between frames, so a quick tap that starts and ends between two
// frames still reaches the game (as one frame of "down" at the spot it touched, then "up").
static bool tapLatched = false;
static int latchX = 0, latchY = 0;

// With the screen off a touch only has to wake it, so the controller is asked every 25 ms instead of every 8 ms:
// a third of the I2C traffic while idle, and still far quicker than any tap.
static void waitPollingTouch(uint32_t ms, bool wasDown, uint32_t every) {
  const uint32_t t0 = millis();
  while (millis() - t0 < ms) {
    int x, y;
    if (!wasDown && !tapLatched && hal::readTouch(x, y)) { tapLatched = true; latchX = x; latchY = y; }
    const uint32_t left = ms - (millis() - t0);
    delay(left < every ? (left ? left : 1) : every);
  }
}

void loop() {
  const uint32_t t0 = millis();
  int tx = 0, ty = 0;
  bool down = hal::readTouch(tx, ty);
  if (tapLatched) {
    if (!down) { down = true; tx = latchX; ty = latchY; }   // it was already lifted: show the press for one frame
    tapLatched = false;
  }
  bool refresh = game.frame(frame, down, tx, ty);
  if (refresh) hal::flush(frame.px, game.palette().c);   // flush first, then brightness: no flash of a stale image on wake
  hal::setBrightness(game.brightness());
  const uint32_t target = game.displayOn() ? 33 : 100;   // ~30 fps awake, 10 Hz while the screen is off
  const uint32_t el = millis() - t0;
  if (el < target) waitPollingTouch(target - el, down, game.displayOn() ? 8 : 25);
}
