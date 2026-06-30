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
// Generic Access Profile service UUID (16-bit 0x1800 expanded to 128-bit)
#define BLE_SERVICE_UUID "00001800-0000-1000-8000-00805f9b34fb"

// ─── Web Server ────────────────────────────────────────────
#define WEB_SERVER_PORT 80
