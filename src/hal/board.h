// Pin maps & I2C addresses for the three Waveshare boards the game runs on. The build picks one with
// -DLF_BOARD_AMOLED216, -DLF_BOARD_AMOLED18 or -DLF_BOARD_LCD183 (the environments in platformio.ini); with none it
// is the 2.16.
//
//   ESP32-S3-Touch-AMOLED-2.16: CO5300 480x480 QSPI AMOLED, CST9220 touch, QMI8658 IMU, PCF85063 RTC, AXP2101,
//     ES8311 codec. The display and touch reset lines are plain GPIOs (no IO expander).
//     Source: Waveshare's demo repo (examples/arduino/libraries/Mylibrary/pin_config.h) and the board's wiki pinout.
//   ESP32-S3-Touch-AMOLED-1.8: CO5300 (V2; SH8601 on the original) 368x448 QSPI AMOLED, CST816 / FT3168 touch, the
//     same IMU, RTC, power chip and codec, plus a TCA9554 IO expander that holds the reset lines.
//     Source: Waveshare's demo repo (examples/arduino/libraries/Mylibrary/pin_config.h).
//   ESP32-S3-Touch-LCD-1.83: ST7789 240x284 SPI IPS LCD with a backlight pin, CST816 touch, the same IMU, RTC and
//     power chip. Reset lines are plain GPIOs.
//     Source: Waveshare's demo repo (examples/arduino/libraries/Mylibrary/pin_config.h).
#pragma once

#if !defined(LF_BOARD_AMOLED216) && !defined(LF_BOARD_AMOLED18) && !defined(LF_BOARD_LCD183)
#define LF_BOARD_AMOLED216
#endif

// The game draws a 184x224 frame and takes touches in a 368x448 space (that frame at 2x, the AMOLED-1.8's panel).
#define GAME_PX_W 368
#define GAME_PX_H 448
// Each board says how big the picture is on its panel (PIC_W x PIC_H) and where (PIC_X0, PIC_Y0); the rest of the
// panel is filled with the LCD's background shade. PIC_SCALE_5_4 0: every frame pixel is 2x2 panel pixels.
// PIC_SCALE_5_4 1: every 4 frame pixels become 5 panel pixels (smoothed), for the smaller LCD.

#if defined(LF_BOARD_AMOLED216)
// ------------------------------------------------------------------------------- ESP32-S3-Touch-AMOLED-2.16
#define BOARD_NAME "AMOLED-2.16"
#define PANEL_CO5300 1
#define LCD_SDIO0 4
#define LCD_SDIO1 5
#define LCD_SDIO2 6
#define LCD_SDIO3 7
#define LCD_SCLK 38
#define LCD_CS 12
#define LCD_RESET 39
#define LCD_WIDTH 480
#define LCD_HEIGHT 480
#define LCD_COL_OFFSET 0
#define LCD_MADCTL 0xA0     // written to register 0x36 after init, as Waveshare's demos do: the panel is mounted a
                            // quarter turn round, this turns the picture upright
#define PIC_SCALE_5_4 0
#define PIC_W 368
#define PIC_H 448
#define PIC_X0 56           // (even numbers: the CO5300 wants even window edges)
#define PIC_Y0 16
#define TOUCH_CST92XX 1     // CST9220 at 0x5A (SensorLib's TouchDrvCST92xx)
#define TOUCH_SWAP_XY 1     // the same quarter turn for the touch panel (the demo's setSwapXY / setMirrorXY)
#define TOUCH_MIRROR_X 1
#define TOUCH_MIRROR_Y 0
#define TOUCH_HOLD 1        // a finger counts as lifted only after TOUCH_HOLD_MS without a reading (board.cpp)
#define TP_INT 11
#define TP_RST 40
#define HAS_IO_EXPANDER 0

#define IIC_SDA 15
#define IIC_SCL 14

#define I2S_MCK_IO 42
#define I2S_BCK_IO 9
#define I2S_WS_IO 45
#define I2S_DI_IO 10        // from the ES7210 microphone ADC (not used)
#define I2S_DO_IO 8         // to the ES8311
#define PA_PIN 46

#define KEY_PIN 18          // the extra user key (KEY3): active low with a pull-up on the board. Wakes the screen
                            // like PWR and BOOT do.

#elif defined(LF_BOARD_LCD183)
// --------------------------------------------------------------------------------- ESP32-S3-Touch-LCD-1.83
#define BOARD_NAME "LCD-1.83"
#define PANEL_CO5300 0      // ST7789 over plain SPI
#define LCD_DC 4
#define LCD_CS 5
#define LCD_SCLK 6
#define LCD_MOSI 7
#define LCD_RESET 38
#define LCD_BL 40           // backlight: brightness by PWM
#define LCD_WIDTH 240
#define LCD_HEIGHT 284
#define LCD_COL_OFFSET 0
#define LCD_ROW_OFFSET 0    // where the 284 rows sit in the ST7789's 320. The sketches in Waveshare's demo repo
                            // use 0; the clock sketch on their wiki uses 20. If the picture sits too high or low
                            // (a strip of noise at one edge), try 20.
#define TOUCH_HOLD 1        // a finger counts as lifted only after TOUCH_HOLD_MS without a reading (board.cpp)
#define PIC_SCALE_5_4 1
#define PIC_W 230           // 184 x 5/4
#define PIC_H 280           // 224 x 5/4
#define PIC_X0 5
#define PIC_Y0 2
#define TOUCH_CST92XX 0     // CST816: read directly over I2C
#define TOUCH_KEEP_AWAKE 1  // this board's CST816 must be told to stay awake and report the finger continuously
                            // (Waveshare's demo sets the same "periodic" mode); at its defaults it only reports
                            // now and then, so drags didn't register and taps landed on stale spots
#define TP_INT 13
#define TP_RST 39
#define HAS_IO_EXPANDER 0

#define IIC_SDA 15
#define IIC_SCL 14

// audio: the same pins as the AMOLED-1.8 (from Waveshare's ESP-IDF board support package for this board)
#define I2S_MCK_IO 16
#define I2S_BCK_IO 9
#define I2S_WS_IO 45
#define I2S_DI_IO 10        // from the ES7210 microphone ADC (not used)
#define I2S_DO_IO 8         // to the ES8311
#define PA_PIN 46

#else
// -------------------------------------------------------------------------------- ESP32-S3-Touch-AMOLED-1.8
#define BOARD_NAME "AMOLED-1.8"
#define PANEL_CO5300 1
#define LCD_SDIO0 4
#define LCD_SDIO1 5
#define LCD_SDIO2 6
#define LCD_SDIO3 7
#define LCD_SCLK 11
#define LCD_CS 12
#define LCD_RESET GFX_NOT_DEFINED   // on the IO expander
#define LCD_WIDTH 368
#define LCD_HEIGHT 448
#define LCD_COL_OFFSET 16
#define PIC_SCALE_5_4 0
#define PIC_W 368
#define PIC_H 448
#define PIC_X0 0
#define PIC_Y0 0
#define TOUCH_CST92XX 0     // CST816 (V2) or FT3168 (original): read directly over I2C
#define TOUCH_HOLD 0        // every reading is believed as it comes
#define TP_INT 21
#define HAS_IO_EXPANDER 1

#define IIC_SDA 15
#define IIC_SCL 14

#define I2S_MCK_IO 16
#define I2S_BCK_IO 9
#define I2S_WS_IO 45
#define I2S_DI_IO 10
#define I2S_DO_IO 8
#define PA_PIN 46
#endif

#define ADDR_TCA9554 0x20   // IO expander (AMOLED-1.8 only): EXIO0..2 = display / touch / etc. reset lines
#define ADDR_FT3168 0x38    // touch (original AMOLED-1.8 board)
#define ADDR_CST816 0x15    // touch (AMOLED-1.8 V2, LCD-1.83)
#define ADDR_QMI8658 0x6B   // IMU (0x6A on some revisions)
#define ADDR_PCF85063 0x51  // RTC
#define ADDR_AXP2101 0x34   // power manager (battery %)
