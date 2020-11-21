#include "Arduino.h"
#include "Raindrop.h"
#define NUM_LEDS 150
#define LOWER_HUE 140
#define UPPER_HUE 210

Raindrop::Raindrop() {
  pos = random(0, NUM_LEDS - 1);
  hue = random(LOWER_HUE, UPPER_HUE);
}

int Raindrop::updatePos() {
  pos--;
  if (pos < 0) {
    pos = random(NUM_LEDS,NUM_LEDS*2);
  }
  return pos;
}

int Raindrop::getHue(){
  return hue;
}
