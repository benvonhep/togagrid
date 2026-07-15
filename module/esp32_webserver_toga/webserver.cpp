#include <Arduino.h>

#include <FS.h>
#include <SPIFFS.h>

#include <WebServer.h>
#include <uri/UriBraces.h>
#include <ArduinoJson.h>
#include <AsyncUDP.h>

#include <FastLED.h>
#include <fl/leds.h>

#include "globals.h"
#include "togaws_config.h"

#include "webserver.h"

extern char *config_filename;
extern Config config;
extern CRGB gSingleColor;
extern size_t gPatterns_cnt;
extern uint32_t gFps;
extern uint32_t gHueSpeed;
extern uint8_t gBrightness;
extern uint8_t gEffect;  // Index number of which pattern is current
extern uint8_t gHue;     // rotating "base color" used by many of the patterns
extern WebServer webServer;

extern CEveryNMillis iFps;
extern CEveryNMillis iHue;

extern fl::Leds mappedLeds;

AsyncUDP udpServer;

void TogaWebServer::begin() {
  // what to do with requests
  // server.on("/", handleRoot);
  webServer.on("/config/", HTTP_GET, handleConfig);
  webServer.on("/config/", HTTP_POST, handleConfigPost);
  webServer.on("/store/", HTTP_GET, handleStore);
  webServer.on(UriBraces("/effect/{}"), handleEffect);
  webServer.on(UriBraces("/color/{}"), handleColor);
  webServer.on(UriBraces("/settings/{}/{}/{}"), handleSettings);
  webServer.onNotFound([]() {
    if (!handleFileRead(webServer.uri())) {
      webServer.send(404, "text/plain", "FileNotFound");
    }
  });

  //  server.onNotFound(handleNotFound);
  webServer.begin();
  if (udpServer.listen(7094)) {
    Serial.print("UDP Listening on Port 7094");
    // Serial.println(WiFi.localIP());
    udpServer.onPacket([](AsyncUDPPacket packet) {
      int i = String((char *)packet.data()).toInt();
      Serial.println(i);
      CRGB col = CRGB::Black;
      switch(i) {
        case 0:
          col = CRGB::Purple;
          break;
        case 1:
          col = CRGB::Red;
          break;
        case 2:
          col = CRGB::Green;
          break;
        case 3:
          col = CRGB::Blue;
          break;
        default:
          col = CRGB::Black;
          break;
      }
      mappedLeds.fill(col);
      /*
      // FastLED.show();

      Serial.print("UDP Packet Type: ");
      Serial.print(packet.isBroadcast() ? "Broadcast" : packet.isMulticast() ? "Multicast"
                                                                             : "Unicast");
      Serial.print(", From: ");
      Serial.print(packet.remoteIP());
      Serial.print(":");
      Serial.print(packet.remotePort());
      Serial.print(", To: ");
      Serial.print(packet.localIP());
      Serial.print(":");
      Serial.print(packet.localPort());
      Serial.print(", Length: ");
      Serial.print(packet.length());
      Serial.print(", Data: ");
      Serial.write(packet.data(), packet.length());
      Serial.println();
      //reply to the client
      packet.printf("Got %lu bytes of data", (unsigned long)packet.length());
      */
      
    });
  }
}

bool TogaWebServer::exists(String path) {
  Serial.print("exists: ");
  Serial.println(path);
  bool yes = false;
  File file = SPIFFS.open(path, "r");
  if (!file.isDirectory()) {
    yes = true;
  }
  file.close();
  return yes;
}

String TogaWebServer::getContentType(String filename) {
  if (webServer.hasArg("download")) {
    return "application/octet-stream";
  } else if (filename.endsWith(".htm")) {
    return "text/html";
  } else if (filename.endsWith(".html")) {
    return "text/html";
  } else if (filename.endsWith(".css")) {
    return "text/css";
  } else if (filename.endsWith(".js")) {
    return "application/javascript";
  } else if (filename.endsWith(".png")) {
    return "image/png";
  } else if (filename.endsWith(".gif")) {
    return "image/gif";
  } else if (filename.endsWith(".jpg")) {
    return "image/jpeg";
  } else if (filename.endsWith(".ico")) {
    return "image/x-icon";
  } else if (filename.endsWith(".xml")) {
    return "text/xml";
  } else if (filename.endsWith(".pdf")) {
    return "application/x-pdf";
  } else if (filename.endsWith(".zip")) {
    return "application/x-zip";
  } else if (filename.endsWith(".gz")) {
    return "application/x-gzip";
  }
  return "text/plain";
}

bool TogaWebServer::handleFileRead(String path) {
  digitalWrite(LED_BLUE, 0);
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) {
    path += "index.html";
  }
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if (exists(pathWithGz) || exists(path)) {
    if (exists(pathWithGz)) {
      path += ".gz";
    }
    File file = SPIFFS.open(path, "r");
    webServer.streamFile(file, contentType);
    file.close();
    digitalWrite(LED_BLUE, 1);
    return true;
  }
  digitalWrite(LED_BLUE, 1);
  return false;
}

void TogaWebServer::handleConfig() {
  String jstr;
  digitalWrite(LED_BLUE, 0);
  Serial.println("GET /config/");
  config.writeConfiguration(jstr);
  webServer.send(200, "application/json", jstr);

  digitalWrite(LED_BLUE, 1);
}

void TogaWebServer::handleConfigPost() {
  digitalWrite(LED_BLUE, 0);
  Serial.println("POST /config/");
  for (int i = 0; i < webServer.args(); i++) {
    Serial.print(i);
    Serial.print(" ");
    Serial.print(webServer.argName(i));
    Serial.print("=");
    Serial.println(webServer.arg(i));
  }
  config.loadConfiguration(webServer.arg("plain"));

  String jstr;
  config.writeConfiguration(jstr);
  webServer.send(200, "application/json", jstr);
  digitalWrite(LED_BLUE, 1);
}

void TogaWebServer::handleStore() {
  digitalWrite(LED_BLUE, 0);
  Serial.println("GET /store/");
  if (config.saveConfiguration(config_filename)) {
    webServer.send(200, "text/html", "OK");
  } else {
    webServer.send(503, "text/html", "Error");
  }
  ESP.restart();
  digitalWrite(LED_BLUE, 1);
}

void TogaWebServer::handleEffect() {
  digitalWrite(LED_BLUE, 0);
  String eff_str = webServer.pathArg(0);
  int effect = eff_str.toInt();
  if (effect >= 1 && effect < gPatterns_cnt) {
    gEffect = effect;
    webServer.send(200, "text/html", "OK");
  } else {
    webServer.send(503, "text/html", "Error");
  }
  digitalWrite(LED_BLUE, 1);
}

void TogaWebServer::handleColor() {
  digitalWrite(LED_BLUE, 0);
  String eff_str = webServer.pathArg(0);
  int effect = strtol(eff_str.c_str(), NULL, 16);
  if (effect >= 0 && effect <= 0xffffff) {
    gSingleColor = effect;
    gEffect = 0;
    webServer.send(200, "text/html", "OK");
  } else {
    gEffect = effect;
    webServer.send(503, "text/html", "Error");
  }
  digitalWrite(LED_BLUE, 1);
}

void TogaWebServer::handleSettings() {
  digitalWrite(LED_BLUE, 0);
  String bright_str = webServer.pathArg(0);
  String fps_str = webServer.pathArg(1);
  String hue_str = webServer.pathArg(2);
  gBrightness = max(min(bright_str.toInt(), 255L), 1L);

  int fps = fps_str.toInt();
  int hue = hue_str.toInt();
  bool ok = false;
  FastLED.setBrightness(gBrightness);
  if (fps >= 1 && fps < 1000) {
    gFps = fps;
    uint32_t msec = 1000 / gFps;
    iFps.setPeriod(msec);
    ok = true;
  }
  if (hue >= 1 && hue < 1000) {
    gHueSpeed = hue;
    uint32_t msec = 2000 / gHueSpeed;
    iHue.setPeriod(msec);
    ok = true;
  }
  if (ok) {
    String response("OK.");
    response += " fps=";
    response += gFps;
    response += ", hue_spd=";
    response += gHueSpeed;
    webServer.send(200, "text/html", response);
  } else {
    webServer.send(503, "text/html", "Error");
  }
  digitalWrite(LED_BLUE, 1);
}

void TogaWebServer::handleNotFound() {
  digitalWrite(LED_BLUE, 0);
  Serial.println("Not Found");
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += webServer.uri();
  message += "\nMethod: ";
  message += (webServer.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += webServer.args();
  message += "\n";

  for (uint8_t i = 0; i < webServer.args(); i++) {
    message += " " + webServer.argName(i) + ": " + webServer.arg(i) + "\n";
  }

  webServer.send(404, "text/plain", message);
  Serial.println(message);
  digitalWrite(LED_BLUE, 1);
}
