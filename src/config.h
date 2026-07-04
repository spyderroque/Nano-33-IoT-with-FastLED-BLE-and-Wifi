#pragma once

// ─── WiFi Credentials ──────────────────────────────────────
// Fill in your network details before uploading.
#define WIFI_SSID     "your_wifi_ssid"
#define WIFI_PASSWORD "your_wifi_password"

// ─── LED Configuration ─────────────────────────────────────
// SK6812 RGBW, single data wire: DIN → pin 6, VCC → 5 V supply, GND → common GND.
// Driven by Adafruit_NeoPixel, which supports RGBW natively (real W channel) —
// no emulation or colour-order gymnastics needed.
//   NEO_GRBW   = SK6812 RGBW byte order (G,R,B,W). If red/green look swapped,
//                try NEO_RGBW instead.
//   NEO_KHZ800 = 800 kHz data rate (SK6812 / WS2812).
// (NEO_* come from <Adafruit_NeoPixel.h>, which main.cpp includes before this.)
#define NUM_LEDS       100
#define LED_DATA_PIN   6
#define LED_PIXEL_TYPE (NEO_GRBW + NEO_KHZ800)

// ─── Web Server ────────────────────────────────────────────
#define WEB_SERVER_PORT 80

// ─── Behaviour / Power ─────────────────────────────────────
// Light state the board comes up in after boot / a power loss.
#define POWER_ON_AT_BOOT true
// NINA WiFi power-save: radio sleeps between beacons but stays associated.
#define WIFI_LOW_POWER   true
