#pragma once

// ─── WiFi Credentials ──────────────────────────────────────
// Fill in your network details before uploading.
#define WIFI_SSID     "your_wifi_ssid"
#define WIFI_PASSWORD "your_wifi_password"

// ─── LED Configuration ─────────────────────────────────────
// SK6812 RGBW, single data wire: DIN → pin 6, VCC → 5 V supply, GND → common GND.
// FastLED ≥ 3.7 native RGBW via setRgbw(). COLOR_ORDER = GRB matches the GRBW
// wire order; W3 = the white byte is the 4th (last) byte per pixel.
#define NUM_LEDS      100
#define LED_DATA_PIN  6
#define LED_TYPE      SK6812
#define COLOR_ORDER   GRB

// ─── Web Server ────────────────────────────────────────────
#define WEB_SERVER_PORT 80

// ─── Behaviour / Power ─────────────────────────────────────
// Light state the board comes up in after boot / a power loss.
// true  = lights return automatically (typical for a light fixture).
#define POWER_ON_AT_BOOT true
// Enable the NINA WiFi power-save mode: the radio sleeps between beacons but
// stays associated. This is the largest idle-power saving on this board.
#define WIFI_LOW_POWER   true
