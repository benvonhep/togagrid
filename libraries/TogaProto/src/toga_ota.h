// ══════════════════════════════════════════════════════════════════════
//  toga_ota.h — firmware updates over the air, on every node, always.
//
//  There is no trigger, no OTA "mode" and no indicator, because there is
//  nothing to switch: the controller's AP is pinned to TOGA_CHANNEL (see
//  togaApBegin in toga_proto.h), which is the channel the ESP-NOW bus
//  already runs on. Joining it therefore moves nobody off the bus, so
//  every node is on the network AND showing at the same time, and can be
//  flashed mid-show without noticing.
//
//  This file used to be four times this size: a TogaOtaMsg, a per-MAC
//  addressing scheme, a JOINING/IDLE/UPLOAD state machine, a button combo
//  on the controller and a breathing indicator on every node. All of it
//  existed to work around the radio being dragged onto a router's channel.
//  Owning the AP deleted the problem rather than managing it.
//
//  Flashing: ./flash-ota.sh <sketch> <hostname>.local
//  Your Mac has to be ON togacontroller for that — it is a private network with
//  no route to the internet, which is the point.
// ══════════════════════════════════════════════════════════════════════
#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <esp_ota_ops.h>
#include "toga_proto.h"

#define TOGA_OTA_PASS "toga-ota"     // espota --auth=
#define TOGA_OTA_PORT 3232

// Hostnames must be unique per board, or mDNS collides and espota silently
// flashes the wrong one. Grid/controller/webserver are singletons and get fixed
// names; a module is one of N, so it wears the last three bytes of its own MAC —
// the identity it already announces in its hello and shows on the web UI.
static inline const char* togaOtaModuleHost(){
  static char h[20];
  uint8_t m[6]; WiFi.macAddress(m);
  snprintf(h, sizeof(h), "toga-mod-%02x%02x%02x", m[3], m[4], m[5]);
  return h;
}

// Call once in setup(), AFTER togaWifiBegin()/togaApBegin(). Non-blocking:
// ArduinoOTA needs no association to arm, and the show must not wait for WiFi.
static inline void togaOtaSetup(const char* hostname){
  ArduinoOTA.setPort(TOGA_OTA_PORT);
  ArduinoOTA.setHostname(hostname);
  ArduinoOTA.setPassword(TOGA_OTA_PASS);
  ArduinoOTA.onStart([](){ Serial.println(F("OTA: start")); });
  ArduinoOTA.onEnd([](){   Serial.println(F("OTA: done, rebooting")); });
  ArduinoOTA.onProgress([](unsigned int d, unsigned int tot){
    static uint8_t last = 255;
    uint8_t p = tot ? (uint8_t)((uint64_t)d * 100 / tot) : 0;
    if(p != last && p % 10 == 0){ last = p; Serial.printf("OTA %u%%\n", p); }
  });
  ArduinoOTA.onError([](ota_error_t e){ Serial.printf("OTA error %u\n", (unsigned)e); });
  ArduinoOTA.begin();
  Serial.printf("OTA armed: %s.local\n", hostname);
}

// Call once per loop(). Must sit ABOVE any early return — the LED sketches
// return early for the frame cap and again for the strobe (on every single
// iteration while it is held), and anything below that would never be flashable
// at the exact moment you most want to fix it.
static inline void togaOtaHandle(){ ArduinoOTA.handle(); }

// ── Rollback: the safety net that makes OTA onto a wall-mounted grid sane ──
//
// The IDF's rollback IS compiled into core 2.0.17
// (CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y). What defeats it is initArduino(),
// which calls the weak verifyRollbackLater() — default false — and so marks
// every freshly OTA'd image valid before setup() has run a single line.
//
// A sketch takes it over with, at file scope, BELOW every #include:
//
//     extern "C" bool verifyRollbackLater() { return true; }
//
// extern "C" IS MANDATORY. The weak symbol lives in esp32-hal-misc.c, a C file,
// and is declared in NO header. A plain C++ definition gets mangled, silently
// fails to override, and you are back to no rollback with no warning at all.
//
// Then call togaOtaMarkValidWhenHealthy() once per frame. The image stays
// PENDING_VERIFY until the node proves itself; if it panics or boot-loops
// first, the bootloader brings back the previous one on its own.
//
// A hook without the commit call is WORSE than neither: the image runs fine,
// then the next reset of any kind silently reverts it. Keep them paired.
//
// What this buys: crashes. What it does NOT buy: an image that boots fine and
// renders garbage marks itself valid quite happily. It also only fires on a
// RESET while pending — a hang with no reset won't trigger it, since Arduino
// leaves the loop task WDT off.
#define TOGA_OTA_COMMIT_FAST_MS  60000   // frames + a packet off the bus: commit
#define TOGA_OTA_COMMIT_MAX_MS  300000   // frames alone for this long: commit anyway

static inline void togaOtaMarkValidWhenHealthy(uint32_t t, bool heardBus){
  static bool committed = false;
  static bool sawBus    = false;
  if(committed) return;
  if(heardBus) sawBus = true;

  // Two ways to prove we are healthy, and the second one is the important one:
  //
  //   · 60s of frames AND a packet off the bus — LEDs up, loop running, radio
  //     working. The strongest evidence, and the normal case.
  //   · 5 minutes of frames, full stop.
  //
  // The bus check may only make us commit SOONER. It must never be able to stop
  // us committing: flash a board with the controller switched off — the bench
  // case — and gating purely on the bus means it runs all evening, gets powered
  // down, and silently comes back on the OLD firmware with nothing to show for
  // it. Rollback only ever protected us from crashes, and a node that has run
  // five minutes has demonstrably not crashed.
  bool healthy = (t >= TOGA_OTA_COMMIT_FAST_MS && sawBus) || (t >= TOGA_OTA_COMMIT_MAX_MS);
  if(!healthy) return;
  committed = true;
  if(esp_ota_mark_app_valid_cancel_rollback() == ESP_OK)
    Serial.printf("OTA image marked valid — rollback window closed (bus=%d)\n", (int)sawBus);
}
