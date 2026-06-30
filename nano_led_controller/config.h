#pragma once

// ─── WiFi Credentials ──────────────────────────────────────
// Fill in your network details before uploading.
#define WIFI_SSID     "your_wifi_ssid"
#define WIFI_PASSWORD "your_wifi_password"

// ─── LED Configuration ─────────────────────────────────────
// SK6812 RGBW uses a single data wire (no clock).
// Connect: SK6812 DIN → pin 6, VCC → 5 V supply, GND → common GND.
//
// FastLED does not natively support 4-channel RGBW in addLeds<>, so we use
// the CRGBW-cast trick: a CRGBW[] array is reinterpreted as CRGB* and passed
// with getRGBWsize() as the LED count so that FastLED sends exactly
// NUM_LEDS * 4 raw bytes (= the GRBW stream the strip expects).
// COLOR_ORDER must be RGB so FastLED sends bytes in their memory order without
// reordering — the G↔R channel swap required by SK6812 wire format is handled
// explicitly inside applyLeds().
#define NUM_LEDS      100
#define LED_DATA_PIN  6
#define LED_TYPE      SK6812
#define COLOR_ORDER   RGB

// ─── BLE ───────────────────────────────────────────────────
#define BLE_DEVICE_NAME  "NanoLED"
// Generic Access Profile service UUID (16-bit 0x1800 expanded to 128-bit)
#define BLE_SERVICE_UUID "00001800-0000-1000-8000-00805f9b34fb"

// ─── Web Server ────────────────────────────────────────────
#define WEB_SERVER_PORT 80
