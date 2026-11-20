# Supported Hardware and Flashing

Lil Fella runs on three boards from Waveshare. The game is the same on all three, and comes out about the same physical size.

| Board | Screen |
| --- | --- |
| **ESP32-S3-Touch-AMOLED-2.16** | 480x480 AMOLED |
| **ESP32-S3-Touch-AMOLED-1.8 (V2)** | 368x448 AMOLED |
| **ESP32-S3-Touch-LCD-1.83** | 240x284 LCD |

The 2.16 and LCD-1.83 builds compile, but the project notes say nobody had watched them run on a board yet.

## Flashing a ready-built image

Ready-made images are in the project's `firmware` folder: one for each board (`lil-fella-v1.0.1-<board>-merged.bin`). Flash the right one at address 0.

**In a browser (Chrome or Edge):** open Espressif's ESP Tool web page, press Connect, add the file at address 0, and press Program.

**With the command line:** `esptool --chip esp32s3 write-flash 0x0` followed by the file name.

If the port is not found, hold BOOT, tap RESET, then let go of BOOT.

Flashing the wrong board's image does no harm. It just shows nothing useful, so flash the right one over it.

Saves live in the board's memory and survive re-flashing.

## Building it yourself

With VS Code and PlatformIO: open the project folder, pick the environment for your board, and click Upload. The default is the 2.16.

From the command line: `pio run -e amoled216 -t upload`, then `pio device monitor`. The other environments are `amoled18` and `lcd183`.
