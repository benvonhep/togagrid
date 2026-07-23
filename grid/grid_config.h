// ══════════════════════════════════════════════════════════════
//  grid_config.h — central tunable constants for the LED grid.
//  This is the one place to adjust hardware, geometry, brightness
//  and timing. (#included first by grid.ino; do not compile alone.)
//
//  Constants the grid SHARES with the modules (brightness cap and step,
//  beat-glow shape, strobe timing, heartbeat timeout) live in
//  <toga_proto.h>, not here — they must match exactly across nodes.
// ══════════════════════════════════════════════════════════════
#pragma once

#include <toga_proto.h>

// FastLED 3.10.x breaks ESP-NOW / the controller link on this project, and
// the failure is a silent one. Refuse to build against anything else.
#if FASTLED_VERSION != 3006000
#error "Pinned to FastLED 3.6.0 — 3.10.x breaks ESP-NOW/controller here."
#endif

// Which broadcasts this node acts on (group 0 = everyone is always accepted).
#define MY_TOGA_GROUP TOGA_GROUP_GRID

// ── LED hardware ──
#define LEDS_ACTIVE    121   // lit LEDs per strip
#define LEDS_SPACER     15   // dead LEDs at the return bend
#define LEDS_PER_STRIP 258   // buffer length per strip (active + spacer + return)
#define NUM_ELEC        6    // driver boards (each drives one X + one Y strip)

// ── LED data GPIO pins (X strips 0..5, then Y strips 0..5) ──
#define PIN_X0 14
#define PIN_X1 15
#define PIN_X2 17
#define PIN_X3 16
#define PIN_X4 13
#define PIN_X5 18
#define PIN_Y0 21
#define PIN_Y1 22
#define PIN_Y2 23
#define PIN_Y3 25
#define PIN_Y4 26
#define PIN_Y5 19

// ── Animation registry ──
#define TOTAL_ANIMS   90     // number of animation modes (0..TOTAL_ANIMS-1)
#define MAX_MODES     96     // upper bound for per-mode arrays (>= TOTAL_ANIMS)
#define ANIM_DURATION 10000  // (reserved)

// ── Brightness ──
// MAX_BRIGHTNESS / BRI_DEFAULT / BRI_STEP: see <toga_proto.h> (shared with modules)
#define BRIGHTNESS      40   // startup brightness

// ── Auto brightness normalization (AGC): consistent perceived brightness across modes ──
#define AGC_TARGET  45.0f    // target avg (sum-of-channels per pixel); tune on hardware
#define AGC_MIN     0.35f    // clamp: how far a very bright mode can be dimmed
#define AGC_MAX     1.5f     // clamp: how far a sparse mode can be lifted
#define AGC_K       0.05f    // smoothing per frame (~0.4s); preserves intended pulses

// ── Beat-Glow (tap-tempo pulse laid over the animations) ──
// Shape + range shared with the modules — BEAT_MIN / BEAT_MIN_MS / BEAT_MAX_MS /
// BEAT_LATENCY_MS / togaBeatFactor(): see <toga_proto.h>.
// The tap-tempo detection itself (debounce, lock, phase) is grid-only: grid_beat.h

// ── Strobe / controller input steps & timing ──
// Button-15 strobe: flash as short as the LED refresh allows, gap is user-tunable
// (hold 15 + press +/-). A full FastLED.show() of all 12 strips takes ~10-15ms,
// so the flash cannot physically get much shorter than STROBE_FLASH_MS.
// STROBE_FLASH_MS / STROBE_GAP_* / HEARTBEAT_TIMEOUT: see <toga_proto.h>
#define STROBE_BRI_STEP   10
#define STROBE_FREQ_STEP   2
#define STROBE_SQ_STEP     1
#define REPEAT_DELAY      500  // ms before key auto-repeat
#define REPEAT_RATE        80  // ms between repeats

// ── Grid geometry (physical strip positions, in LED units from centre) ──
const float CENTER   = 61.0f;
const float MAX_DIST = 72.0f;
const float X_POS[12] = { -51,-42,-32,-23,-14,-5, 5,14,23,32,42,51 };
const float Y_POS[12] = { -51,-42,-32,-23,-14,-5, 5,14,23,32,42,51 };
const int   INTS[12]  = {10,19,29,38,47,56,66,75,84,93,103,112};  // strip-crossing LED indices
