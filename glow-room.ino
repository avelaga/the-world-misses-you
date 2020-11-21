#include <FastLED.h>
#include "Raindrop.h"
#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "milk steak with a side of jelly";
char currMode;
unsigned long timeSinceRequest = 0;

const int requestFreq = 1000;

#define NUM_LEDS 150
#define DATA1 2

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
#define DROP_HUE 180
#define DROP_DECREMENT 45;
#define DROP_DELAY 30
#define NUM_DROPS 5

Raindrop raindrops[NUM_DROPS];

float currHue = 0;
float inc = 0;
float hueInc = .03;
int noisePos = 0;
float incSpeed;

CRGB leds[NUM_LEDS];

void connectToNetwork() {
  WiFi.begin(ssid);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Establishing connection to WiFi..");
  }
  Serial.println("Connected to network");
}

void setup() {
  LEDS.addLeds<WS2812, DATA1, RGB>(leds, NUM_LEDS);
  LEDS.setBrightness(255);
  incSpeed = random(6, 9) / 100.0;

  Serial.begin(115200);
  connectToNetwork();
  Serial.println(WiFi.macAddress());
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.localIP());
  timeSinceRequest = millis();
}

void loop() {
  updateStatus();
  switch (currMode) {
    case '0':
      rainbow();
      break;
    case '1':
      twinkle();
      break;
    case '2':
      drop();
      break;
    default:
      clearLeds();
      break;
  }
  FastLED.show();
}

void updateStatus() {
  if (millis() - timeSinceRequest >= requestFreq) {
    timeSinceRequest = millis();
    if ((WiFi.status() == WL_CONNECTED)) { //Check the current connection status
      HTTPClient http;
      http.begin("http://192.168.4.1/status");
      int httpCode = http.GET();                                        //Make the request
      if (httpCode > 0) { //Check for the returning code
        currMode = http.getString().charAt(0);
        Serial.println(currMode);
      }
      else {
        Serial.println("Error on HTTP request");
      }
      http.end(); //Free the resources
    }
  }
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
  inc += hueInc;
  int currBrightness;
  for (int i = 0; i < NUM_LEDS; i++) {
    currBrightness = map(inoise8(i * 100, noisePos), 0, 255, -100, 255);
    if (currBrightness < 0) {
      currBrightness = 0;
    }
    leds[i] = CHSV(currHue, 255, currBrightness);
    currHue += .8; // incremenration of hues in the strip
  }
  noisePos += 1;
}

// OPTION 2
void twinkle() {
  twinkleDecrementBrightness();
  twinkleIncrementRandom();
  delay(10);
}

// OPTION 3
void drop() {
  for (int i = 0; i < NUM_LEDS; i++) {
    brightness[i] -= DROP_DECREMENT;
    if (brightness[i] < 0) {
      brightness[i] = 0;
    }
    leds[i] = CHSV(hue[i], 255, brightness[i]);
  }

  int currPos;
  int currHue;
  for (int dropIndex = 0; dropIndex < NUM_DROPS; dropIndex++) {
    currPos = raindrops[dropIndex].updatePos();
    if (currPos < NUM_LEDS) {
      currHue = raindrops[dropIndex].getHue();
      brightness[currPos] = 255;
      hue[currPos] = currHue;
      leds[currPos] = CHSV(currHue, 255, 255);
    }
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
