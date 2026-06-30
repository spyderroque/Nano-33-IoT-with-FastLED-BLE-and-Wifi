# Nano 33 IoT – SK6812 RGBW LED Controller (FastLED + BLE + WiFi)

Control up to 100 SK6812 RGBW LEDs from any phone browser.  
BLE acts as the on/off switch for WiFi: the web UI is only reachable while a phone is connected over Bluetooth.

---

## Features

| Feature | Detail |
|---|---|
| **White mode** | Drives the SK6812 **dedicated W LED** only (RGB off) – the truest white |
| **Colour mode** | Individual R / G / B sliders (W LED off) |
| **Brightness** | Global FastLED brightness, scales all four channels uniformly |
| **Power saving** | WiFi starts only when a BLE device connects; stops on disconnect |

---

## Hardware

| SK6812 pin | Nano 33 IoT pin |
|---|---|
| DIN (data in) | D6 (single data wire — no clock needed) |
| VCC | External 5 V supply (use a dedicated PSU for ≥ 20 LEDs) |
| GND | GND (common with Nano GND) |

> **SK6812 RGBW** Each package contains three RGB LEDs **plus** a fourth warm-white LED driven by the W byte.  White mode here uses only the W channel for the most natural and efficient white light output.

> **FastLED CRGBW cast trick** FastLED has no native 4-channel `addLeds<>` for SK6812 RGBW.  The sketch uses a `CRGBW[]` array cast to `CRGB*` with a byte-count calculated by `getRGBWsize()`, so FastLED sends exactly `NUM_LEDS × 4` bytes in the correct GRBW wire order.

---

## Required Libraries

Install all three via **Arduino → Tools → Manage Libraries**:

| Library | Minimum version |
|---|---|
| FastLED | 3.6 |
| ArduinoBLE | 1.3 |
| WiFiNINA | 1.8 |

---

## Configuration

Open `nano_led_controller/config.h` and fill in your credentials:

```cpp
#define WIFI_SSID     "your_wifi_ssid"
#define WIFI_PASSWORD "your_wifi_password"
```

Adjust LED count if needed:

```cpp
#define NUM_LEDS 100
```

---

## Usage

1. Flash the sketch to the Nano 33 IoT.
2. Open the **Serial Monitor** (115200 baud) to see status messages.
3. On your phone, connect to Bluetooth device **NanoLED**.  
   *(No pairing PIN needed – the BLE service carries no characteristics; connection itself is the trigger.)*
4. The Serial Monitor will print the board's IP address, e.g. `http://192.168.1.42`.
5. Open that address in your phone browser.
6. Use the **White** / **Color** toggle and sliders. Changes take effect in real time.
7. Disconnect Bluetooth → WiFi stops automatically.

---

## Project Structure

```
nano_led_controller/
├── nano_led_controller.ino   # Main sketch
├── config.h                  # WiFi credentials & LED/BLE settings
└── web_ui.h                  # Minified HTML+CSS+JS stored in flash
```
