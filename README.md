# Nano 33 IoT – SK9822 LED Controller (FastLED + BLE + WiFi)

Control up to 100 SK9822 RGB LEDs from any phone browser.  
BLE acts as the on/off switch for WiFi: the web UI is only reachable while a phone is connected over Bluetooth.

---

## Features

| Feature | Detail |
|---|---|
| **White mode** | Single slider drives R = G = B (warm-white effect) |
| **Colour mode** | Individual R / G / B sliders |
| **Brightness** | Global FastLED brightness, independent of colour |
| **Power saving** | WiFi starts only when a BLE device connects; stops on disconnect |

---

## Hardware

| SK9822 pin | Nano 33 IoT pin |
|---|---|
| DATA | D11 (MOSI / SPI) |
| CLOCK | D13 (SCK / SPI) |
| VCC | External 5 V supply (use a dedicated PSU for ≥ 20 LEDs) |
| GND | GND (common with Nano GND) |

> **Note** The Nano 33 IoT's NINA-W102 module uses an *internal* SPI bus, not the header pins, so there is no conflict with FastLED's SPI usage on D11/D13.

> **RGBW note** SK9822 LEDs are RGB + a 5-bit per-LED global brightness channel. They are not natively 4-channel RGBW. "White" here means driving R, G, B equally. If you have a strip with a true dedicated white LED, you will need additional driver circuitry and custom code beyond FastLED.

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
