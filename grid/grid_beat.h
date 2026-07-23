// ══════════════════════════════════════════════════════════════════════
//  grid_beat.h — tap-tempo tracker.
//
//  The controller forwards every press of button 11 as a raw TOGA_CMD_BEAT_TAP
//  the instant it happens; ALL of the thinking lives here, next to the frame
//  clock that has to act on it. That split is the whole point: the tap is the
//  phase anchor, and a phase anchor is worthless if it is averaged on one node
//  and replayed on another.
//
//  Two properties this file exists to guarantee:
//
//   1. The peak lands ON the tap. beatPhase() is derived from an absolute
//      anchor time (`bAnchor`), never integrated frame by frame, so it cannot
//      accumulate error and cannot drift away from where you tapped.
//   2. The tempo, once locked, is hard to lose. Assumption: the tempo does not
//      change inside a song. A tap that fits only nudges the period by
//      BEAT_ADJUST; it takes BEAT_LOSE consecutive misfits to give up.
//
//  Nothing here blocks and nothing here allocates.
// ══════════════════════════════════════════════════════════════════════
#pragma once

#include <Arduino.h>
#include <math.h>
#include <toga_proto.h>   // BEAT_MIN_MS/MAX_MS, BEAT_LATENCY_MS, togaBeatFactor()

// ── Tuning ──
#define BEAT_DEBOUNCE_MS  100   // taps closer together than this are contact bounce
#define BEAT_WINDOW         4   // intervals averaged while learning (rolling)
#define BEAT_LOCK_IVS       3   // lock after this many valid intervals (= 4 taps)
#define BEAT_TOL         0.15f  // ±15% — how far off an interval may be and still "fit"
#define BEAT_ADJUST      0.05f  // a fitting interval moves the locked period 5% toward it
#define BEAT_PULL        0.30f  // a tap pulls the phase 30% of the way to where it landed
#define BEAT_LOSE           6   // consecutive misfits before the tempo is thrown away

// ── The off switch: ONE tap, then silence ──
// The glow had no way to be switched off at all — once locked it ran forever.
// "Tap once and walk away" is the gesture that reads as stop; two taps are
// always the start of a tempo, so the two cannot be confused. A sequence is
// broken by BEAT_SOLO_MS of silence, and only a sequence of exactly one tap
// turns the glow off.
//
// The cost, deliberately paid: a lone tap no longer nudges the phase of a
// locked tempo — it kills it. Nudging now takes two taps (which the tracker
// reads as an on-tempo interval and pulls the phase anyway).
#define BEAT_SOLO_MS     2000   // silence after a lone tap before the glow goes off

// ── State ──
// The true anchor is (bAnchor + bAnchorFrac): the ms of a beat, split into a
// whole part and a 0..1 remainder. Splitting it keeps every subtraction small,
// so a float never has to hold a millis() value it cannot represent exactly
// (millis() passes 2^24 after ~4.7 hours and float ms would start snapping to
// 2ms steps — audible as jitter on a 500ms beat).
static float    bPeriod     = 0.0f;   // locked beat period in ms (0 = no tempo, glow off)
static bool     bLocked     = false;
static uint32_t bAnchor     = 0;
static float    bAnchorFrac = 0.0f;
static uint32_t bLastTap    = 0;
static float    bIvs[BEAT_WINDOW];    // rolling window of intervals, learning only
static uint8_t  bIvCount    = 0;
static uint8_t  bMiss       = 0;      // consecutive intervals that fit nothing
static uint8_t  bSeqTaps    = 0;      // taps in the current sequence; 1 + silence = off
static bool     bSoloArmed  = false;  // a tap is waiting to be judged lone-or-not

// Move the anchor by `ms` (may be negative), keeping the whole/frac split normal.
static inline void beatAnchorShift(float ms){
  float na = bAnchorFrac + ms;
  float w  = floorf(na);
  bAnchor += (uint32_t)(int32_t)w;    // negative w wraps correctly in uint32 arithmetic
  bAnchorFrac = na - w;               // stays in [0,1)
}

static inline void beatAnchorSet(uint32_t t){ bAnchor = t; bAnchorFrac = 0.0f; }

static inline void beatForget(const char* why){
  bPeriod = 0.0f; bLocked = false; bIvCount = 0; bMiss = 0;
  Serial.printf("[beat] Tempo verworfen (%s) — tippe neu ein\n", why);
}

// Where we are inside the current beat, 0.0 (on the beat) .. 1.0 (just before
// the next). Advances the anchor in whole periods so the subtraction below
// stays small; the fractional carry is what keeps it drift-free.
static inline float beatPhase(uint32_t now){
  if(bPeriod <= 0.0f) return 0.0f;
  // Run the clock ahead so the peak is pulled forward by BEAT_LATENCY_MS.
  uint32_t tt = now + BEAT_LATENCY_MS;
  float el = (float)(int32_t)(tt - bAnchor) - bAnchorFrac;   // ms since the anchor beat
  // Normalize into [0, bPeriod). el can go slightly negative when a phase pull
  // has just moved the anchor ahead of the (latency-shifted) clock.
  if(el >= bPeriod || el < 0.0f){
    float n = floorf(el / bPeriod);
    beatAnchorShift(n * bPeriod);
    el -= n * bPeriod;
  }
  float ph = el / bPeriod;
  return ph < 0.0f ? 0.0f : (ph > 1.0f ? 1.0f : ph);
}

// The brightness factor to multiply the whole frame by: 0.25..1.0, and exactly
// 1.0 while no tempo is known (i.e. the glow is simply off).
static inline float beatFactor(uint32_t now){
  if(bPeriod <= 0.0f) return 1.0f;
  return togaBeatFactor(beatPhase(now));
}

// Append an interval to the rolling window. Fed on every in-range tap, locked
// or not: the window is what lets a locked tempo be overruled by a new one.
static inline void beatPushIv(float iv){
  if(bIvCount < BEAT_WINDOW) bIvs[bIvCount++] = iv;
  else { for(int i=1;i<BEAT_WINDOW;i++) bIvs[i-1]=bIvs[i]; bIvs[BEAT_WINDOW-1]=iv; }
}

// Do the last BEAT_LOCK_IVS intervals agree on a tempo of their own? They do if
// every one of them sits within BEAT_TOL of their mean — i.e. you tapped
// BEAT_LOCK_IVS+1 times evenly. `out` gets the mean.
//
// This is the override gesture, and it deliberately ignores what is already
// locked. Four even taps state a tempo; nothing about the old tempo — not the
// 2x-harmonic reading, not BEAT_LOSE — is allowed to outvote them.
static inline bool beatWindowTempo(float* out){
  if(bIvCount < BEAT_LOCK_IVS) return false;
  float sum = 0.0f;
  for(int i = bIvCount - BEAT_LOCK_IVS; i < bIvCount; i++) sum += bIvs[i];
  float mean = sum / (float)BEAT_LOCK_IVS;
  for(int i = bIvCount - BEAT_LOCK_IVS; i < bIvCount; i++)
    if(fabsf(bIvs[i] - mean) > BEAT_TOL * mean) return false;
  *out = mean;
  return true;
}

// Nudge the phase toward a tap that we believe is on a beat. Never a hard
// re-anchor once locked: taps have ~±10ms of poll jitter on them, and snapping
// to each one would make the pulse wobble around a tempo it already knows.
static inline void beatPullPhase(uint32_t now){
  float ph  = ((float)(int32_t)(now - bAnchor) - bAnchorFrac) / bPeriod;
  float err = ph - roundf(ph);                 // -0.5..0.5 beats from the nearest beat
  beatAnchorShift(err * bPeriod * BEAT_PULL);
}

// One tap from button 11. `now` is the tap's arrival time, already corrected
// for the grid's own queue delay by the caller.
static inline void beatOnTap(uint32_t now){
  // Contact bounce: a real tap is never 100ms after the last one (that'd be 600 BPM).
  if(bLastTap && (uint32_t)(now - bLastTap) < BEAT_DEBOUNCE_MS) return;

  // Sequence bookkeeping for the off switch — must read bLastTap before the
  // line below overwrites it. A gap longer than BEAT_SOLO_MS starts a new
  // sequence, which is the same silence that arms the off switch, so a tap can
  // never be both "the last of the old sequence" and "the first of the new".
  if(bLastTap == 0 || (uint32_t)(now - bLastTap) > BEAT_SOLO_MS) bSeqTaps = 1;
  else if(bSeqTaps < 255) bSeqTaps++;
  bSoloArmed = true;                  // re-armed by every tap; beatTick judges it

  uint32_t iv = bLastTap ? (now - bLastTap) : 0;
  bLastTap = now;

  if(iv == 0){ Serial.println("[beat] Tap"); return; }          // first tap of a session
  Serial.printf("[beat] Tap  %lums  %.1f BPM\n", (unsigned long)iv, 60000.0f/(float)iv);

  // Outlier (a pause, or a double-hit that survived the debounce): it says
  // nothing about the tempo. Drop it — but do NOT let it kill a locked tempo.
  if(iv < BEAT_MIN_MS || iv > BEAT_MAX_MS){
    bIvCount = 0;   // averaging across a pause would be nonsense; the lock survives
    return;
  }

  beatPushIv((float)iv);

  if(bLocked){
    // ── Override: BEAT_LOCK_IVS+1 even taps replace the locked tempo ──
    // Tested before the on-tempo/harmonic branches below, because those are
    // exactly what used to swallow a deliberate retap: tap half as fast and
    // every interval reads as "you missed a beat" at ~2x, which CONFIRMS the
    // old period. Anything else needed BEAT_LOSE misfits in a row. Fast→slow
    // could therefore never win, however long you tapped.
    //
    // Only fires when the window's tempo actually differs — an even run at the
    // locked tempo must fall through to the phase pull below, or every tap
    // would hard re-anchor and the pulse would wobble on its own jitter.
    float m;
    if(beatWindowTempo(&m) && fabsf(m - bPeriod) > BEAT_TOL * bPeriod){
      bPeriod = m;
      bMiss   = 0;
      beatAnchorSet(now);          // THE tap is the beat: anchor hard, as on first lock
      Serial.printf("[beat] LOCK NEU  %.0fms  %.1f BPM\n", bPeriod, 60000.0f/bPeriod);
      return;
    }

    float d1 = fabsf((float)iv - bPeriod);
    float d2 = fabsf((float)iv - 2.0f * bPeriod);
    if(d1 <= BEAT_TOL * bPeriod){                     // on tempo → nudge period, pull phase
      bPeriod += ((float)iv - bPeriod) * BEAT_ADJUST;
      bMiss = 0;
      beatPullPhase(now);
    } else if(d2 <= BEAT_TOL * 2.0f * bPeriod){       // ~2x period: we simply missed one beat
      bMiss = 0;                                      // still on the grid — tempo is fine
      beatPullPhase(now);                             // and the tap is still on a beat
    } else {                                          // fits nothing
      if(++bMiss >= BEAT_LOSE){
        beatForget("Takt passt nicht mehr");
        bIvs[0] = (float)iv; bIvCount = 1;            // this tap starts the new sequence
      }
    }
    return;
  }

  // ── Learning: mean of the last BEAT_WINDOW intervals ──
  // Stays forgiving on purpose: no evenness test here, so sloppy first taps
  // still lock. The evenness test is only what it takes to overrule a tempo
  // that is already locked.
  if(bIvCount >= BEAT_LOCK_IVS){
    float sum = 0.0f; for(int i=0;i<bIvCount;i++) sum += bIvs[i];
    bPeriod = sum / bIvCount;
    bLocked = true;
    bMiss   = 0;
    beatAnchorSet(now);                               // THE tap is the beat: anchor hard, once
    Serial.printf("[beat] LOCK  %.0fms  %.1f BPM\n", bPeriod, 60000.0f/bPeriod);
  }
}

// Call once per frame. The lone-tap timeout is the one thing in here that no
// tap can trigger — it exists precisely because nothing else happened.
// Returns true exactly once, on the frame the glow is switched off, so the
// caller can persist the change.
static inline bool beatTick(uint32_t now){
  if(!bSoloArmed || bLastTap == 0) return false;
  if(bSeqTaps != 1){ bSoloArmed = false; return false; }        // 2+ taps = a tempo, never an off
  if((uint32_t)(now - bLastTap) < BEAT_SOLO_MS) return false;   // still might get a second tap
  bSoloArmed = false;
  if(bPeriod <= 0.0f) return false;                             // nothing was running: nothing to say
  beatForget("einzelner Tap");
  return true;
}

// Adopt a period directly (restored from NVS, or TOGA_CMD_BEAT). Counts as
// locked: the phase is arbitrary until the first tap pulls it into place.
static inline void beatSetPeriod(float ms, uint32_t now){
  bPeriod = constrain(ms, (float)BEAT_MIN_MS, (float)BEAT_MAX_MS);
  bLocked = true; bIvCount = 0; bMiss = 0;
  bSoloArmed = false;                 // not a tap; nothing pending to judge
  beatAnchorSet(now);
}
