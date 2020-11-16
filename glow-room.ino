#include <FastLED.h>

#define NUM_LEDS 50
#define DATA1 4

// twinkle vars
#define DECREMENT_BY 4
#define INC_BY 150
#define HUE_MIN 180
#define HUE_MAX 210
#define HUE 150
int brightness[NUM_LEDS];
int hue[NUM_LEDS];
int randomNumber;

// raindrop vars
#define DROP_HUE 200
#define DROP_DECREMENT 20;
#define DROP_FREQ 30
#define DROP_DELAY 20
int dropPosition = NUM_LEDS - 1;

float currHue = 0;
float inc = 0;
float hueInc = .5;
float incSpeed;

CRGB leds[NUM_LEDS];

void setup() {
  LEDS.addLeds<WS2812, DATA1, RGB>(leds, NUM_LEDS);
  LEDS.setBrightness(255);
  incSpeed = random(4, 6) / 100.0;
}

void loop() {
  rainbow();
//  twinkle();
//  drop();
  FastLED.show();
}

void clearLeds() {
  for (int x = 0; x < NUM_LEDS; x++) {
    brightness[x] = 0;
    leds[x] = CHSV(0, 0, 0);
    hue[x] = 0;
  }
}

// OPTION 1
void rainbow() {
  currHue = inc;
  inc += incSpeed;

  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(currHue, 255, 255);
    currHue += hueInc; // incremenration of hues in the strip
  }
}

// OPTION 2
void twinkle() {
  twinkleDecrementBrightness();
  twinkleIncrementRandom();
  delay(20);
}

// OPTION 3
void drop() {
  for (int i = 0; i < NUM_LEDS; i++) {
    brightness[i] -= DROP_DECREMENT;
    if (brightness[i] < 0) {
      brightness[i] = 0;
    }
    leds[i] = CHSV(DROP_HUE, 255, brightness[i]);
  }

  brightness[dropPosition] = 255;
  leds[dropPosition] = CHSV(DROP_HUE, 255, 255);
  dropPosition--;
  if (dropPosition < 0) {
    dropPosition = NUM_LEDS - 1;
  }
  delay(DROP_DELAY);
}

void twinkleDecrementBrightness() {
  for (int x = 0; x < NUM_LEDS; x++) {
    if (brightness[x]) {
      if (brightness[x] - DECREMENT_BY < 0) {
        brightness[x] = 0;
        leds[x] = CHSV(0, 0, 0);
      } else {
        brightness[x] -= DECREMENT_BY;
        leds[x] = CHSV(hue[x], 255, brightness[x]);
      }
    }
  }
}

void twinkleIncrementRandom() {
  randomNumber = random(0, NUM_LEDS - 1);
  brightness[randomNumber] += INC_BY;
  hue[randomNumber] = random(HUE_MIN, HUE_MAX);
  if (brightness[randomNumber] > 255) {
    brightness[randomNumber] = 255;
  }
  leds[randomNumber] = CHSV(hue[randomNumber], 255, brightness[randomNumber]);
}
