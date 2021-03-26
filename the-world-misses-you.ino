// (c) 2020 abhi velaga
// abhi.work/software
// instagram.com/abhi.velaga

#include <FastLED.h>
#include "Raindrop.h"
#include <WiFi.h>
#include "time.h"
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

TaskHandle_t networkTask;
AsyncWebServer server(80);

const char* PARAM_INPUT_1 = "input1";
const char* PARAM_INPUT_2 = "input2";

String mySsid = "LEDSSSSSS";
bool apMode = true;


// HTML web page to handle 2 input fields (input1, input2)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>ESP Input Form</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  </head><body>
  <h1>
  <form action="/get">
    wifi ssid: <input type="text" name="input1"><br/>
    password: <input type="text" name="input2"> <br/>
    <input type="submit" value="Submit">
  </form><br></h1>
</body></html>)rawliteral";

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

enum Conditions {
  day, night, off, sunny, cloudy, rainy
};

Conditions weather = sunny;
Conditions timeOfDay = day;

String ssid     = "";
String password = "";

const char* serverName = "http://api.weatherapi.com/v1/current.json?key=125ccd14d85a40e49f3224915201911&q=Austin";
String weatherStr = "";
String response;

//char currMode = '0';
unsigned long timeSinceRequest = 0;

const int requestFreq = 900000; // every 15 mins

// time things
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -21600; // for CDT
const int   daylightOffset_sec = 3600;
char currHour[3];

#define NUM_LEDS 150
#define DATA1 2

// twinkle vars
#define DECREMENT_BY 2
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

// rainbow vars
float currHue = 0;
float inc = 0;
float hueInc = .03;
int noisePos = 0;
float incSpeed;

CRGB leds[NUM_LEDS];

// switch to station mode and connect to wifi with submitted parameters
void connectToNetwork() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Establishing connection to WiFi..");
  }
  Serial.println("Connected to network");
  apMode = false;

  Serial.println(WiFi.macAddress());
  Serial.println(WiFi.localIP());
  timeSinceRequest = millis();

  // Initialize time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  getCurrTime();
  getWeatherCondition();
  updateEnums();

  xTaskCreatePinnedToCore(
    updateConditions,   /* Task function. */
    "networkTask",     /* name of task. */
    40000,       /* Stack size of task */
    NULL,        /* parameter of the task */
    2,           /* priority of the task */
    &networkTask,      /* Task handle to keep track of created task */
    0);          /* pin task to core 0 */
  delay(500);
}

// setup access point, then deploy config page
void AP() {
  Serial.print("Setting AP (Access Point)…");
  WiFi.softAP(mySsid.c_str());
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  server.begin();
  configPage();
}

// deploy server to handle config page
void configPage() {
  // Send web page with input fields to client
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/html", index_html);
  });

  // Send a GET request to <ESP_IP>/get?input1=<inputMessage>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest * request) {
    String p1;
    String p2;
    // GET input1 value on <ESP_IP>/get?input1=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1)) {
      ssid = request->getParam(PARAM_INPUT_1)->value();
      p1 = PARAM_INPUT_1;
    }
    // GET input2 value on <ESP_IP>/get?input2=<inputMessage>
    if (request->hasParam(PARAM_INPUT_2)) {
      password = request->getParam(PARAM_INPUT_2)->value();
      p2 = PARAM_INPUT_2;
    }

    Serial.println(ssid);
    Serial.println(password);
    request->send(200, "text/html", "<h1>Connecting to " + ssid + "</h1>");
    connectToNetwork(); // PARAMS SUBMITTED! switch to station mode and connect to wifi with them
  });
  server.onNotFound(notFound);
  server.begin();
}

void setup() {
  LEDS.addLeds<WS2812, DATA1, RGB>(leds, NUM_LEDS);
  LEDS.setBrightness(255);
  incSpeed = random(6, 9) / 100.0;

  Serial.begin(115200);



  //  connectToNetwork();
  AP();


}

void loop() {
  if (!apMode) {
    switch (timeOfDay) {
      case day:
        switch (weather) {
          case sunny:
            rainbow();
            break;
          case rainy:
            drop();
            break;
          case cloudy:
            twinkle();
            break;
          default:
            clearLeds();
            break;
        }
        break;
      case night:
        twinkle();
        break;
      case off:
        clearLeds();
        break;
      default:
        clearLeds();
        break;
    }
    FastLED.show();
  }
}

// helper for getWeatherCondition()
String weatherAPIRequest(const char* serverName) {
  HTTPClient http;
  http.begin(serverName);
  int httpResponseCode = http.GET();

  String payload = "{}";

  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }

  http.end();
  return payload;
}

// sets weather string to latest condition
void getWeatherCondition() {
  if (WiFi.status() == WL_CONNECTED) {
    response = weatherAPIRequest(serverName);
    Serial.println(response); // prints full correct body
    JSONVar myObject = JSON.parse(response);

    if (JSON.typeof(myObject) == "undefined") {
      Serial.println("Parsing input failed!");
      return;
    }

    JSONVar current = myObject["current"];
    JSONVar condition = current["condition"];
    JSONVar text = condition["text"];
    weatherStr = JSON.stringify(text);
    Serial.println(weatherStr);
  }
  else {
    Serial.println("WiFi Disconnected");
  }
}

void getCurrTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }

  Serial.println("the hour is:");
  strftime(currHour, 3, "%H", &timeinfo);
  Serial.println(currHour);
  Serial.println();
}

// continuously update weatherStr and time variables, replaces updateStatus
// this function updates weatherStr and currHour every 15 mins
void updateConditions(void * pvParameters ) {
  while (true) {
    if (!apMode) {
      if (WiFi.status() != WL_CONNECTED) {
        while (WiFi.status() != WL_CONNECTED) {
          WiFi.begin(ssid.c_str(), password.c_str());
          delay(1000);
          Serial.println("Trying to recconect to wifi");
        }
      }
      vTaskDelay(requestFreq);
      timeSinceRequest = millis();
      getWeatherCondition();
      getCurrTime();
      updateEnums();
    }
  }
}

// look at currHour[3] and weatherStr to set current enums
void updateEnums() {

  // convert to int
  int currHourInt = 10 * (currHour[0] - '0') + (currHour[1] - '0');

  // day time
  if ((currHourInt >= 10) && (currHourInt <= 17)) {
    timeOfDay = day;
  }
  // night time, ends at midnight
  else if (currHourInt >= 18) {
    timeOfDay = night;
  }
  else {
    // midnight to morning
    timeOfDay = off;
  }

  weatherStr.toLowerCase();
  if (weatherStr.indexOf("rain") >= 0) {
    weather = rainy;
  }
  else if (weatherStr.indexOf("cloud") >= 0 || weatherStr.indexOf("overcast") >= 0) {
    weather = cloudy;
  } else {
    weather = sunny;
  }


}

void clearLeds() {
  for (int x = 0; x < NUM_LEDS; x++) {
    brightness[x] = 0;
    leds[x] = CHSV(0, 0, 0);
    hue[x] = 0;
  }
  delay(15); // prevents esp from programming leds too fast and failing
}

// LED OPTION 1
void rainbow() {
  currHue = inc;
  inc += hueInc;
  int currBrightness;
  for (int i = 0; i < NUM_LEDS; i++) {
    currBrightness = map(inoise8(i * 100, noisePos), 0, 255, -100, 255);
    if (currBrightness < 0) {
      currBrightness = 0;
    }
    hue[i] = currHue;
    brightness[i] = currBrightness;
    //    if (currBrightness > 100) { // allows for  abetter transition
    leds[i] = CHSV(currHue, 255, currBrightness);
    //    }
    currHue += .8; // incremenration of hues in the strip
  }
  noisePos += 1;
}

// LED OPTION 2
void twinkle() {
  twinkleDecrementBrightness();
  twinkleIncrementRandom();
  delay(15); // prevents esp from programming leds too fast and failing
}

// LED OPTION 3
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

// twinkle helper 1
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

// twinkle helper 2
void twinkleIncrementRandom() {
  randomNumber = random(0, NUM_LEDS - 1);
  brightness[randomNumber] += INC_BY;
  hue[randomNumber] = random(HUE_MIN, HUE_MAX);
  if (brightness[randomNumber] > 255) {
    brightness[randomNumber] = 255;
  }
  leds[randomNumber] = CHSV(hue[randomNumber], 255, brightness[randomNumber]);
}
