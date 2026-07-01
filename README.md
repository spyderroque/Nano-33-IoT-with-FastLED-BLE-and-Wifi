# Nano 33 IoT – SK6812 RGBW LED Controller (FastLED + BLE + WiFi)

Control up to 100 SK6812 RGBW LEDs from any phone browser.  
BLE acts as the on/off switch for WiFi: the web UI is only reachable while a phone is connected over Bluetooth.

Built with **[PlatformIO](https://platformio.org/)** — the pure request-parsing
logic is unit-tested on the host, the rest is flashed to the board.

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

> **SK6812 RGBW** Each package contains three RGB LEDs **plus** a fourth warm-white LED driven by the W byte. White mode here uses only the W channel for the most natural and efficient white light output.

> **FastLED RGBW** FastLED ≥ 3.7 supports SK6812 RGBW natively. The sketch uses a standard `CRGB` array and calls `.setRgbw(Rgbw(kRGBWDefaultColorTemp, kRGBWExactColors, W3))` on `addLeds<>`. FastLED's `RGBWEmulatedController` handles packing the 4th W byte automatically — no cast tricks or manual channel swapping needed. `kRGBWExactColors` transfers `min(R,G,B)` to the W channel so neutral/white tones drive the physical white LED instead of the RGB mix.

---

## Installation

### 1. Install PlatformIO

Choose one:

- **VS Code (recommended):** install [VS Code](https://code.visualstudio.com/),
  then the **PlatformIO IDE** extension from the marketplace.
- **Command line:** install PlatformIO Core (`pio`):
  ```bash
  python3 -m pip install --upgrade platformio
  ```
  Verify with `pio --version`.

### 2. Get the code

```bash
git clone https://github.com/spyderroque/Nano-33-IoT-with-FastLED-BLE-and-Wifi.git
cd Nano-33-IoT-with-FastLED-BLE-and-Wifi
```

The board toolchain and the libraries (FastLED, ArduinoBLE, WiFiNINA) listed in
`platformio.ini` under `lib_deps` are downloaded automatically on the first
build — no manual library installation required.

### 3. Configure your WiFi

Edit `src/config.h`:

```cpp
#define WIFI_SSID     "your_wifi_ssid"
#define WIFI_PASSWORD "your_wifi_password"
```

Adjust the LED count if needed (default 100):

```cpp
#define NUM_LEDS 100
```

### 4. Build and upload

Connect the Nano 33 IoT over USB, then:

```bash
pio run -e nano_33_iot -t upload
```

In VS Code you can instead click the **PlatformIO: Upload** (→) button in the
status bar.

### 5. Open the serial monitor

```bash
pio device monitor
```

(115200 baud — configured in `platformio.ini`.) It prints the BLE status and,
once a phone connects, the board's IP address.

---

## Usage

1. Flash the sketch (see above) and open the Serial Monitor.
2. On your phone, connect to the Bluetooth device **NanoLED**.  
   *(No pairing PIN needed – the BLE service carries no characteristics; the connection itself is the trigger.)*
3. The Serial Monitor prints the board's IP address, e.g. `http://192.168.1.42`.
4. Open that address in your phone browser.
5. Use the **White** / **Color** toggle and sliders. Changes take effect in real time.
6. Disconnect Bluetooth → WiFi stops automatically to save power.

---

## Tests

The query-string parser (`parseParam`) is isolated in `lib/ParamParser` with no
Arduino dependency, so it can be verified on your computer without a board:

```bash
pio test -e native
```

This runs the [Unity](https://docs.platformio.org/en/latest/plus/unit-testing.html)
suite in `test/test_param_parser/`, which covers single/multiple parameters,
empty and boundary values, `?`/`&` delimiting, and specifically the regression
where a short key (`r`) must **not** be matched inside a longer one (`br`).

---

## Project Structure

```
.
├── platformio.ini                 # Build envs: nano_33_iot (board) + native (tests)
├── src/
│   ├── main.cpp                   # Main firmware (BLE → WiFi → web server → LEDs)
│   ├── config.h                   # WiFi credentials & LED/BLE settings
│   └── web_ui.h                   # Minified HTML+CSS+JS stored in flash
├── lib/
│   └── ParamParser/               # Pure, host-testable query-string parser
│       ├── param_parser.h
│       └── param_parser.cpp
└── test/
    └── test_param_parser/         # Unity unit tests (env: native)
        └── test_param_parser.cpp
```

---

## Libraries

Declared in `platformio.ini` and fetched automatically:

| Library | Constraint |
|---|---|
| FastLED | `^3.7.0` (native RGBW `setRgbw` support) |
| ArduinoBLE | `^1.3.0` |
| WiFiNINA | `^1.8.0` |
