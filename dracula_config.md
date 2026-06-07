# Dracula Lyrics OLED Display
### Synthwave & Audio Visualizer — ESP32 + SSD1306

A word-by-word animated lyrics display with a live synthwave background. Concentric tunnel rings, jumping EQ bars, and shockwave blasts react to each lyric as it appears. Built for ESP32 with a 128×64 I2C OLED.

---

## Hardware Required

| Component | Details |
|---|---|
| ESP32 dev board | Any standard 38-pin ESP32 |
| SSD1306 OLED | 128×64, I2C, 3.3V |
| USB cable | Data cable (not charge-only) |
| Breadboard + jumper wires | Standard male-to-female |

---

## Wiring

The display uses I2C. Connect four wires from the OLED to the ESP32:

```
OLED        ESP32
────────────────────
VCC    →    3.3V
GND    →    GND
SDA    →    GPIO 21
SCL    →    GPIO 22
```

> **Note:** Do not connect VCC to 5V — the SSD1306 is a 3.3V device and will be damaged.

If your OLED module has a `RESET` pin, leave it unconnected (the code uses `-1` for reset, which disables it).

### I2C Address

The code uses address `0x3C`, which is the default for most SSD1306 modules. If your display doesn't initialise, it may be `0x3D`. You can scan for it with an I2C scanner sketch, or check the back of your OLED PCB for a solder bridge labelled `SA0`.

---

## Dependencies

Install these libraries via Arduino IDE → Library Manager:

| Library | Version tested |
|---|---|
| `Adafruit SSD1306` | 2.5.x |
| `Adafruit GFX Library` | 1.11.x |

Both are by Adafruit. Install `Adafruit GFX` first as `SSD1306` depends on it.

---

## Board Setup

1. In Arduino IDE go to **File → Preferences** and add this URL to *Additional Boards Manager URLs*:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
2. Go to **Tools → Board → Boards Manager**, search `esp32`, install the package by Espressif.
3. Select **Tools → Board → ESP32 Arduino → ESP32 Dev Module**.
4. Set **Tools → Upload Speed → 115200** (avoids `termios` errors on macOS).
5. Set **Tools → Partition Scheme → Default** (sketch is ~25% of flash, no changes needed).

---

## How It Works

### Lyrics Timeline

Each word is defined as a `Lyric` struct with four properties:

```cpp
{ delayBeforeMs, durationMs, "word", EFFECT, textSize }
```

- `delayBeforeMs` — pause before this word appears (used for gaps between phrases)
- `durationMs` — how long the word stays on screen
- `EFFECT` — animation applied while the word is displayed
- `textSize` — Adafruit GFX text size (1–3); size 3 is ~24px tall

The sketch pre-calculates an absolute `calculatedStartTime` for every word at boot, then drives everything off `millis()`.

### Effects

| Effect | Behaviour |
|---|---|
| `EFFECT_NONE` | Plain text, no animation |
| `EFFECT_POP` | Text jumps one size larger for the first 15% of its duration, then snaps back |
| `EFFECT_SHAKE` | Random ±3px jitter on X and Y every frame |
| `EFFECT_INVERT` | Full screen colour inversion + subtle jitter |
| `EFFECT_GLITCH` | Random horizontal scan-line bars, occasional inversion, position jitter |
| `EFFECT_ZOOM_IN` | Text grows from size−1 → size → size+1 over the word's duration; shakes and flickers toward the end |

### Background Layers

Three layers are drawn bottom-to-top before the lyric text:

**1. Tunnel** — 7 concentric circles expand outward from the screen centre and wrap back to radius 1 when they exceed 120px. 8 radial lines give the perspective illusion. Speed scales with effect intensity:

| Condition | Speed |
|---|---|
| Idle (no lyric) | 0.5 px/frame |
| Normal lyric | 1.0 px/frame |
| SHAKE / GLITCH | 4.0 px/frame |
| ZOOM_IN (the drop) | 8.0 px/frame |

**2. EQ Bars** — 16 bars across the full width. Target heights are randomised each frame and smoothed with a simple easing (`+4` toward target, `−3` away). Heights are capped based on the current effect.

**3. Shockwave** — a double-ring circle that expands from radius 5 to 140 at 8px/frame. Triggered on any word with `EFFECT_POP`, `EFFECT_INVERT`, `EFFECT_ZOOM_IN`, or `EFFECT_SHAKE`.

### Lyric Rendering

Text is centred using `getTextBounds()`. A filled rounded rectangle with a white border is drawn behind the text to keep it readable against the busy background. If the text would overflow the screen width (e.g. `DRACULA` at size 4), it automatically falls back to size 2.

---

## Customising the Lyrics

To change the song, edit the `lyrics[]` array. Each line follows this pattern:

```cpp
{ delayBefore, duration, "WORD", EFFECT_TYPE, textSize, 0 },
```

The last `0` is `calculatedStartTime` — always leave it as `0`, it is filled in at boot.

**Timing tips:**
- A normal spoken syllable is around 150–250ms
- Emphasis words work well at 400–800ms
- Gaps between phrases use `delayBeforeMs` on the first word of the next phrase
- The total loop time is calculated automatically — no need to update it manually

**Example phrase:**
```cpp
{ 300, 400, "Hello",   EFFECT_POP,    2, 0 },
{ 0,   300, "from",    EFFECT_NONE,   2, 0 },
{ 0,   800, "EARTH",   EFFECT_ZOOM_IN, 3, 0 },
```

---

## Troubleshooting

**Display stays blank after upload**
- Run an I2C scanner sketch and confirm the OLED appears at `0x3C` (or change the address in the code to `0x3D`)
- Check SDA/SCL are not swapped
- Confirm you are on 3.3V not 5V

**Upload fails with `termios.error: (22, Invalid argument)`**
- Lower upload speed to 115200 in Tools → Upload Speed
- Hold the BOOT button on the ESP32 when the IDE starts connecting, release once upload begins
- Try a different USB cable — many cables are charge-only

**Text is off-centre or clipped**
- Increase `durationMs` so the word has time to render fully
- If a long word clips, reduce its `textSize` from 3 to 2

**Loop timing drifts over time**
- The sketch uses `millis()` which is accurate to ±1ms. For very long songs (> ~10 minutes) integer overflow is not a concern as `millis()` rolls over safely via unsigned arithmetic.

---

## File Structure

```
dracula-oled/
└── dracula-oled.ino    — full sketch, single file
```

No additional header files or assets required.
