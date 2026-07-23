// ═══════════════════════════════════════════════════════
//  module_battery.h — Li-Ion voltage monitoring + undervoltage lockout.
//  (#included by module.ino after the globals; do not compile alone.)
//
//  Single cell, measured through a 50:50 resistive divider (98.7k / 100.66k)
//  on GPIO34 = ADC1_CHANNEL_6 (input-only, no strapping, ADC1 so it works with
//  WiFi active). The divider ratio is treated as EXACTLY 0.5 in software; the
//  real 0.5049 (0.49% error) is intentionally NOT compensated so the firmware
//  stays identical across hardware units. No per-unit calibration constants.
//
//  esp_adc_cal uses the chip's eFuse factory calibration; 1100 is only the
//  fallback Vref for chips without eFuse data. analogRead() would bypass this,
//  so it is never used here.
//
//  The self-contained primitives live here (ADC setup, sampling, IIR). The
//  actual UVLO shutdown (blank the strip, tri-state the LED data pin, deep
//  sleep) lives in module.ino where the LED buffer and pin are — see loop().
// ═══════════════════════════════════════════════════════
#pragma once

#include <Arduino.h>
#include "driver/adc.h"
#include "esp_adc_cal.h"

// ── Parameters ──
#define BATT_PIN          34      // GPIO34 = ADC1_CHANNEL_6 (used below as the channel)
#define BATT_CHECK_MS     10000   // measurement interval
#define BATT_SAMPLES      8
#define BATT_THRESHOLD_MV 1650    // at the pin -> 3.30 V cell. (1500 -> 3.00 V cutoff)
#define BATT_CONFIRM      3       // consecutive breaches -> ~30s before shutdown
#define IIR_ALPHA         0.15f   // tau = -10s/ln(0.85) ~= 61s; ~3min to fully track a step

// ── State ──
esp_adc_cal_characteristics_t adc_chars;
float    battFiltered_mV = -1.0f;   // -1 = uninitialised sentinel (first sample adopted)
float    battVolt        = 0.0f;    // cell volts — exposed globally for UI/telemetry
int      battLowCount    = 0;
uint32_t lastBattCheck   = 0;
uint8_t  battMonEn       = 0;       // 0 = monitoring disabled (DEFAULT); NVS key "bmon".
                                    // Must default OFF: with no cell/divider the pin floats
                                    // and reads arbitrary values, which would otherwise
                                    // trigger a false shutdown during USB-only bench testing.

// Survives deep sleep in RTC RAM. No wake source is configured, so the device
// stays asleep until a REAL power cycle (supply fully removed) clears RTC RAM
// and resets this to false. A reset button or brownout is NOT sufficient — that
// is deliberate: it stops the device repeatedly waking and draining an empty cell.
RTC_DATA_ATTR bool batteryDead = false;

// Call once in setup(), before the first read.
static inline void battSetupAdc() {
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11);
  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11,
                           ADC_WIDTH_BIT_12, 1100, &adc_chars);
}

// Voltage at the PIN in mV. Averages the RAW counts first, then converts once.
// 2ms spacing is required for ADC settling; without it consecutive reads repeat.
// Blocks ~16ms total — acceptable because it runs only once per 10s.
static inline uint32_t readBattMv() {
  long sum = 0;
  for (int i = 0; i < BATT_SAMPLES; i++) {
    sum += adc1_get_raw(ADC1_CHANNEL_6);
    delay(2);
  }
  return esp_adc_cal_raw_to_voltage((int)(sum / BATT_SAMPLES), &adc_chars);
}

// One-pole IIR low-pass. Rejects LED load transients (~61s time constant).
// The first sample is adopted as-is so the filter primes without a ramp.
static inline float applyIIR(float new_mv) {
  if (battFiltered_mV < 0.0f) battFiltered_mV = new_mv;
  else battFiltered_mV = IIR_ALPHA * new_mv + (1.0f - IIR_ALPHA) * battFiltered_mV;
  return battFiltered_mV;
}
