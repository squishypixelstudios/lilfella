// Display, touch, IMU, RTC, power chip, storage, for all three boards (board.h has the pins and what differs).
// Talks to the chips directly over I2C; the libraries are Arduino_GFX for the display and, on the 2.16 only,
// SensorLib for its CST9220 touch controller.
#include <Arduino.h>
#include <string.h>
#include <Wire.h>
#include <Preferences.h>
#include <esp_wifi.h>
#include <esp32-hal-bt.h>
#include <Arduino_GFX_Library.h>
#include "board.h"
#if TOUCH_CST92XX
#include <TouchDrvCSTXXX.hpp>
#endif
#include "hal.h"
#include "../game/lcd.h"

namespace hal {

// ---------------------------------------------------------------------------------------
// I2C helpers
// ---------------------------------------------------------------------------------------
static bool wr(uint8_t addr, uint8_t reg, uint8_t val) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write(val);
  return Wire.endTransmission() == 0;
}
static bool rd(uint8_t addr, uint8_t reg, uint8_t* buf, uint8_t n) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((int)addr, (int)n) != n) return false;
  for (uint8_t i = 0; i < n; i++) buf[i] = Wire.read();
  return true;
}

// ---------------------------------------------------------------------------------------
// display
// ---------------------------------------------------------------------------------------
static Arduino_DataBus* bus = nullptr;
#if PANEL_CO5300
static Arduino_CO5300* gfx = nullptr;   // the AMOLEDs: the 2.16 and the 1.8 V2 (the original 1.8 used SH8601)
#else
static Arduino_ST7789* gfx = nullptr;   // the LCD-1.83
#endif
static uint16_t* chunk = nullptr;        // DMA-capable scratch for the upscaler
static constexpr int LW = 184, LH = 224;
static constexpr int ROWS = 16;          // logical rows per chunk -> 32 physical rows (20 at 5/4)
static_assert(LW == lf::W && LH == lf::H && LW * 2 == GAME_PX_W && LH * 2 == GAME_PX_H, "touches are in the frame at 2x");
#if PIC_SCALE_5_4
static_assert(LW * 5 / 4 == PIC_W && LH * 5 / 4 == PIC_H && LW % 4 == 0 && LH % 4 == 0 && ROWS % 4 == 0, "the picture is the frame at 5/4");
#else
static_assert(LW * 2 == PIC_W && LH * 2 == PIC_H, "the picture is the frame at 2x");
#endif
static_assert(PIC_X0 >= 0 && PIC_Y0 >= 0 && PIC_X0 + PIC_W <= LCD_WIDTH && PIC_Y0 + PIC_H <= LCD_HEIGHT, "the game's picture must fit on the panel");
#ifndef LF_TOUCH_LOG
#define LF_TOUCH_LOG 0   // 1 = print raw touch coordinates to the Serial Monitor (max 4 lines/s while touching)
#endif

#ifndef TOUCH_KEEP_AWAKE
#define TOUCH_KEEP_AWAKE 0         // CST816: disable auto-sleep (reg 0xFE=1) + report continuously while touched (0xFA=0x40); board.h turns it on where needed
#endif
// Boards with TOUCH_HOLD (the 2.16's CST9220, the LCD-1.83's CST816): the controller is read by polling, and a read
// that lands between two of its own scans can come back empty while the finger is still down. A finger counts as
// lifted only once nothing has been read for this long (so a release is seen this much late, which no tap or drag
// notices; Waveshare's LCD-1.83 demo holds for 80 ms the same way). 0 = believe every empty read.
#ifndef TOUCH_HOLD_MS
#define TOUCH_HOLD_MS 70
#endif


static int lastBright = -1;   // -1 = never set, so the first request always goes through
static bool haveRtc = false;
static bool haveAxp = false;
#if TOUCH_CST92XX
static TouchDrvCST92xx cst;
static bool haveTouch = false;
#else
static uint8_t touchAddr = ADDR_CST816;
#endif

#if HAS_IO_EXPANDER
static void expanderReset() {
  // TCA9554: EXIO0..2 are reset lines for display / touch / peripherals. Pulse them low, then high.
  uint8_t cfg = 0xFF, out = 0xFF;
  if (!rd(ADDR_TCA9554, 0x03, &cfg, 1)) return;
  rd(ADDR_TCA9554, 0x01, &out, 1);
  wr(ADDR_TCA9554, 0x03, cfg & ~0x07);   // pins 0..2 -> outputs
  wr(ADDR_TCA9554, 0x01, out & ~0x07);   // low
  delay(20);
  wr(ADDR_TCA9554, 0x01, out | 0x07);    // high
  delay(50);
}
#endif

// Clock out 9 pulses on SCL and send a STOP so a slave left mid-transfer releases SDA.
static void i2cRecover() {
  pinMode(IIC_SDA, INPUT_PULLUP);
  pinMode(IIC_SCL, OUTPUT);
  for (int i = 0; i < 9; i++) {
    digitalWrite(IIC_SCL, LOW); delayMicroseconds(10);
    digitalWrite(IIC_SCL, HIGH); delayMicroseconds(10);
  }
  pinMode(IIC_SDA, OUTPUT);
  digitalWrite(IIC_SDA, LOW); delayMicroseconds(10);
  digitalWrite(IIC_SDA, HIGH); delayMicroseconds(10);
}

// WiFi and Bluetooth: never used, never on. Nothing in this firmware starts either radio, so neither is powered and
// nothing scans or advertises. This makes sure of it at every boot (should a library ever bring one up, it is
// stopped and its driver freed here) and says so in the boot log.
static void radiosOff() {
  const esp_err_t w = esp_wifi_stop();
  esp_wifi_deinit();
  bool bt = false;
#if defined(CONFIG_BT_ENABLED)
  bt = btStarted();
  if (bt) btStop();
#endif
  Serial.printf("[hal] radios: wifi %s, bluetooth %s\n", w == ESP_ERR_WIFI_NOT_INIT ? "off (never started)" : "stopped", bt ? "stopped" : "off (never started)");
}

void begin() {
  radiosOff();
  i2cRecover();
  Wire.begin(IIC_SDA, IIC_SCL);
  Wire.setClock(400000);
  Wire.setTimeOut(20);
  Serial.printf("[hal] board %s, panel %dx%d, game picture %dx%d at %d,%d\n", BOARD_NAME, LCD_WIDTH, LCD_HEIGHT, PIC_W, PIC_H, PIC_X0, PIC_Y0);
#if HAS_IO_EXPANDER
  expanderReset();
#endif

#if TOUCH_CST92XX
  // touch: CST9220, set up the way Waveshare's demo for this board does it (the driver pulses the reset pin itself;
  // the bus is already up, so it isn't given the pins again). Turned to match the picture (LCD_MADCTL).
  cst.setPins(TP_RST, TP_INT);
  for (int tries = 0; tries < 3 && !haveTouch; tries++) haveTouch = cst.begin(Wire, CST92XX_SLAVE_ADDRESS);
  if (haveTouch) {
    cst.setMaxCoordinates(LCD_WIDTH, LCD_HEIGHT);
    cst.setSwapXY(TOUCH_SWAP_XY);
    cst.setMirrorXY(TOUCH_MIRROR_X, TOUCH_MIRROR_Y);
    Serial.printf("[hal] touch @0x%02X ok (%s)\n", CST92XX_SLAVE_ADDRESS, cst.getModelName());
  } else {
    Serial.println("[hal] touch not answering");
  }
#else
  // touch: CST816 / FT3168, left at its power-on defaults (except TOUCH_KEEP_AWAKE, below). Find which address answers.
#ifdef TP_RST
  pinMode(TP_RST, OUTPUT);   // (its reset line is a plain pin on this board)
  digitalWrite(TP_RST, LOW); delay(20);
  digitalWrite(TP_RST, HIGH);
#endif
  delay(50);
  {
    uint8_t probe = 0;
    if (rd(ADDR_CST816, 0x02, &probe, 1)) { touchAddr = ADDR_CST816; Serial.println("[hal] touch @0x15 ok (defaults)"); }
    else if (rd(ADDR_FT3168, 0x02, &probe, 1)) { touchAddr = ADDR_FT3168; Serial.println("[hal] touch @0x38 ok (defaults)"); }
    else Serial.println("[hal] touch not answering");
#if TOUCH_KEEP_AWAKE
    // (the chip only answers while it's awake: straight after the reset above, or while touched. Checked by reading back.)
    bool okSleep = false, okMode = false;
    for (int tries = 0; tries < 5 && !(okSleep && okMode); tries++) {
      uint8_t v = 0;
      wr(ADDR_CST816, 0xFE, 0x01);   // no auto-sleep
      wr(ADDR_CST816, 0xFA, 0x40);   // report continuously while touched
      okSleep = rd(ADDR_CST816, 0xFE, &v, 1) && v == 0x01;
      okMode = rd(ADDR_CST816, 0xFA, &v, 1) && (v & 0x40);
      if (!(okSleep && okMode)) delay(10);
    }
    Serial.printf("[hal] touch stay-awake %s, continuous reports %s\n", okSleep ? "ok" : "NOT SET", okMode ? "ok" : "NOT SET");
#endif
  }
#endif

  // display (reset: its own pin, except on the AMOLED-1.8 where it's on the IO expander above)
#if PANEL_CO5300
  bus = new Arduino_ESP32QSPI(LCD_CS, LCD_SCLK, LCD_SDIO0, LCD_SDIO1, LCD_SDIO2, LCD_SDIO3);
  gfx = new Arduino_CO5300(bus, LCD_RESET, 0, LCD_WIDTH, LCD_HEIGHT, LCD_COL_OFFSET, 0, 0, 0);
#else
  pinMode(LCD_BL, OUTPUT);
  digitalWrite(LCD_BL, LOW);   // backlight off until the first picture is up
  bus = new Arduino_ESP32SPI(LCD_DC, LCD_CS, LCD_SCLK, LCD_MOSI);
  gfx = new Arduino_ST7789(bus, LCD_RESET, 0, true /* IPS */, LCD_WIDTH, LCD_HEIGHT, LCD_COL_OFFSET, LCD_ROW_OFFSET, 0, 0);
#endif
  if (!gfx->begin()) Serial.println("[hal] gfx->begin() failed");
#ifdef LCD_MADCTL
  bus->beginWrite();
  bus->writeC8D8(0x36, LCD_MADCTL);
  bus->endWrite();
#endif
  gfx->fillScreen(0x0000);
#if PANEL_CO5300
  gfx->setBrightness(0);
#endif
  chunk = (uint16_t*)heap_caps_malloc((size_t)PIC_W * ROWS * 2 * sizeof(uint16_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
  if (!chunk) Serial.println("[hal] out of DMA memory for display chunk!");

  // IMU: QMI8658. The game no longer uses motion, so the chip is put to sleep and never read. (It keeps its
  // settings across a reset of the ESP32, so one left running by older firmware is switched off here.)
  uint8_t id = 0;
  uint8_t imuAddr = ADDR_QMI8658;
  if (!(rd(imuAddr, 0x00, &id, 1) && id == 0x05)) { imuAddr = 0x6A; if (!rd(imuAddr, 0x00, &id, 1)) id = 0; }
  if (id == 0x05) {
    wr(imuAddr, 0x08, 0x00);  // CTRL7: accelerometer and gyroscope off
    wr(imuAddr, 0x02, 0x41);  // CTRL1: bit 0 = sensor disable (its internal oscillator stops)
  }
  Serial.printf("[hal] imu %s\n", id == 0x05 ? "off" : "missing");

  // Power manager AXP2101: turn on battery detection + voltage measuring so the fuel gauge works
  {
    uint8_t v = 0;
    if (rd(ADDR_AXP2101, 0x68, &v, 1)) { wr(ADDR_AXP2101, 0x68, v | 0x01); haveAxp = true; }
    if (haveAxp && rd(ADDR_AXP2101, 0x30, &v, 1)) wr(ADDR_AXP2101, 0x30, v | 0x01);
    if (haveAxp && rd(ADDR_AXP2101, 0x41, &v, 1)) wr(ADDR_AXP2101, 0x41, v | 0x08);   // PWR key short-press flag
    if (haveAxp) wr(ADDR_AXP2101, 0x49, 0x08);                                          // clear any stale press
    Serial.printf("[hal] power chip %s\n", haveAxp ? "ok" : "missing");
  }

  // RTC: PCF85063 - make sure it is running in 24h mode
  uint8_t c1 = 0;
  if (rd(ADDR_PCF85063, 0x00, &c1, 1)) {
    c1 &= ~((1 << 5) | (1 << 1));   // STOP = 0, 24h mode
    wr(ADDR_PCF85063, 0x00, c1);
    haveRtc = true;
  }
  Serial.printf("[hal] rtc %s\n", haveRtc ? "ok" : "missing");
#if LF_AUDIO
  audioBegin();
#else
  Serial.println("[hal] audio disabled (LF_AUDIO=0)");
#endif
}

void setBrightness(uint8_t level) {
  if (!gfx || level == lastBright) return;
  lastBright = level;
#if PANEL_CO5300
  gfx->setBrightness(level);
#else
  analogWrite(LCD_BL, level);   // the LCD's backlight: PWM duty 0..255
#endif
}

#if PIC_SCALE_5_4
// 5/4 scaling: every 4x4 block of frame pixels becomes 5x5 panel pixels, each a blend of the (up to) two frame
// pixels it covers along each axis, weighted by how much of each. A block never borrows from its neighbours, so the
// changed rectangle only has to be widened to whole blocks. The dot matrix is lined up with the blocks (a dot is
// one block: 3 lit + 1 gap), so every dot comes out the same: 3 3/4 px lit and a clean 1 px gap.
static constexpr uint8_t kTapA[5] = {0, 0, 1, 2, 3}, kTapB[5] = {0, 1, 2, 3, 3}, kWB[5] = {0, 3, 2, 1, 0};   // weight of B in quarters (A gets the rest)
static_assert(lf::DOT == 4 && lf::MX % 4 == 0 && lf::MY % 4 == 0, "the dot matrix must line up with the 4x4 blocks");
// a colour with its 3 fields spread apart, so weighted sums (weights adding to 16) can't spill into each other
static inline uint32_t spread565(uint16_t c) { return ((uint32_t)c | ((uint32_t)c << 16)) & 0x07E0F81Fu; }
static inline uint16_t pack565(uint32_t v) { v &= 0x07E0F81Fu; return (uint16_t)(v | (v >> 16)); }
#endif

// Only what changed since the last flush is sent: the frame is compared, band by band (16 rows), with a copy of what
// the panel is showing, and inside a changed band only the rows and columns that differ go out (most frames only
// the fella moves, a small rectangle). A palette change repaints everything. The copy costs 41 KB of RAM; comparing
// it is a few memcmp calls, far cheaper than fingerprinting every byte, and much less goes over the bus.
static uint8_t shown[LW * LH];
static uint16_t shownPal[4];
static bool shownValid = false;

void flush(const uint8_t* frame, const uint16_t* pal) {
  if (!gfx || !chunk) return;
  const bool all = !shownValid || memcmp(shownPal, pal, sizeof shownPal) != 0;
  if (all) {   // a panel bigger than the game's picture: the rest of it is plain LCD background
    const int x0 = PIC_X0, y0 = PIC_Y0, x1 = x0 + PIC_W, y1 = y0 + PIC_H;
    if (y0 > 0) gfx->fillRect(0, 0, LCD_WIDTH, y0, pal[0]);
    if (y1 < LCD_HEIGHT) gfx->fillRect(0, y1, LCD_WIDTH, LCD_HEIGHT - y1, pal[0]);
    if (x0 > 0) gfx->fillRect(0, y0, x0, PIC_H, pal[0]);
    if (x1 < LCD_WIDTH) gfx->fillRect(x1, y0, LCD_WIDTH - x1, PIC_H, pal[0]);
  }
#if PIC_SCALE_5_4
  const uint32_t sp[4] = {spread565(pal[0]), spread565(pal[1]), spread565(pal[2]), spread565(pal[3])};
#endif
  for (int ly = 0; ly < LH; ly += ROWS) {
    const uint8_t* src = frame + ly * LW;
    uint8_t* old = shown + ly * LW;
    int x0 = 0, x1 = LW - 1, r0 = 0, r1 = ROWS - 1;
    if (!all) {
      if (memcmp(src, old, (size_t)LW * ROWS) == 0) continue;   // this band hasn't changed
      x0 = LW; x1 = -1; r0 = ROWS; r1 = -1;
      for (int r = 0; r < ROWS; r++) {
        const uint8_t* a = src + r * LW;
        const uint8_t* b = old + r * LW;
        if (memcmp(a, b, LW) == 0) continue;
        if (r < r0) r0 = r;
        r1 = r;
        int i = 0; while (a[i] == b[i]) i++;          // (the row differs somewhere, so both scans stop inside it)
        int j = LW - 1; while (a[j] == b[j]) j--;
        if (i < x0) x0 = i;
        if (j > x1) x1 = j;
      }
    }
    memcpy(old, src, (size_t)LW * ROWS);
#if PIC_SCALE_5_4
    // widen the changed rectangle to whole 4x4 blocks, then blend each block into 5x5 panel pixels
    x0 &= ~3; x1 |= 3; r0 &= ~3; r1 |= 3;
    const int pw = (x1 - x0 + 1) * 5 / 4, ph = (r1 - r0 + 1) * 5 / 4;
    for (int py = 0; py < ph; py++) {
      const int by = r0 + (py / 5) * 4, j = py % 5;
      const uint8_t* ra = src + (by + kTapA[j]) * LW + x0;
      const uint8_t* rb = src + (by + kTapB[j]) * LW + x0;
      const uint32_t wyb = kWB[j], wya = 4 - wyb;
      uint16_t* d = chunk + py * pw;
      for (int px = 0; px < pw; px++) {
        const int bx = (px / 5) * 4, i = px % 5;
        const uint8_t a0 = ra[bx + kTapA[i]] & 3, a1 = ra[bx + kTapB[i]] & 3, b0 = rb[bx + kTapA[i]] & 3, b1 = rb[bx + kTapB[i]] & 3;
        if (a0 == a1 && a0 == b0 && a0 == b1) { d[px] = pal[a0]; continue; }
        const uint32_t wxb = kWB[i], wxa = 4 - wxb;
        d[px] = pack565((sp[a0] * (wxa * wya) + sp[a1] * (wxb * wya) + sp[b0] * (wxa * wyb) + sp[b1] * (wxb * wyb)) >> 4);
      }
    }
    gfx->draw16bitRGBBitmap(PIC_X0 + x0 * 5 / 4, PIC_Y0 + (ly + r0) * 5 / 4, chunk, pw, ph);
#else
    // expand the changed rectangle to physical RGB565 (2x2 per logical pixel); always even coordinates and sizes
    const int w = x1 - x0 + 1, h = r1 - r0 + 1, pw = w * 2;
    for (int r = 0; r < h; r++) {
      const uint8_t* s = src + (r0 + r) * LW + x0;
      uint16_t* d0 = chunk + (r * 2) * pw;
      uint16_t* d1 = d0 + pw;
      for (int x = 0; x < w; x++) {
        const uint16_t c = pal[s[x] & 3];
        d0[x * 2] = d0[x * 2 + 1] = d1[x * 2] = d1[x * 2 + 1] = c;
      }
    }
    gfx->draw16bitRGBBitmap(PIC_X0 + x0 * 2, PIC_Y0 + (ly + r0) * 2, chunk, pw, h * 2);
#endif
  }
  memcpy(shownPal, pal, sizeof shownPal);
  shownValid = true;
}

// ---------------------------------------------------------------------------------------
// touch. touchRaw() reads the controller (panel coordinates); readTouch() hands the game a spot in its own 368x448
// space: the picture's place and size on the panel are undone, and a touch on the border around the picture counts
// as its nearest edge.
// ---------------------------------------------------------------------------------------
#if TOUCH_CST92XX
// CST9220: the first finger, already turned to match the picture by the driver.
static bool touchRaw(int& x, int& y) {
  int16_t xs[8], ys[8];   // (the driver fills one entry per finger it reports: 2 at most)
  if (!haveTouch || cst.getPoint(xs, ys, cst.getSupportTouchPoint()) == 0) return false;
  x = xs[0]; y = ys[0];
  return true;
}
#else
// CST816 / FT3168: plain read. Regs 0x02.. = finger count, X high, X low, Y high, Y low (same layout on both).
// Coordinates are used exactly as the controller reports them.
static bool touchRaw(int& x, int& y) {
  uint8_t b[5];
  if (!rd(touchAddr, 0x02, b, 5) || (b[0] & 0x0F) == 0) return false;
  x = ((b[1] & 0x0F) << 8) | b[2];
  y = ((b[3] & 0x0F) << 8) | b[4];
  return true;
}
#endif

bool readTouch(int& x, int& y) {
  static int lx = 0, ly = 0;
  int rx = 0, ry = 0;
  const bool got = touchRaw(rx, ry);
  if (got) { lx = rx; ly = ry; }
#if TOUCH_HOLD
  static bool seen = false;
  static uint32_t seenMs = 0;
  if (got) { seen = true; seenMs = ::millis(); }
  else if (!seen || ::millis() - seenMs > (uint32_t)TOUCH_HOLD_MS) { seen = false; return false; }
#else
  if (!got) return false;
#endif
#if LF_TOUCH_LOG
  static uint32_t lastLog = 0;
  if (::millis() - lastLog > 250) { lastLog = ::millis(); Serial.printf("[touch] %d,%d\n", lx, ly); }   // panel coords
#endif
  x = (lx - PIC_X0) * GAME_PX_W / PIC_W;
  y = (ly - PIC_Y0) * GAME_PX_H / PIC_H;
  if (x < 0) x = 0;
  if (y < 0) y = 0;
  if (x > GAME_PX_W - 1) x = GAME_PX_W - 1;
  if (y > GAME_PX_H - 1) y = GAME_PX_H - 1;
  return true;
}

// ---------------------------------------------------------------------------------------
// EspPlatform
// ---------------------------------------------------------------------------------------
static uint8_t bcd2bin(uint8_t v) { return (uint8_t)((v >> 4) * 10 + (v & 0x0F)); }
static uint8_t bin2bcd(uint8_t v) { return (uint8_t)(((v / 10) << 4) | (v % 10)); }

uint32_t EspPlatform::millis() { return ::millis(); }

bool EspPlatform::rtcGet(lf::DateTime& dt) {
  if (!haveRtc) return false;
  uint8_t r[7];
  if (!rd(ADDR_PCF85063, 0x04, r, 7)) return false;
  if (r[0] & 0x80) return false;   // oscillator-stop flag: time is not trustworthy
  dt.second = bcd2bin(r[0] & 0x7F);
  dt.minute = bcd2bin(r[1] & 0x7F);
  dt.hour = bcd2bin(r[2] & 0x3F);
  dt.day = bcd2bin(r[3] & 0x3F);
  dt.month = bcd2bin(r[5] & 0x1F);
  dt.year = (uint16_t)(2000 + bcd2bin(r[6]));
  if (dt.year < 2025 || dt.month < 1 || dt.month > 12 || dt.day < 1 || dt.day > 31 || dt.hour > 23 || dt.minute > 59) return false;
  return true;
}

void EspPlatform::rtcSet(const lf::DateTime& dt) {
  if (!haveRtc) return;
  uint32_t days = lf::toEpoch(dt) / 86400u;       // 2000-01-01 was a Saturday (=6)
  uint8_t wd = (uint8_t)((days + 6) % 7);
  Wire.beginTransmission(ADDR_PCF85063);
  Wire.write(0x04);
  Wire.write(bin2bcd(dt.second) & 0x7F);           // also clears the oscillator-stop flag
  Wire.write(bin2bcd(dt.minute));
  Wire.write(bin2bcd(dt.hour));
  Wire.write(bin2bcd(dt.day));
  Wire.write(wd);
  Wire.write(bin2bcd(dt.month));
  Wire.write(bin2bcd((uint8_t)(dt.year - 2000)));
  Wire.endTransmission();
}

// PWR key (via the AXP2101's short-press flag), the BOOT button (GPIO0, active low) or, where the board has one,
// the extra user key: one event per press.
bool EspPlatform::powerButton() {
  static bool pinReady = false, bootWas = false;
  static uint32_t lastPoll = 0;
  bool pressed = false;
#ifdef KEY_PIN
  static bool keyWas = false;
  if (!pinReady) pinMode(KEY_PIN, INPUT_PULLUP);
  const bool key = digitalRead(KEY_PIN) == LOW;
  if (key && !keyWas) pressed = true;
  keyWas = key;
#endif
  if (!pinReady) { pinMode(0, INPUT_PULLUP); pinReady = true; }
  const bool boot = digitalRead(0) == LOW;
  if (boot && !bootWas) pressed = true;
  bootWas = boot;
  // The PWR key is asked for once a second (it was 12 times a second): the power chip latches a short press until
  // it is read, so none is missed; the screen just answers the PWR key up to a second late. BOOT and the user key
  // above are plain pins, read every frame at no cost.
  if (haveAxp && ::millis() - lastPoll >= 1000) {
    lastPoll = ::millis();
    uint8_t st = 0;
    if (rd(ADDR_AXP2101, 0x49, &st, 1) && (st & 0x08)) { wr(ADDR_AXP2101, 0x49, 0x08); pressed = true; }
  }
  return pressed;
}

// Battery, from the AXP2101's fuel gauge (switched on in begin()). Read at most every 2 s.
//   reg 0x00 bit 3: a battery is connected;  reg 0xA4: charge left, 0..100 %;  reg 0x01 bits 6-5: 01 = charging
static int battPct = -1;
static bool battChg = false;
static void battPoll() {
  static uint32_t last = 0;
  static bool first = true;
  if (!haveAxp || (!first && ::millis() - last < 2000)) return;
  first = false; last = ::millis();
  uint8_t st = 0, pct = 0, chg = 0;
  if (!rd(ADDR_AXP2101, 0x00, &st, 1) || !(st & 0x08) || !rd(ADDR_AXP2101, 0xA4, &pct, 1)) { battPct = -1; battChg = false; return; }
  battPct = pct > 100 ? 100 : pct;
  battChg = rd(ADDR_AXP2101, 0x01, &chg, 1) && ((chg >> 5) & 3) == 1;
}
int EspPlatform::batteryPercent() { battPoll(); return battPct; }
bool EspPlatform::batteryCharging() { battPoll(); return battChg; }

static Preferences prefs;

// The save lives in NVS namespace "lilfella". Builds before v0.03 used an older namespace name: a save found there is
// loaded once, written to "lilfella" on the next save, and the old copy is then erased.
static const char* const NVS_NS = "lilfella";
static const char* const NVS_OLD_NS = "kitten";
static bool fromOldNs = false;

static bool loadFrom(const char* ns, void* buf, uint32_t len) {
  if (!prefs.begin(ns, true)) return false;
  bool ok = prefs.getBytesLength("save") == len && prefs.getBytes("save", buf, len) == len;
  prefs.end();
  return ok;
}

bool EspPlatform::loadSave(void* buf, uint32_t len) {
  if (loadFrom(NVS_NS, buf, len)) return true;
  fromOldNs = loadFrom(NVS_OLD_NS, buf, len);
  return fromOldNs;
}

bool EspPlatform::storeSave(const void* buf, uint32_t len) {
  if (!prefs.begin(NVS_NS, false)) return false;
  bool ok = prefs.putBytes("save", buf, len) == len;
  prefs.end();
  if (ok && fromOldNs && prefs.begin(NVS_OLD_NS, false)) { prefs.clear(); prefs.end(); fromOldNs = false; }
  return ok;
}

void EspPlatform::playSound(lf::Sound s, uint8_t volume) { audioPlay(s, volume); }
uint32_t EspPlatform::randomSeed() { return esp_random(); }

}  // namespace hal
