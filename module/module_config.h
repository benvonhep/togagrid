// ══════════════════════════════════════════════════════════════
//  module_config.h — constants for a Toga LED module.
//  (#included first by module.ino; do not compile alone.)
//
//  The module deliberately owns very few numbers. Everything that has to
//  match the grid (brightness cap, beat-glow shape, strobe timing,
//  heartbeat timeout) comes from <toga_proto.h>, and everything that
//  varies per physical module (group, geometry, pin, effect, gain) comes
//  from NVS at runtime. What is left here is genuinely module-wide.
// ══════════════════════════════════════════════════════════════
#pragma once

#include <toga_proto.h>

// FastLED 3.10.x breaks ESP-NOW / the controller link on this project, and
// the failure is a silent one. Refuse to build against anything else.
#if FASTLED_VERSION != 3006000
#error "Pinned to FastLED 3.6.0 — 3.10.x breaks ESP-NOW/controller here."
#endif

// Which broadcasts this node acts on. NVS overrides it; this is the default
// for a freshly flashed board. (Group 0 = everyone is always accepted.)
#define MODULE_GROUP_DEFAULT TOGA_GROUP_MODULES

// ── LED hardware ──
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
// Fixed ceiling for the pixel buffer. Geometry is runtime, but the array is
// not — see addLedsForPin() in module.ino for why the PIN cannot be.
// Past ~400px a FastLED.show() no longer fits inside STROBE_FLASH_MS and the
// strobe stops looking sharp (same physical limit as the grid's 12 strips).
#define MAX_LEDS  512

// ── 1D mode set (module_effects.h) ──
// The 90 procedural modes are designed and supported for strips of 1..120 LEDs.
// MAX_LEDS stays 512 so existing 32x8 panels keep working (they render the 1D
// animation as vertical bars; n for all maths is the column count = gWidth).
#define MOD_LEDS_MIN       1
#define MOD_LEDS_MAX     120
#define MOD_MAX_ELEMENTS  48   // particle/element ceiling per mode
#define MOD_DEGENERATE_N   6   // below this, positional archetypes fall back to whole-strip

// GPIO2 is the onboard LED on most ESP32 dev boards — used as link status.
// (The old firmware also drove GPIO0 as LED_RED; that is a boot strapping
// pin and driving it can stop the board from booting. Dropped.)
#define LED_BLUE 2

// ── Frame pacing ──
#define FRAME_MS 20            // 50 FPS cap, same as the grid

// ── Mode-strobe / castle-strobe blink timing ──
// The protocol has no param for these, so the grid can never move off its
// own defaults — see the strobeOnMs/strobeOffMs defaults in grid.ino.
// If those ever change, change these too.
#define MODE_ON_MS    40
#define MODE_OFF_MS   40
#define CASTLE_ON_MS  40
#define CASTLE_OFF_MS 40

// Brightness the strobes flash at, before the per-module gain trim.
// The button-15 strobe does NOT reach this: STROBE_MAX_BRI (<toga_proto.h>)
// caps it at 70% after the trim. The castle strobe still uses the full value.
#define STROBE_BRI 255

// ── Salvaged from the old globals.h ──
typedef void (*SimplePatternList[])();
#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

template<class T, size_t N>
constexpr size_t array_size(T (&)[N]) {
  return N;
}
