#pragma once

// ─── WiFi Credentials ──────────────────────────────────────
// Fill in your network details before uploading.
#define WIFI_SSID     "your_wifi_ssid"
#define WIFI_PASSWORD "your_wifi_password"

// ─── LED Configuration ─────────────────────────────────────
// SK6812 RGBW uses a single data wire (no clock).
// Connect: SK6812 DIN → pin 6, VCC → 5 V supply, GND → common GND.
//
// FastLED ≥ 3.7.7 has native RGBW support via setRgbw().
// The sketch uses a standard CRGB array; FastLED's RGBWEmulatedController
// (configured via setRgbw) handles packing the 4th W byte transparently.
// COLOR_ORDER = GRB matches SK6812 GRBW wire format (G,R,B first three bytes).
// W3 tells FastLED the white byte is the 4th (last) byte per pixel.
#define NUM_LEDS      100
#define LED_DATA_PIN  6
#define LED_TYPE      SK6812
#define COLOR_ORDER   GRB

// ─── BLE ───────────────────────────────────────────────────
#define BLE_DEVICE_NAME  "NanoLED"
// Custom 128-bit service UUID (not a SIG-reserved UUID; avoids conflicts with
// ArduinoBLE's internally managed Generic Access Profile service 0x1800).
#define BLE_SERVICE_UUID "7ea7b78c-de64-4f0c-bdd9-20d9bd3289da"

// ─── Web Server ────────────────────────────────────────────
#define WEB_SERVER_PORT 80
