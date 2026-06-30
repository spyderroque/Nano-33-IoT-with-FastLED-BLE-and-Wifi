/*
 * Nano 33 IoT – SK6812 RGBW LED Controller
 *
 * Behaviour:
 *   • Advertises via BLE (name: NanoLED).
 *   • When a mobile device connects over BLE, the NINA module also connects
 *     to WiFi and starts a web server on port 80.
 *   • The web UI offers two modes:
 *       White – drives the dedicated SK6812 W LED at the chosen intensity
 *               (RGB channels off); this gives the truest white light.
 *       Color – drives R, G, B channels at chosen values (W channel off).
 *     Both modes share a global Brightness slider.
 *   • When the BLE device disconnects, WiFi is stopped to save power.
 *
 * Wiring:
 *   SK6812 DIN  → pin 6 (single data wire, no clock)
 *   SK6812 VCC  → 5 V dedicated supply (not the Nano's 5 V pin for >20 LEDs)
 *   SK6812 GND  → GND (common with Nano GND)
 *
 * RGBW / FastLED notes:
 *   FastLED does not have a native 4-channel RGBW addLeds<> variant for
 *   SK6812.  We use the CRGBW-cast workaround:
 *     - Declare CRGBW leds[NUM_LEDS].
 *     - Reinterpret as CRGB* and pass getRGBWsize(NUM_LEDS) as the count,
 *       so FastLED sends exactly NUM_LEDS * 4 bytes to the strip.
 *     - Use COLOR_ORDER = RGB so the byte stream is not reordered by FastLED
 *       (reordering would break the 4-byte-per-pixel alignment).
 *     - SK6812 RGBW wire order is G, R, B, W.  CRGBW memory order is
 *       {r, g, b, w}.  The applyLeds() function accounts for this by storing
 *       the user's G value in leds[i].r and R value in leds[i].g.
 *   FastLED.setBrightness() scales all bytes uniformly, which is correct.
 *   FastLED.setCorrection(UncorrectedColor) disables colour-temperature
 *   correction that would otherwise distort the W channel.
 *
 * Required libraries (install via Arduino Library Manager):
 *   • FastLED    ≥ 3.6
 *   • ArduinoBLE ≥ 1.3
 *   • WiFiNINA   ≥ 1.8
 */

#include <FastLED.h>
#include <ArduinoBLE.h>
#include <WiFiNINA.h>

#include "config.h"
#include "web_ui.h"

// ─── LED setup ─────────────────────────────────────────────
// CRGBW-cast trick: 4-byte per LED, reinterpreted as CRGB* for FastLED.
CRGBW leds[NUM_LEDS];
CRGB *ledsRGB = (CRGB *)&leds[0];

// FastLED needs to know how many 3-byte CRGB elements cover NUM_LEDS * 4 bytes.
static uint16_t getRGBWsize(uint16_t n) {
    uint32_t nb = (uint32_t)n * 4;
    return (uint16_t)((nb % 3 == 0) ? nb / 3 : nb / 3 + 1);
}

// ─── LED state ─────────────────────────────────────────────
struct State {
  uint8_t r          = 255;
  uint8_t g          = 255;
  uint8_t b          = 255;
  uint8_t w          = 255;   // white channel intensity (SK6812 W LED)
  uint8_t brightness = 128;
  bool    whiteOnly  = true;
} state;

// ─── WiFi / server ─────────────────────────────────────────
WiFiServer server(WEB_SERVER_PORT);
bool wifiActive = false;

// ─── BLE ───────────────────────────────────────────────────
BLEService ledService(BLE_SERVICE_UUID);

// ─── LED helpers ───────────────────────────────────────────

// Set all LEDs to the given RGBW values.
// SK6812 wire order is G, R, B, W but CRGBW memory is {r, g, b, w}.
// With FastLED RGB byte order the stream is sent as-is, so:
//   leds[i].r byte → SK6812 G channel
//   leds[i].g byte → SK6812 R channel
//   leds[i].b byte → SK6812 B channel
//   leds[i].w byte → SK6812 W channel
// Swap R↔G when storing to compensate.
static void fillLeds(uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
        leds[i].r = g;   // → SK6812 G
        leds[i].g = r;   // → SK6812 R
        leds[i].b = b;   // → SK6812 B
        leds[i].w = w;   // → SK6812 W
    }
}

void applyLeds() {
    if (state.whiteOnly) {
        // Drive the physical white LED only; RGB channels off.
        fillLeds(0, 0, 0, state.w);
    } else {
        // Colour mode: use RGB channels; W channel off.
        fillLeds(state.r, state.g, state.b, 0);
    }
    FastLED.setBrightness(state.brightness);
    FastLED.show();
}

// ─── HTTP helpers ──────────────────────────────────────────

static int parseParam(const String &req, const char *key) {
    String k = String(key) + "=";
    int i = req.indexOf(k);
    if (i < 0) return -1;
    i += k.length();
    int j = i;
    while (j < (int)req.length() && req[j] != '&' && req[j] != ' ' && req[j] != '\r') {
        j++;
    }
    if (j == i) return -1;
    return req.substring(i, j).toInt();
}

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

        int v;
        if ((v = parseParam(req, "w"))  >= 0) state.w          = (uint8_t)constrain(v, 0, 255);
        if ((v = parseParam(req, "r"))  >= 0) state.r          = (uint8_t)constrain(v, 0, 255);
        if ((v = parseParam(req, "g"))  >= 0) state.g          = (uint8_t)constrain(v, 0, 255);
        if ((v = parseParam(req, "bl")) >= 0) state.b          = (uint8_t)constrain(v, 0, 255);
        if ((v = parseParam(req, "br")) >= 0) state.brightness = (uint8_t)constrain(v, 0, 255);

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

    FastLED.addLeds<LED_TYPE, LED_DATA_PIN, COLOR_ORDER>(ledsRGB, getRGBWsize(NUM_LEDS));
    FastLED.setCorrection(UncorrectedColor);  // no per-channel skew; W scales cleanly
    FastLED.setBrightness(0);
    fillLeds(0, 0, 0, 0);
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
