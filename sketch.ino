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

// Star speed is carried in tenths so it can be ramped smoothly. Whatever is
// left over after moving a whole step is kept here for the next frame.
const int SPEED_DRIFT = 10;
const int SPEED_PEAK = 10;
int speedRemainder = 0;

// Logo artwork, kept in flash rather than RAM. A 2 marks the star, a 1 marks
// the rest of the mark, which is what lets them be drawn separately.
const uint8_t LOGO_WIDTH = 19;
const uint8_t LOGO_HEIGHT = 22;

const uint8_t LOGO_X = 49;
const uint8_t LOGO_Y = 15;
const uint8_t LOGO_DRAW_W = 29;
const uint8_t LOGO_DRAW_H = 33;

// Middle of the star, in the scaled up drawing, so it can grow from there.
const uint8_t STAR_CX = 21;
const uint8_t STAR_CY = 6;

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

  if (elapsed < 1000) {
    // Open on the starfield alone.
    updateStars(SPEED_DRIFT);

  } else if (elapsed < 1800) {
    // The star out of the logo grows in first.
    updateStars(SPEED_DRIFT);
    int scale = map(elapsed, 1000, 1800, 0, 100);
    drawLogoPart(2, scale);

  } else if (elapsed < 3500) {
    // Then the rest of the mark joins it.
    updateStars(SPEED_DRIFT);
    drawLogoPart(2, 100);
    drawLogoPart(1, 100);

  } else if (elapsed < 7000) {
    // Wordmark.
    updateStars(SPEED_DRIFT);
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(16, 25);
    display.println(F("SPAWN.CO"));

  } else if (elapsed < 7800) {
    // Pick the stars up gently.
    updateStars(map(elapsed, 7000, 7800, SPEED_DRIFT, SPEED_PEAK));

  } else if (elapsed < 9200) {
    // Then settle them back to the drifting speed, so the loop point matches
    // how the sequence opens and the restart does not show.
    updateStars(map(elapsed, 7800, 9200, SPEED_PEAK, SPEED_DRIFT));

  } else {
    startTime = millis();
  }

  display.display();
  delay(20);
}

// Draws the cells holding one particular value, scaled from the 19x22 artwork
// up to 29x33 by nearest neighbour. A scale below 100 pulls the shape in
// towards the star, so it can be grown outwards from a point.
void drawLogoPart(uint8_t want, int scale) {
  for (int py = 0; py < LOGO_DRAW_H; py++) {
    for (int px = 0; px < LOGO_DRAW_W; px++) {
      int srcC = (px * LOGO_WIDTH) / LOGO_DRAW_W;
      int srcR = (py * LOGO_HEIGHT) / LOGO_DRAW_H;

      if (pgm_read_byte(&(SPAWN_LOGO[srcR][srcC])) != want) {
        continue;
      }

      int dx = px;
      int dy = py;

      if (scale < 100) {
        dx = STAR_CX + ((px - STAR_CX) * scale) / 100;
        dy = STAR_CY + ((py - STAR_CY) * scale) / 100;
      }

      display.drawPixel(LOGO_X + dx, LOGO_Y + dy, SSD1306_WHITE);
    }
  }
}

// Step every star towards the viewer, project it, and plot it. Speed arrives
// in tenths, so a frame may move the field by a whole step or not at all.
void updateStars(int speedTenths) {
  speedRemainder += speedTenths;
  int step = speedRemainder / 10;
  speedRemainder -= step * 10;

  for (int i = 0; i < NUM_STARS; i++) {
    stars[i].z -= step;

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
