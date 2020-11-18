#include "Arduino.h"
#include "Raindrop.h"
#define NUM_LEDS 150

Raindrop::Raindrop() {
  pos = random(0, NUM_LEDS - 1);
}

int Raindrop::updatePos() {
  pos--;
  if (pos < 0) {
    pos = NUM_LEDS;
  }
  return pos;
}
