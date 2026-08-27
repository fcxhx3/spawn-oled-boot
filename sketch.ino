#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// One star. z is depth, smaller means closer to the viewer.
struct Star {
  int8_t x, y;
  uint8_t z;
};

const int NUM_STARS = 25;
Star stars[NUM_STARS];
unsigned long startTime;

void setup() {
  Serial.begin(9600);

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  for (int i = 0; i < NUM_STARS; i++) {
    stars[i].x = random(-64, 64);
    stars[i].y = random(-32, 32);
    stars[i].z = random(1, 64);
  }

  display.clearDisplay();
  startTime = millis();
}

void loop() {
  display.clearDisplay();

  unsigned long elapsed = millis() - startTime;

  if (elapsed < 3500) {
    // Sparkle grows in first, then the logo appears next to it.
    updateStars(2);

    if (elapsed < 800) {
      int sparkleSize = map(elapsed, 0, 800, 0, 6);
      drawSparkle(72, 18, sparkleSize);
    } else {
      drawSparkle(72, 18, 5);
      drawSpawnLogo(60, 32, 10);
    }

  } else if (elapsed < 7000) {
    // Wordmark, stars still drifting behind it.
    updateStars(2);
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(16, 25);
    display.println(F("SPAWN.CO"));

  } else if (elapsed < 8000) {
    // Speed the stars up for a second before looping.
    updateStars(7);

  } else {
    startTime = millis();
  }

  display.display();
  delay(20);
}

// Four points, built from two triangles per axis.
void drawSparkle(int cx, int cy, int r) {
  if (r <= 0) return;

  int w = (r > 2) ? r / 2 : 1;

  display.fillTriangle(cx, cy - r, cx - w, cy, cx + w, cy, SSD1306_WHITE);
  display.fillTriangle(cx, cy + r, cx - w, cy, cx + w, cy, SSD1306_WHITE);
  display.fillTriangle(cx - r, cy, cx, cy - w, cx, cy + w, SSD1306_WHITE);
  display.fillTriangle(cx + r, cy, cx, cy - w, cx, cy + w, SSD1306_WHITE);
}

// Filled half circles, drawn a scanline at a time.
void drawLeftHalfCircle(int cx, int cy, int r) {
  for (int y = -r; y <= r; y++) {
    int w = round(sqrt(r * r - y * y));
    display.drawFastHLine(cx - w, cy + y, w, SSD1306_WHITE);
  }
}

void drawRightHalfCircle(int cx, int cy, int r) {
  for (int y = -r; y <= r; y++) {
    int w = round(sqrt(r * r - y * y));
    display.drawFastHLine(cx, cy + y, w, SSD1306_WHITE);
  }
}

// Two offset half circles, so they read as interlocking crescents.
void drawSpawnLogo(int cx, int cy, int radius) {
  drawLeftHalfCircle(cx - 2, cy - 6, radius);
  drawRightHalfCircle(cx + 2, cy + 6, radius);
}

// Step every star towards the viewer, project it, and plot it.
void updateStars(int speed) {
  for (int i = 0; i < NUM_STARS; i++) {
    // z is unsigned, so respawn before subtracting. Subtracting first and
    // testing for zero afterwards lets it wrap round to 255 instead.
    if (stars[i].z <= speed) {
      stars[i].x = random(-64, 64);
      stars[i].y = random(-32, 32);
      stars[i].z = 64;
    } else {
      stars[i].z -= speed;
    }

    int px = (stars[i].x * 64) / stars[i].z + 64;
    int py = (stars[i].y * 32) / stars[i].z + 32;

    if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
      display.drawPixel(px, py, SSD1306_WHITE);
    }
  }
}
