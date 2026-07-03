# Nano 33 IoT – SK6812 RGBW LED Controller (WiFi only, low power)

Control up to 100 SK6812 RGBW LEDs from any phone browser over WiFi.  
The board stays **permanently connected** to your network, **reconnects by itself**
after a power loss or router reboot, and the web UI has a **Power on/off switch**
plus White / Color modes and a Brightness slider.

No Bluetooth — which also means **no NINA firmware juggling** and no WiFi/BLE
coexistence limits. Built with **[PlatformIO](https://platformio.org/)**; the
request parser is unit-tested on the host.

---

## Features

| Feature | Detail |
|---|---|
| **Power switch** | On/off toggle in the web UI. "Off" drives every LED to black |
| **Always online** | Connects at boot and stays associated; the UI is always reachable |
| **Auto-reconnect** | Rejoins the network automatically after a power loss / router reboot |
| **White mode** | Drives the SK6812 **dedicated W LED** only – the truest white |
| **Colour mode** | Individual R / G / B sliders |
| **Brightness** | Global FastLED brightness across all channels |
| **Low power** | NINA WiFi power-save mode; lean, `String`-free request handling |

---

## Hardware

| SK6812 pin | Nano 33 IoT pin |
|---|---|
| DIN (data in) | D6 (single data wire — no clock needed) |
| VCC | External 5 V supply (use a dedicated PSU for ≥ 20 LEDs) |
| GND | GND (common with Nano GND) |

> **SK6812 RGBW** Each package contains three RGB LEDs **plus** a fourth warm-white LED driven by the W byte. White mode uses only the W channel for the most natural, efficient white.

> **FastLED RGBW** FastLED ≥ 3.7 supports SK6812 natively. The sketch uses a `CRGB` array and `.setRgbw(Rgbw(kRGBWDefaultColorTemp, kRGBWExactColors, W3))`; `kRGBWExactColors` moves `min(R,G,B)` to the physical white LED.

---

## Low power & minimal overhead

This build is deliberately lean:

- **`WiFi.lowPowerMode()`** puts the NINA radio into beacon power-save while
  staying associated — the single biggest idle-power saving on this board.
  Toggle it with `WIFI_LOW_POWER` in `config.h`.
- **"Off" turns the LEDs black**, so a 100-LED strip drops to quiescent current.
- **No Bluetooth stack** is linked at all (ArduinoBLE removed).
- **No `String`**: HTTP requests are parsed in a fixed `char` buffer (no heap
  churn), and the parser (`lib/ParamParser`) is plain C on `const char*`.

> **About `Arduino.h`:** it isn't `#include`d explicitly, but the Arduino core
> (`millis`, `Serial`, the SPI/GPIO used to reach the NINA and drive the LEDs)
> is pulled in by FastLED/WiFiNINA and **cannot be avoided** while using those
> libraries — removing the include line wouldn't change what gets linked. Truly
> bare-metal would mean reimplementing the NINA WiFi protocol and the SK6812
> timing by hand. The real savings above are where the power actually goes.

For deeper sleep you'd need to stop serving continuously (e.g. deep-sleep the
SAMD21 and wake on a button/RTC), which is incompatible with an always-reachable
web server — hence the focus on the radio and the LEDs.

---

## Installation

### 1. Install PlatformIO

- **VS Code (recommended):** install [VS Code](https://code.visualstudio.com/),
  then the **PlatformIO IDE** extension.
- **CLI:** `python3 -m pip install --upgrade platformio` (verify with `pio --version`).

### 2. Get the code

```bash
git clone https://github.com/spyderroque/Nano-33-IoT-with-FastLED-BLE-and-Wifi.git
cd Nano-33-IoT-with-FastLED-BLE-and-Wifi
```

FastLED and WiFiNINA (in `platformio.ini` → `lib_deps`) download automatically
on the first build.

### 3. Configure

Edit `src/config.h`:

```cpp
#define WIFI_SSID     "your_wifi_ssid"
#define WIFI_PASSWORD "your_wifi_password"

#define NUM_LEDS         100
#define POWER_ON_AT_BOOT true    // lights return automatically after a power loss
#define WIFI_LOW_POWER   true    // NINA beacon power-save (recommended)
```

### 4. Build, upload, and find the IP

```bash
pio run -e nano_33_iot -t upload
pio device monitor            # 115200 baud — prints "WiFi: connected – http://<ip>"
```

(You can also read the IP from your router's client list. Assigning the board a
DHCP reservation gives it a stable address.)

---

## Usage

1. Open `http://<ip>` from any browser on the same network.
2. Use the **Power** switch to turn the lights on/off.
3. Use **White** / **Color** and the **Brightness** slider; changes apply live.

The board stays connected, so the page is always available. After a power outage
it rejoins WiFi on its own and comes back in the `POWER_ON_AT_BOOT` state.

---

## Tests

The query-string parser is isolated in `lib/ParamParser` (plain C, no Arduino),
so it runs on your computer without a board:

```bash
pio test -e native
```

The [Unity](https://docs.platformio.org/en/latest/plus/unit-testing.html) suite
covers multiple/empty/boundary values, `?`/`&` delimiting, the `br`-vs-`r`
regression, and the on/off switch (`on=0` must return `0`, not "absent").

---

## Project Structure

```
.
├── platformio.ini                 # Build envs: nano_33_iot (board) + native (tests)
├── src/
│   ├── main.cpp                   # WiFi-only firmware (server → LEDs, auto-reconnect)
│   ├── config.h                   # WiFi credentials, LED config, power options
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

Declared in `platformio.ini`, fetched automatically:

| Library | Constraint |
|---|---|
| FastLED | `^3.7.0` (native RGBW `setRgbw` support) |
| WiFiNINA | `^2.1.0` |

No ArduinoBLE and no special NINA firmware are required for this WiFi-only build.
