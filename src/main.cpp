/*
 * Nano 33 IoT – SK6812 RGBW LED Controller (WiFi only, low power)
 *
 * LED driver: Adafruit_NeoPixel.
 *   SK6812 RGBW is supported natively — no emulation, no colour-order tricks:
 *     • White mode → strip.Color(0, 0, 0, w): drives ONLY the physical W LED.
 *     • Color mode → strip.Color(r, g, b, 0): drives the RGB LEDs, W off.
 *   The NEO_GRBW flag (config.h) tells the library the wire order; Color() is
 *   always logical (r, g, b, w), so there is nothing to reorder by hand.
 *
 * Behaviour:
 *   • Connects to WiFi at boot and stays permanently connected; reconnects by
 *     itself after a power loss / router reboot.
 *   • Web UI on port 80: Power on/off switch, White / Color modes, Brightness.
 *   • No Bluetooth — no NINA WiFi/BLE coexistence limits, no special firmware.
 *
 * Power notes:
 *   • WiFi.lowPowerMode() puts the NINA radio in beacon power-save while online.
 *   • "Off" writes every LED to 0, so the strip drops to quiescent current.
 *   • No explicit <Arduino.h>; the core is pulled in by the libraries. Request
 *     handling uses a fixed char buffer instead of String (no heap).
 *
 * Wiring:  SK6812 DIN → pin 6,  VCC → 5 V supply,  GND → common GND.
 *   NOTE: the Nano 33 IoT drives DIN at 3.3 V. If colours glitch, add a 3.3→5 V
 *   level shifter on DIN (see README) — that is hardware, not code.
 *
 * Build (PlatformIO):
 *   pio run -e nano_33_iot -t upload   # build + flash
 *   pio device monitor                 # serial monitor (115200 baud)
 *   pio test -e native                 # host unit tests (parser)
 */

#include <Adafruit_NeoPixel.h>       // native RGBW; defines NEO_GRBW / NEO_KHZ800
#include <WiFiNINA.h>
#include <string.h>                  // strstr, strncmp — C stdlib, no Arduino String

#include "config.h"
#include "web_ui.h"
#include "param_parser.h"

// ─── LEDs ──────────────────────────────────────────────────
Adafruit_NeoPixel strip(NUM_LEDS, LED_DATA_PIN, LED_PIXEL_TYPE);

struct State {
  bool    on         = POWER_ON_AT_BOOT;   // master on/off switch
  bool    whiteOnly  = true;
  uint8_t w          = 255;                // white intensity (0–255)
  uint8_t r          = 255;
  uint8_t g          = 255;
  uint8_t b          = 255;
  uint8_t brightness = 128;
} state;

WiFiServer server(WEB_SERVER_PORT);

// ─── helpers ───────────────────────────────────────────────
static inline uint8_t clamp8(int v) {
    return v < 0 ? 0 : (v > 255 ? 255 : (uint8_t)v);
}

void applyLeds() {
    uint32_t c;
    if (!state.on) {
        c = 0;                                          // off → all channels 0
    } else if (state.whiteOnly) {
        c = strip.Color(0, 0, 0, state.w);              // physical white LED only
    } else {
        c = strip.Color(state.r, state.g, state.b, 0);  // RGB LEDs, white off
    }
    strip.setBrightness(state.brightness);
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
        strip.setPixelColor(i, c);
    }
    strip.show();
}

// ─── HTTP ──────────────────────────────────────────────────
static void sendJson(WiFiClient &client) {
    client.print(F("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n"));
    client.print(F("{\"on\":"));     client.print(state.on ? 1 : 0);
    client.print(F(",\"mode\":\"")); client.print(state.whiteOnly ? F("white") : F("color"));
    client.print(F("\",\"w\":"));    client.print(state.w);
    client.print(F(",\"r\":"));      client.print(state.r);
    client.print(F(",\"g\":"));      client.print(state.g);
    client.print(F(",\"b\":"));      client.print(state.b);
    client.print(F(",\"br\":"));     client.print(state.brightness);
    client.print(F("}"));
}

static void sendHtml(WiFiClient &client) {
    client.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n"));
    client.print(HTML_P1);
    client.print(HTML_P2);
}

void handleClient() {
    WiFiClient client = server.available();
    if (!client) return;

    char req[256];
    size_t len = 0;
    unsigned long deadline = millis() + 1500UL;

    while (client.connected() && millis() < deadline && len < sizeof(req) - 1) {
        while (client.available() && len < sizeof(req) - 1) {
            req[len++] = (char)client.read();
        }
        req[len] = '\0';
        if (strstr(req, "\r\n\r\n")) break;     // end of request headers
    }
    req[len] = '\0';

    if (strncmp(req, "GET /set", 8) == 0) {
        if      (strstr(req, "mode=white")) state.whiteOnly = true;
        else if (strstr(req, "mode=color")) state.whiteOnly = false;

        int v;
        if ((v = parseParam(req, "on")) >= 0) state.on         = (v != 0);
        if ((v = parseParam(req, "w"))  >= 0) state.w          = clamp8(v);
        if ((v = parseParam(req, "r"))  >= 0) state.r          = clamp8(v);
        if ((v = parseParam(req, "g"))  >= 0) state.g          = clamp8(v);
        if ((v = parseParam(req, "bl")) >= 0) state.b          = clamp8(v);
        if ((v = parseParam(req, "br")) >= 0) state.brightness = clamp8(v);

        applyLeds();
        sendJson(client);

    } else if (strncmp(req, "GET /status", 11) == 0) {
        sendJson(client);                        // poll: state only, no LED write

    } else if (strncmp(req, "GET /", 5) == 0) {
        sendHtml(client);
    }

    delay(1);                                    // let the response flush
    client.stop();
}

// ─── WiFi (permanent, auto-reconnect) ──────────────────────
// Returns true while associated. On a dropped link it re-runs WiFi.begin(),
// so the board rejoins the network by itself after a router or power outage.
bool ensureWifi() {
    if (WiFi.status() == WL_CONNECTED) return true;

    Serial.print(F("WiFi: connecting to "));
    Serial.println(F(WIFI_SSID));
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    unsigned long deadline = millis() + 15000UL;
    while (WiFi.status() != WL_CONNECTED && millis() < deadline) {
        delay(250);
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println(F("WiFi: connect failed – will retry."));
        return false;
    }

    if (WIFI_LOW_POWER) {
        WiFi.lowPowerMode();     // NINA beacon power-save while staying online
    }
    server.begin();
    Serial.print(F("WiFi: connected – http://"));
    Serial.println(WiFi.localIP());
    return true;
}

// ─── Setup / Loop ──────────────────────────────────────────
void setup() {
    Serial.begin(115200);

    strip.begin();
    applyLeds();                 // show the boot state (POWER_ON_AT_BOOT)

    ensureWifi();
}

void loop() {
    if (ensureWifi()) {
        handleClient();
    } else {
        delay(2000);             // back off before the next reconnect attempt
    }
}
