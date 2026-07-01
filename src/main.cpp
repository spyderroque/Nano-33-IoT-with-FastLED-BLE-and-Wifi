/*
 * Nano 33 IoT – SK6812 RGBW LED Controller
 *
 * Behaviour:
 *   • Advertises via BLE (name: NanoLED).
 *   • When a mobile device connects over BLE, the NINA module also connects
 *     to WiFi and starts a web server on port 80.
 *   • The web UI offers two modes:
 *       White – sets R=G=B=w (slider value); FastLED's kRGBWExactColors
 *               conversion extracts the common component into the W channel,
 *               driving the physical white LED with no RGB contribution.
 *       Color – drives R, G, B at chosen values; near-neutral mixes
 *               automatically blend in W for better quality.
 *     Both modes share a global Brightness slider (FastLED.setBrightness).
 *   • When the BLE device disconnects, WiFi is stopped to save power.
 *
 * Wiring:
 *   SK6812 DIN  → pin 6 (single data wire, no clock)
 *   SK6812 VCC  → 5 V dedicated supply (not the Nano's 5 V pin for >20 LEDs)
 *   SK6812 GND  → GND (common with Nano GND)
 *
 * FastLED RGBW:
 *   FastLED ≥ 3.7 supports SK6812 RGBW natively via setRgbw().
 *   A standard CRGB array is used; an RGBWEmulatedController (configured
 *   by setRgbw) handles packing the 4th W byte automatically.
 *     - kRGBWExactColors: min(R,G,B) is transferred to W; RGB is reduced
 *       by the same amount.  Gray/white tones drive the W LED cleanly.
 *     - W3: the W byte is the 4th byte per pixel (SK6812 GRBW wire order).
 *     - kRGBWDefaultColorTemp (6000 K): colour temperature used when
 *       computing how much of a colour to assign to the white channel.
 *
 * Build system: PlatformIO (see platformio.ini).  Dependencies (FastLED,
 * ArduinoBLE, WiFiNINA) are declared in lib_deps and fetched automatically.
 *   pio run -e nano_33_iot -t upload   # build + flash
 *   pio device monitor                 # serial monitor (115200 baud)
 *   pio test -e native                 # run host unit tests
 */

#include <Arduino.h>

#include <FastLED.h>
#include <ArduinoBLE.h>
#include <WiFiNINA.h>

#include "config.h"
#include "web_ui.h"
#include "param_parser.h"   // pure query-string parser (unit-tested on host)

// ─── LED setup ─────────────────────────────────────────────
CRGB leds[NUM_LEDS];

// ─── LED state ─────────────────────────────────────────────
struct State {
  uint8_t r          = 255;
  uint8_t g          = 255;
  uint8_t b          = 255;
  uint8_t w          = 255;   // white intensity (0–255)
  uint8_t brightness = 128;
  bool    whiteOnly  = true;
} state;

// ─── WiFi / server ─────────────────────────────────────────
WiFiServer server(WEB_SERVER_PORT);
bool wifiActive = false;

// ─── BLE ───────────────────────────────────────────────────
BLEService ledService(BLE_SERVICE_UUID);

// ─── LED helpers ───────────────────────────────────────────
void applyLeds() {
    CRGB color;
    if (state.whiteOnly) {
        // Equal R=G=B causes kRGBWExactColors to push the full value into the
        // W channel (min(w,w,w) = w → W LED = w, RGB = 0).
        color = CRGB(state.w, state.w, state.w);
    } else {
        color = CRGB(state.r, state.g, state.b);
    }
    fill_solid(leds, NUM_LEDS, color);
    FastLED.setBrightness(state.brightness);
    FastLED.show();
}

// ─── HTTP helpers ──────────────────────────────────────────
// parseParam() lives in lib/ParamParser so it can be unit-tested on the host
// without any Arduino dependency (see test/test_param_parser).

static void sendJson(WiFiClient &client) {
    client.print(F("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n"));
    client.print(F("{\"mode\":\""));
    client.print(state.whiteOnly ? F("white") : F("color"));
    client.print(F("\",\"w\":"));   client.print(state.w);
    client.print(F(",\"r\":"));     client.print(state.r);
    client.print(F(",\"g\":"));     client.print(state.g);
    client.print(F(",\"b\":"));     client.print(state.b);
    client.print(F(",\"br\":"));    client.print(state.brightness);
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

    String req;
    req.reserve(256);
    unsigned long deadline = millis() + 1500UL;

    while (client.connected() && millis() < deadline) {
        while (client.available() && req.length() < 256) {
            req += (char)client.read();
        }
        if (req.indexOf("\r\n\r\n") >= 0 || req.length() >= 256) break;
    }

    if (req.startsWith("GET /set")) {
        if      (req.indexOf("mode=white") >= 0) state.whiteOnly = true;
        else if (req.indexOf("mode=color") >= 0) state.whiteOnly = false;

        const char *r = req.c_str();
        int v;
        if ((v = parseParam(r, "w"))  >= 0) state.w          = (uint8_t)constrain(v, 0, 255);
        if ((v = parseParam(r, "r"))  >= 0) state.r          = (uint8_t)constrain(v, 0, 255);
        if ((v = parseParam(r, "g"))  >= 0) state.g          = (uint8_t)constrain(v, 0, 255);
        if ((v = parseParam(r, "bl")) >= 0) state.b          = (uint8_t)constrain(v, 0, 255);
        if ((v = parseParam(r, "br")) >= 0) state.brightness = (uint8_t)constrain(v, 0, 255);

        applyLeds();
        sendJson(client);

    } else if (req.startsWith("GET /")) {
        sendHtml(client);
    }

    delay(1);
    client.stop();
}

// ─── WiFi management ───────────────────────────────────────
void startWifi() {
    Serial.print(F("Connecting to WiFi "));
    Serial.print(F(WIFI_SSID));
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    for (int i = 0; i < 30 && WiFi.status() != WL_CONNECTED; i++) {
        delay(500);
        Serial.print('.');
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.print(F("\nConnected. Open http://"));
        Serial.println(WiFi.localIP());
        server.begin();
        wifiActive = true;
    } else {
        Serial.println(F("\nWiFi failed – web UI unavailable."));
    }
}

void stopWifi() {
    if (!wifiActive) return;
    server.end();
    WiFi.disconnect();
    wifiActive = false;
    Serial.println(F("WiFi stopped."));
}

// ─── Setup / Loop ──────────────────────────────────────────
void setup() {
    Serial.begin(115200);

    // Native RGBW support via setRgbw() – no cast tricks needed.
    // kRGBWExactColors: transfers min(R,G,B) to the W channel so that
    //   neutral/white tones drive the physical W LED, not the RGB mix.
    // W3: W byte is 4th in the GRBW stream (SK6812 wire order).
    FastLED.addLeds<LED_TYPE, LED_DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS)
           .setRgbw(Rgbw(kRGBWDefaultColorTemp, kRGBWExactColors, W3));

    FastLED.setBrightness(0);
    FastLED.clear();
    FastLED.show();

    if (!BLE.begin()) {
        Serial.println(F("BLE init failed – halting."));
        while (true) {}
    }
    BLE.setLocalName(BLE_DEVICE_NAME);
    BLE.setAdvertisedService(ledService);
    BLE.addService(ledService);
    BLE.advertise();
    Serial.println(F("BLE advertising as \"" BLE_DEVICE_NAME "\""));
    Serial.println(F("Connect a phone over BLE to activate the web UI."));
}

void loop() {
    BLEDevice central = BLE.central();
    if (!central) return;

    Serial.print(F("BLE connected: "));
    Serial.println(central.address());
    startWifi();

    while (central.connected()) {
        BLE.poll();
        if (wifiActive) handleClient();
    }

    Serial.println(F("BLE disconnected."));
    stopWifi();
    BLE.advertise();
}
