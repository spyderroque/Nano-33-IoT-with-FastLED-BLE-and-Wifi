/*
 * Nano 33 IoT – SK6812 RGBW LED Controller (WiFi only, low power)
 *
 * Behaviour:
 *   • Connects to WiFi at boot and stays permanently connected. If the link
 *     drops (router reboot, power loss, range) it reconnects automatically.
 *   • Serves a minimal web UI on port 80: a Power on/off switch, White / Color
 *     modes, and a Brightness slider.
 *   • No Bluetooth. Dropping BLE removes the NINA WiFi+BLE coexistence limits
 *     entirely and needs no special module firmware.
 *
 * Power notes:
 *   • WiFi.lowPowerMode() puts the NINA radio in beacon power-save while staying
 *     associated — the biggest idle-power saving on this board.
 *   • "Off" drives every LED to black, so the strip draws only quiescent current.
 *   • The Arduino core (millis/Serial/…) is pulled in by FastLED/WiFiNINA and is
 *     unavoidable when using those libraries; there is no explicit <Arduino.h>.
 *     Request handling uses a fixed char buffer instead of String (no heap).
 *
 * Wiring:  SK6812 DIN → pin 6,  VCC → 5 V supply,  GND → common GND.
 *
 * Build (PlatformIO):
 *   pio run -e nano_33_iot -t upload   # build + flash
 *   pio device monitor                 # serial monitor (115200 baud)
 *   pio test -e native                 # host unit tests (parser)
 */

#include <FastLED.h>
#include <WiFiNINA.h>
#include <string.h>            // strstr, strncmp — C stdlib, no Arduino String

#include "config.h"
#include "web_ui.h"
#include "param_parser.h"

// ─── LED state ─────────────────────────────────────────────
CRGB leds[NUM_LEDS];

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
    CRGB color;
    if (!state.on) {
        color = CRGB::Black;                       // off → LEDs draw ~quiescent
    } else if (state.whiteOnly) {
        color = CRGB(state.w, state.w, state.w);   // equal RGB → W channel
    } else {
        color = CRGB(state.r, state.g, state.b);
    }
    fill_solid(leds, NUM_LEDS, color);
    FastLED.setBrightness(state.brightness);
    FastLED.show();
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

    FastLED.addLeds<LED_TYPE, LED_DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS)
           .setRgbw(Rgbw(kRGBWDefaultColorTemp, kRGBWExactColors, W3));
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
