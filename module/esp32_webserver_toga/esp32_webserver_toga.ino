/* Board: ESP32 Dev Module
   Upload Speed: 921600
*/

#include <FS.h>
#include <SPIFFS.h>
#include <esp_now.h>

#include <FastLED.h>
#include <fl/leds.h> // gfx
#include <fl/xymap.h> // math

#include <AsyncWiFiMulti.h>

using namespace fl;
using namespace GuLinux;

#include <WebServer.h>
#include <ESPmDNS.h>
#include "globals.h"

WebServer webServer(80);

#define DATA_PIN 4
//#define LED_TYPE WS2811
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
// #define NUM_LEDS    50
constexpr uint16_t WIDTH = 32;
constexpr uint16_t HEIGHT = 8;
constexpr uint16_t NUM_LEDS = WIDTH * HEIGHT;

CRGB leds[NUM_LEDS];

bool kMatrixSerpentineLayout = true;
bool kMatrixVertical = false;

uint16_t xyMap(uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
  uint16_t i;

  if (x & 0x01) {
    // Odd columns run backwards
    uint8_t reverseY = (height - 1) - y;
    i = (x * height) + reverseY;
  } else {
    // Even rows run forwards
    i = (x * height) + y;
  }
  /*
    Serial.print("xyMap(");
    Serial.print(x);
    Serial.print(", ");
    Serial.print(y);
    Serial.print(", ");
    Serial.print(width);
    Serial.print(", ");
    Serial.print(height);
    Serial.print(") = ");
    Serial.println(i);
  */
  return i;
}

auto matrix = fl::XYMap::constructWithUserFunction(WIDTH, HEIGHT, &xyMap);
fl::Leds mappedLeds = fl::Leds(leds, matrix);

uint32_t gFps = 60;
uint32_t gHueSpeed = 50;
uint8_t gBrightness = 255;
uint8_t gEffect = 1;  // Index number of which pattern is current
uint8_t gHue = 0;     // rotating "base color" used by many of the patterns
CRGB gSingleColor;

AsyncWiFiMulti wifiMulti;

#include "togaws_config.h"
#include "webserver.h"
const char *config_filename = "/config.json";
Config config;
TogaWebServer tws;

void esp_now_recv_cb(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, int data_len) {
  Serial.print("esp_now: ");
  Serial.println(data_len);
}

void setup(void) {
  Serial.begin(115200);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  pinMode(DATA_PIN, OUTPUT);
  digitalWrite(LED_BLUE, 0);
  digitalWrite(LED_RED, 1);

  delay(100);

  if (!SPIFFS.begin(false)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }
  // setCpuFrequencyMhz(80);
  config.loadConfiguration(config_filename);
  Serial.println(config.hostname);
  Serial.println(config.leds);
  Serial.println(" LEDs");

  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, config.leds).setCorrection(0xffe0b0);
  FastLED.setBrightness(gBrightness);
  mappedLeds.fill(CRGB::Black);
  FastLED.show();

  Serial.println("FastLED started");

  esp_now_register_recv_cb(esp_now_recv_cb);

  WiFi.hostname(config.hostname);

  // wifiMulti.addAP("immer_is_irgendwas", "das hast du nicht gesehen");
  wifiMulti.addAP("aha-AP", "rasselbande");
  wifiMulti.addAP("aha-AP", "rasselbande");
  // wifiMulti.addAP("SPG_Guest 2", "hotspot123");
  wifiMulti.onConnected([](const AsyncWiFiMulti::ApSettings &ap) {
    digitalWrite(LED_BLUE, 1);
    Serial.printf("Connected to AP: %s, localIP: %s\n", ap.ssid.c_str(), WiFi.localIP().toString().c_str());
  });
  wifiMulti.onFailure([]() {
    digitalWrite(LED_BLUE, 0);
    Serial.println("Failed to connect to any configured APs. Retrying in 10 seconds...");
    // adc_power_off();
    WiFi.mode(WIFI_OFF);
    delay(10000);
    // adc_power_on();
    wifiMulti.start();  // Restart the connection attempt
  });
  wifiMulti.onDisconnected([](const char *ssid, uint8_t disconnectionReason) {
    digitalWrite(LED_BLUE, 0);
    Serial.printf("Disconnected from AP: %s, Reason: %d\n", ssid, disconnectionReason);
    WiFi.disconnect();
    Serial.println("Retrying in 10 seconds...");
    // adc_power_off();
    WiFi.mode(WIFI_OFF);
    delay(10000);
    // adc_power_on();
    wifiMulti.start();  // Restart the connection attempt
  });
  Serial.println("Start WiFi");
  wifiMulti.start();
  Serial.println("Start WebServer");
  tws.begin();
  initStarfield();

  Serial.println("Start MDNS");
  if (MDNS.begin(WiFi.getHostname())) {
    Serial.print("MDNS responder started: ");
    Serial.println(WiFi.getHostname());
  }
  MDNS.addService("http", "tcp", 80);
  listDir(SPIFFS, "/", 0);
  Serial.println("setup done.");
  digitalWrite(LED_RED, 0);
}

SimplePatternList gPatterns = { singleColor, starfield, glimmer, rainbowWithGlitter, confetti, sinelon, juggle, bpm, powersave };
size_t gPatterns_cnt = ARRAY_SIZE(gPatterns);

CEveryNMillis iFps(1000 / gFps);
CEveryNMillis iHue(2000 / gHueSpeed);

void loop(void) {
  // MDNS.update();
  webServer.handleClient();

  // Call the current pattern function once, updating the 'leds' array
  if (iFps) {
    gPatterns[gEffect]();
    // noInterrupts();
    FastLED.show();
    // interrupts();
  }
  if (iHue) {
    gHue++;  // slowly cycle the "base color" through the rainbow
  }

  int remaining = min(iFps.getRemaining(), iHue.getRemaining());
  // Serial.println(remaining);
  delay(remaining);
}

void listDir(fs::FS &fs, const char *dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\r\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("- failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println(" - not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels) {
        listDir(fs, file.path(), levels - 1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("\tSIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

void singleColor() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = gSingleColor;
  }
}

CHSV gGlimmerBaseHSV(40, 255, 255);
CRGB gGlimmerBase[NUM_LEDS];
int8_t gGlimmerState[NUM_LEDS];
uint8_t gGlimmerVal[NUM_LEDS];

void glimmer() {
  for (int i = 0; i < NUM_LEDS; i++) {
    gGlimmerVal[i] += gGlimmerState[i];
    leds[i] = gGlimmerBase[i] % gGlimmerVal[i];
    if (gGlimmerVal[i] >= 250) {
      gGlimmerState[i] = -1 - random8(3);
    }
    if (gGlimmerVal[i] <= 70) {
      gGlimmerBase[i] = CHSV(30 + random8(20), 128 + random8(128), 255);
      gGlimmerState[i] = 1 + random8(3);
    }
  }
}

void rainbow() {
  // FastLED's built-in rainbow generator
  fill_rainbow(leds, NUM_LEDS, gHue, 7);
}

void rainbowWithGlitter() {
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow();
  addGlitter(80);
}

void addGlitter(fract8 chanceOfGlitter) {
  if (random8() < chanceOfGlitter) {
    leds[random16(NUM_LEDS)] += CRGB::White;
  }
}

void confetti() {
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy(leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV(gHue + random8(64), 200, 255);
}

void sinelon() {
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy(leds, NUM_LEDS, 20);
  int pos = beatsin16(13, 0, NUM_LEDS - 1);
  leds[pos] += CHSV(gHue, 255, 192);
}

void bpm() {
  // colored stripes pulsing at a defined Beats-Per-Minute(BPM)
  uint8_t BeatsPerMinute = 62;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8(BeatsPerMinute, 64, 255);
  for (int i = 0; i < NUM_LEDS; i++) {  //9948
    leds[i] = ColorFromPalette(palette, gHue + (i * 2), beat - gHue + (i * 10));
  }
}

void juggle() {
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy(leds, NUM_LEDS, 20);
  byte dothue = 0;
  for (int i = 0; i < 8; i++) {
    leds[beatsin16(i + 7, 0, NUM_LEDS - 1)] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}

void powersave() {
  delay(200);
}


struct Star {
  int8_t x, y, z;
  uint8_t hue;
};

Star _stars[24];
const int8_t MAX_DEPTH = 127;

void initStarfield() {
  Serial.println("ModeMood::initStarfield()");
  for (uint8_t i = 0; i < array_size(_stars); i++) {
    _stars[i] = {
      (int8_t)random(-12, 12),
      (int8_t)random(-4, 4),
      (int8_t)random(1, MAX_DEPTH),
      (uint8_t)random8(0, 191)
    };
  }
}

// adapted from http://codentronix.com/2011/07/22/html5-canvas-3d-starfield/
void starfield() {
  /*
    // TEST left to right each line
    static int step_x=0;
    static int step_y=0;
    for(int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Black;
    }
    //mappedLeds.fill(CRGB::Black);

    mappedLeds(step_x, step_y) = CRGB::White;

    if (++step_x >= matrix.getWidth())
    {
    step_x = 0;
    step_y++;
    }

    if (step_y >= matrix.getHeight())
    {
    step_y = 0;
    }
  */


  fadeToBlackBy(leds, NUM_LEDS, 64);
  // mappedLeds.fill(CRGB::Black);

  uint8_t halfWidth = matrix.getWidth() / 2;
  uint8_t halfHeight = matrix.getHeight() / 2;
  for (uint8_t i = 0; i < array_size(_stars); i++) {
    uint8_t warp = 0;
    for (uint8_t w = 0; w <= warp; w++) {
      float k = MAX_DEPTH / (float)(_stars[i].z + w);
      int px = _stars[i].x * k + halfWidth;
      int py = _stars[i].y * k + halfHeight;
      if (px >= 0 && px < matrix.getWidth() && py >= 0 && py <= matrix.getHeight() && _stars[i].z > 0) {
        mappedLeds(px, py) = CRGB::Black;
      }
    }
    _stars[i].z -= 1;
    warp = 0;
    for (uint8_t w = 0; w <= warp; w++) {
      float k = MAX_DEPTH / (float)(_stars[i].z + w);
      int px = _stars[i].x * k + halfWidth;
      int py = _stars[i].y * k + halfHeight;
      if (px >= 0 && px < matrix.getWidth() && py >= 0 && py <= matrix.getHeight() && _stars[i].z > 0) {
        float brightness = (1 - _stars[i].z / (float)MAX_DEPTH);

        CHSV hsv;
        hsv.hue = _stars[i].hue;
        hsv.val = 255 * brightness;
        hsv.sat = 60;
        mappedLeds(px, py) = hsv;
      } else {
        if (_stars[i].z < 30) {
          _stars[i] = {
            (int8_t)random(-8, 8),
            (int8_t)random(-3, 3),
            MAX_DEPTH,
            (uint8_t)(random8(0, 191))
          };
        }
      }
    }
  }
}
