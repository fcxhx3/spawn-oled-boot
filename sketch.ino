#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// One star. z is depth and has to be signed, otherwise it wraps instead of
// hitting the reset below.
struct Star {
  int8_t x, y;
  int16_t z;
};

const int NUM_STARS = 25;
Star stars[NUM_STARS];
unsigned long startTime;

// Logo artwork, kept in flash rather than RAM.
const uint8_t LOGO_WIDTH = 19;
const uint8_t LOGO_HEIGHT = 22;

const uint8_t SPAWN_LOGO[22][19] PROGMEM = {
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,2,2,2,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,2,2,2,2,2,0,0},
  {0,0,0,0,1,1,0,0,0,0,2,2,2,2,2,2,2,2,2},
  {0,0,1,1,1,1,0,0,0,0,0,0,2,2,2,2,2,0,0},
  {0,1,1,1,1,1,0,0,0,0,0,0,0,2,2,2,0,0,0},
  {0,1,1,1,1,1,0,0,0,0,0,0,0,0,2,0,0,0,0},
  {1,1,1,1,1,1,0,0,0,0,0,0,0,0,2,0,0,0,0},
  {1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {1,1,1,1,1,1,0,0,0,1,1,0,0,0,0,0,0,0,0},
  {1,1,1,1,1,1,0,0,0,1,1,1,1,0,0,0,0,0,0},
  {0,1,1,1,1,1,0,0,0,1,1,1,1,1,0,0,0,0,0},
  {0,1,1,1,1,1,0,0,0,1,1,1,1,1,0,0,0,0,0},
  {0,0,1,1,1,1,0,0,0,1,1,1,1,1,1,0,0,0,0},
  {0,0,0,0,1,1,0,0,0,1,1,1,1,1,1,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0}
};

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
    // Logo on its own, stars drifting behind it.
    updateStars(2);
    drawSpawnMatrixLogoSolid(49, 15);

  } else if (elapsed < 7000) {
    // Wordmark.
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

// Scales the 19x22 artwork up to 29x33 by nearest neighbour. Any non zero
// value in the matrix is drawn, so the shape comes out filled.
void drawSpawnMatrixLogoSolid(int x, int y) {
  int targetW = 29;
  int targetH = 33;

  for (int py = 0; py < targetH; py++) {
    for (int px = 0; px < targetW; px++) {
      int srcC = (px * LOGO_WIDTH) / targetW;
      int srcR = (py * LOGO_HEIGHT) / targetH;

      uint8_t pixelVal = pgm_read_byte(&(SPAWN_LOGO[srcR][srcC]));
      if (pixelVal > 0) {
        display.drawPixel(x + px, y + py, SSD1306_WHITE);
      }
    }
  }
}

// Step every star towards the viewer, project it, and plot it.
void updateStars(int speed) {
  for (int i = 0; i < NUM_STARS; i++) {
    stars[i].z -= speed;

    if (stars[i].z <= 0) {
      stars[i].x = random(-64, 64);
      stars[i].y = random(-32, 32);
      stars[i].z = 64;
    }

    int px = (stars[i].x * 64) / stars[i].z + 64;
    int py = (stars[i].y * 32) / stars[i].z + 32;

    if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
      display.drawPixel(px, py, SSD1306_WHITE);
    }
  }
}
