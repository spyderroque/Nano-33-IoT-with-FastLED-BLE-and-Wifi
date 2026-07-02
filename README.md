# Nano 33 IoT – SK6812 RGBW LED Controller (FastLED + BLE + WiFi)

Control up to 100 SK6812 RGBW LEDs from any phone browser.  
BLE acts as the on switch for WiFi: connecting a phone over Bluetooth brings the
web UI up for **5 minutes**, then WiFi switches off again to save power. The UI
shows a live **mm:ss countdown** to that shutdown. WiFi keeps serving for the
whole window even if Bluetooth drops, and the board's `http://<ip>` URL is
pushed to the phone over a BLE characteristic — no Serial Monitor needed.

Built with **[PlatformIO](https://platformio.org/)** — the pure request-parsing
logic is unit-tested on the host, the rest is flashed to the board.

---

## Features

| Feature | Detail |
|---|---|
| **White mode** | Drives the SK6812 **dedicated W LED** only (RGB off) – the truest white |
| **Colour mode** | Individual R / G / B sliders (W LED off) |
| **Brightness** | Global FastLED brightness, scales all four channels uniformly |
| **Power saving** | WiFi starts on BLE connect and auto-stops **only** when the `WIFI_ON_SECONDS` countdown ends (default 5 min); a BLE disconnect no longer stops it |
| **Countdown** | Web UI shows a live mm:ss countdown to the WiFi shutdown |
| **IP over BLE** | The web URL `http://<ip>` is pushed to a read/notify BLE characteristic, so the phone gets the address directly |

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

> ⚠️ **Simultaneous WiFi + BLE requires up-to-date firmware.** This project keeps
> a BLE connection alive *while* serving the web UI over WiFi. On the NINA-W102
> module that is only possible with **NINA firmware ≥ 3.0.1** together with
> **WiFiNINA 2.0.0** and **ArduinoBLE 2.0.0** (both pinned in `platformio.ini`).
> Update the module firmware once via the Arduino IDE (**Tools → Firmware
> Updater**, or the standalone [`arduino-fwuploader`](https://github.com/arduino/arduino-fwuploader))
> before flashing. With older firmware WiFi and BLE cannot run at the same time.

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

Adjust the LED count or the WiFi on-time if needed (defaults 100 LEDs / 5 min):

```cpp
#define NUM_LEDS        100
#define WIFI_ON_SECONDS 300   // how long WiFi stays up after a BLE connection
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

1. Flash the sketch (see above).
2. On your phone, connect to the Bluetooth device **NanoLED** — e.g. with a
   generic BLE app (nRF Connect / LightBlue) or your own app. The connection
   itself is the trigger; no pairing PIN is required.
3. Read the board's URL over BLE: the **IP characteristic** (read + notify,
   UUID `…b78d…`) holds `http://<ip>` and is pushed the moment WiFi connects.
   The URL is also printed to the Serial Monitor, e.g. `http://192.168.1.42`.
4. Open that address in your phone browser.
5. Use the **White** / **Color** toggle and sliders. Changes take effect in real time.
6. The badge at the top counts down **mm:ss** until WiFi shuts off (5 min after
   connecting). WiFi keeps serving for the whole window **even if you disconnect
   Bluetooth**. When the countdown reaches `00:00` the web UI goes offline —
   reconnect Bluetooth to start a fresh 5-minute window.

> **Opening the page directly.** A generic BLE app displays the `http://<ip>`
> value so you can tap/copy it. For true one-tap open, a small companion app or
> a [Web Bluetooth](https://developer.mozilla.org/docs/Web/API/Web_Bluetooth_API)
> page (Chrome on Android) can read the characteristic and follow the link — the
> device-side prerequisite (the characteristic) is already in place.

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
| ArduinoBLE | `^2.1.0` (required for simultaneous WiFi + BLE) |
| WiFiNINA | `^2.1.0` (required for simultaneous WiFi + BLE; adds `WiFiServer::end()`) |

The `^` (caret) means "latest release below the next major", so each library
auto-updates within its major line — e.g. `^2.1.0` resolves to the newest 2.x
(currently 2.1.0) but never to a potentially breaking 3.0.0.

> Simultaneous WiFi + BLE landed in the `2.0.0` releases (March 2026), which
> moved BLE onto the SPI bus so both can share the NINA radio. They require
> **NINA firmware ≥ 3.0.1** — see the note at the top of [Installation](#installation).
