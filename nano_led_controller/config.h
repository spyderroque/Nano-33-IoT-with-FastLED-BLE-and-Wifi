#pragma once

// ─── WiFi Credentials ──────────────────────────────────────
// Fill in your network details before uploading.
#define WIFI_SSID     "your_wifi_ssid"
#define WIFI_PASSWORD "your_wifi_password"

// ─── LED Configuration ─────────────────────────────────────
// SK9822 uses hardware SPI: DATA → pin 11 (MOSI), CLOCK → pin 13 (SCK)
// Nano 33 IoT's NINA module uses a separate internal SPI bus → no conflict.
#define NUM_LEDS      100
#define LED_DATA_PIN  11    // MOSI
#define LED_CLOCK_PIN 13    // SCK
#define LED_TYPE      SK9822
#define COLOR_ORDER   BGR   // SK9822 wire order is BGR

// ─── BLE ───────────────────────────────────────────────────
#define BLE_DEVICE_NAME  "NanoLED"
// Generic Access Profile service UUID (16-bit 0x1800 expanded to 128-bit)
#define BLE_SERVICE_UUID "00001800-0000-1000-8000-00805f9b34fb"

// ─── Web Server ────────────────────────────────────────────
#define WEB_SERVER_PORT 80
