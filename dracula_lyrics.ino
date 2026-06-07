/*
 * Dracula Lyrics OLED Display (ESP8266 + SSD1306) - SYNTHWAVE & AUDIO VISUALIZER THEME
 * 
 * This version completely changes the vibe! Instead of a simple starfield, it features
 * a highly dynamic "Music Visualizer / Cyberpunk" abstract background:
 * 1. 3D Wireframe Tunnel: Concentric rings and radiating lines simulate flying through a wormhole.
 * 2. Audio EQ Bars: A jumping frequency visualizer at the bottom of the screen.
 * 3. Shockwaves: A massive circular blast occurs whenever an impactful word appears.
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1  
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- Background System Variables ---
// 1. Tunnel
#define NUM_CIRCLES 7
float circleRadii[NUM_CIRCLES];

// 2. Audio Visualizer EQ
#define NUM_BARS 16
int barHeights[NUM_BARS];
int targetHeights[NUM_BARS];

// 3. Shockwave
struct Shockwave {
  float radius;
  bool active;
} wave;

int lastLyricIdx = -1; // To detect when a NEW lyric appears

// --- Lyric Effects Enum ---
enum Effect {
  EFFECT_NONE = 0,
  EFFECT_POP,     // Briefly enlarges word
  EFFECT_SHAKE,   // Earth-quake shake effect
  EFFECT_INVERT,  // Inverts the whole screen for impact
  EFFECT_GLITCH,  // Adds random lines and jitters
  EFFECT_ZOOM_IN  // Starts small and zooms to large
};

// --- Lyric Data Structure ---
struct Lyric {
  unsigned long delayBeforeMs;        
  unsigned long durationMs;           
  const char* word;                   
  Effect effect;                      
  uint8_t size;                       
  unsigned long calculatedStartTime;  
};

// =========================================================================
// THE LYRICS TIMELINE
// =========================================================================
Lyric lyrics[] = {
  // Phrase 1 (Wait 1000ms before starting)
  { 0, 300, "The", EFFECT_NONE, 2, 0 },
  { 0, 300, "morning", EFFECT_POP, 2, 0 },
  { 0, 300, "light", EFFECT_NONE, 2, 0 },
  { 0, 300, "is", EFFECT_NONE, 2, 0 },
  { 0, 300, "turning", EFFECT_NONE, 2, 0 },
  { 100, 500, "BLUE", EFFECT_INVERT, 3, 0 }, 

  // Phrase 2 (Wait 200ms before starting 'the')
  { 0, 300, "the", EFFECT_NONE, 2, 0 },
  { 0, 300, "feeling", EFFECT_POP, 2, 0 },
  { 0, 300, "is", EFFECT_NONE, 2, 0 },
  { 0, 800, "BIZARRE", EFFECT_GLITCH, 2, 0 },

  // Phrase 3 (Wait 300ms before starting 'The')
  { 200, 300, "The", EFFECT_NONE, 2, 0 },
  { 0, 300, "night", EFFECT_POP, 2, 0 },
  { 0, 300, "is", EFFECT_NONE, 2, 0 },
  { 0, 300, "almost", EFFECT_NONE, 2, 0 },
  { 0, 500, "over,", EFFECT_NONE, 2, 0 },

  // Phrase 4
  { 200, 300, "I", EFFECT_NONE, 2, 0 },
  { 0, 300, "still", EFFECT_POP, 2, 0 },
  { 0, 300, "don't", EFFECT_NONE, 2, 0 },
  { 0, 300, "know", EFFECT_NONE, 2, 0 },
  { 0, 200, "where", EFFECT_NONE, 2, 0 },
  { 0, 200, "you", EFFECT_NONE, 2, 0 },
  { 0, 600, "ARE", EFFECT_SHAKE, 3, 0 },

  // Phrase 5
  { 200, 300, "The", EFFECT_NONE, 2, 0 },
  { 0, 400, "shadows,", EFFECT_SHAKE, 2, 0 },
  { 0, 300, "yeah,", EFFECT_NONE, 2, 0 },
  { 0, 300, "they", EFFECT_NONE, 2, 0 },
  { 0, 300, "keep", EFFECT_NONE, 2, 0 },
  { 0, 300, "me", EFFECT_NONE, 2, 0 },
  { 0, 300, "pretty", EFFECT_POP, 2, 0 },
  { 0, 300, "like", EFFECT_NONE, 2, 0 },
  { 0, 300, "a", EFFECT_NONE, 2, 0 },
  { 0, 300, "movie", EFFECT_NONE, 2, 0 },
  { 0, 800, "STAR", EFFECT_INVERT, 3, 0 },

  // Phrase 6
  { 300, 500, "Daylight", EFFECT_POP, 2, 0 },
  { 0, 300, "makes", EFFECT_NONE, 2, 0 },
  { 0, 300, "me", EFFECT_NONE, 2, 0 },
  { 0, 300, "feel", EFFECT_NONE, 2, 0 },
  { 0, 200, "like", EFFECT_NONE, 2, 0 },

  // THE DROP
  { 100, 2500, "DRACULA", EFFECT_ZOOM_IN, 3, 0 }
};
const int numLyrics = sizeof(lyrics) / sizeof(Lyric);

unsigned long startTime = 0;
bool isPlaying = true;
unsigned long TOTAL_LOOP_TIME = 0;  

void setup() {
  Serial.begin(115200);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for (;;);
  }

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextWrap(false);

  // Init Tunnel
  for(int i=0; i<NUM_CIRCLES; i++) {
    circleRadii[i] = i * (100.0 / NUM_CIRCLES);
  }

  // Init EQ
  for(int i=0; i<NUM_BARS; i++) {
    barHeights[i] = 0;
    targetHeights[i] = 0;
  }
  wave.active = false;

  unsigned long runningTime = 0;
  for (int i = 0; i < numLyrics; i++) {
    runningTime += lyrics[i].delayBeforeMs;
    lyrics[i].calculatedStartTime = runningTime;
    runningTime += lyrics[i].durationMs;
  }
  TOTAL_LOOP_TIME = runningTime + 3000;

  startTime = millis();
}

void loop() {
  if (!isPlaying) return;

  unsigned long now = millis() - startTime;
  if (now > TOTAL_LOOP_TIME) {
    startTime = millis();
    now = 0;
    lastLyricIdx = -1;
  }

  display.clearDisplay();

  int currentLyricIdx = -1;
  for (int i = 0; i < numLyrics; i++) {
    if (now >= lyrics[i].calculatedStartTime && now <= (lyrics[i].calculatedStartTime + lyrics[i].durationMs)) {
      currentLyricIdx = i;
      break;
    }
  }

  Effect currentEffect = EFFECT_NONE;
  bool lyricActive = false;
  float progress = 0.0;
  bool invertScreen = false;

  // Track state
  if (currentLyricIdx != -1) {
    lyricActive = true;
    const Lyric& l = lyrics[currentLyricIdx];
    currentEffect = l.effect;
    float elapsedLyric = now - l.calculatedStartTime;
    progress = elapsedLyric / (float)l.durationMs;

    // Check for NEW word to trigger shockwave
    if (currentLyricIdx != lastLyricIdx) {
      if (l.effect == EFFECT_POP || l.effect == EFFECT_INVERT || l.effect == EFFECT_ZOOM_IN || l.effect == EFFECT_SHAKE) {
        wave.active = true;
        wave.radius = 5.0;
      }
      lastLyricIdx = currentLyricIdx;
    }
  } else {
    lastLyricIdx = -1; // reset when empty space
  }

  // 1. DRAW BACKGROUND TUNNEL & EQ FIRST
  drawTunnel(currentEffect, lyricActive);
  drawShockwave();
  drawEQ(currentEffect, lyricActive);

  // 2. DRAW LYRICS ON TOP
  if (currentLyricIdx != -1) {
    const Lyric& l = lyrics[currentLyricIdx];
    
    int textSize = l.size;
    int offsetX = 0;
    int offsetY = 0;

    // Apply Effects
    if (l.effect == EFFECT_POP) {
      if (progress < 0.15) textSize = l.size + 1;
    } else if (l.effect == EFFECT_SHAKE) {
      offsetX = random(-3, 4);
      offsetY = random(-3, 4);
    } else if (l.effect == EFFECT_INVERT) {
      invertScreen = true;
      if (random(10) > 5) { offsetX = random(-2, 3); offsetY = random(-2, 3); }
    } else if (l.effect == EFFECT_GLITCH) {
      if (random(10) > 6) {
        offsetX = random(-6, 6);
        // Draw glitch static over background
        display.fillRect(0, random(SCREEN_HEIGHT), SCREEN_WIDTH, random(2, 8), WHITE);
      }
      if (random(10) > 8) invertScreen = true;
    } else if (l.effect == EFFECT_ZOOM_IN) {
      if (progress < 0.05) textSize = l.size > 1 ? l.size - 1 : 1;
      else if (progress < 0.1) textSize = l.size;
      else textSize = l.size + 1;

      if (progress > 0.4) {
        offsetX = random(-4, 5);
        offsetY = random(-4, 5);
      }
      if (progress > 0.3 && random(10) > 7) invertScreen = true;
    }

    display.setTextSize(textSize);
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(l.word, 0, 0, &x1, &y1, &w, &h);
    
    // Safety fallback: if text somehow is too large, reset to size 2
    if (w > SCREEN_WIDTH) {
        textSize = 2;
        display.setTextSize(textSize);
        display.getTextBounds(l.word, 0, 0, &x1, &y1, &w, &h);
    }

    int drawX = (SCREEN_WIDTH - w) / 2 + offsetX;
    int drawY = (SCREEN_HEIGHT - h) / 2 + offsetY - 5; // Shifted up slightly to avoid EQ bars

    // Draw a sleek text box behind the text to make it extremely readable
    display.fillRoundRect(drawX - 4, drawY - 4, w + 8, h + 8, 3, BLACK);
    display.drawRoundRect(drawX - 4, drawY - 4, w + 8, h + 8, 3, WHITE);

    if (l.effect == EFFECT_GLITCH && random(10) > 8) {
      display.setTextColor(BLACK, WHITE);
      display.fillRoundRect(drawX - 2, drawY - 2, w + 4, h + 4, 3, WHITE); // White background glitch
    } else {
      display.setTextColor(WHITE);
    }

    display.setCursor(drawX, drawY);
    display.print(l.word);
  }

  display.invertDisplay(invertScreen);
  display.display();
}

// ----------------------------------------------------
// Abstract Theme Functions
// ----------------------------------------------------

void drawTunnel(Effect currentEffect, bool lyricActive) {
  float speed = 1.0;
  if (currentEffect == EFFECT_ZOOM_IN) speed = 8.0; // Hyper speed tunnel
  else if (currentEffect == EFFECT_SHAKE || currentEffect == EFFECT_GLITCH) speed = 4.0;
  else if (!lyricActive) speed = 0.5; // Idle
  
  int centerX = SCREEN_WIDTH/2;
  int centerY = SCREEN_HEIGHT/2;
  
  if (currentEffect == EFFECT_SHAKE || currentEffect == EFFECT_ZOOM_IN) {
    centerX += random(-3, 4);
    centerY += random(-3, 4);
  }

  // Draw Radiating Lines (Tunnel Walls)
  for (float angle = 0; angle < 2 * PI; angle += PI / 4) {
    int x1 = centerX + cos(angle) * 5; 
    int y1 = centerY + sin(angle) * 5;
    int x2 = centerX + cos(angle) * 120;
    int y2 = centerY + sin(angle) * 120;
    display.drawLine(x1, y1, x2, y2, WHITE);
  }

  // Draw Concentric Circles moving outward
  for(int i=0; i<NUM_CIRCLES; i++) {
    circleRadii[i] += speed;
    if (circleRadii[i] > 120) circleRadii[i] = 1; // Reset at center
    
    if (circleRadii[i] > 2) {
      display.drawCircle(centerX, centerY, circleRadii[i], WHITE);
    }
  }
}

void drawEQ(Effect currentEffect, bool lyricActive) {
  int barWidth = SCREEN_WIDTH / NUM_BARS;
  
  for (int i = 0; i < NUM_BARS; i++) {
    // Update target heights based on music intensity
    if (random(10) > 4) { 
      if (lyricActive) {
        if (currentEffect == EFFECT_ZOOM_IN) {
          targetHeights[i] = random(15, 35); // Massive EQ spikes during drop
        } else if (currentEffect == EFFECT_SHAKE) {
          targetHeights[i] = random(10, 25);
        } else {
          targetHeights[i] = random(5, 15);
        }
      } else {
        targetHeights[i] = random(1, 6); // Very low when idle
      }
    }
    
    // Smooth interpolation
    if (barHeights[i] < targetHeights[i]) barHeights[i] += 4;
    else if (barHeights[i] > targetHeights[i]) barHeights[i] -= 3;
    
    // Keep in bounds
    if (barHeights[i] < 1) barHeights[i] = 1;
    
    // Draw bar at the very bottom
    display.fillRect(i * barWidth + 1, SCREEN_HEIGHT - barHeights[i], barWidth - 1, barHeights[i], WHITE);
  }
}

void drawShockwave() {
  if (wave.active) {
    display.drawCircle(SCREEN_WIDTH/2, SCREEN_HEIGHT/2, wave.radius, WHITE);
    // Draw a slightly smaller circle to make the shockwave thicker
    display.drawCircle(SCREEN_WIDTH/2, SCREEN_HEIGHT/2, wave.radius - 1, WHITE);
    
    wave.radius += 8.0; // Fast expand
    if (wave.radius > 140) wave.active = false;
  }
}
