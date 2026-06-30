/*
 * Nano 33 IoT – SK9822 LED Controller
 *
 * Behaviour:
 *   • Advertises via BLE (name: NanoLED).
 *   • When a mobile device connects over BLE, the NINA module also connects
 *     to WiFi and starts a web server on port 80.
 *   • The web UI lets you pick White-only mode (single brightness slider) or
 *     full RGB colour mode, plus a global brightness slider.
 *   • When the BLE device disconnects, WiFi is stopped to save power.
 *
 * Wiring:
 *   SK9822 DATA  → pin 11 (MOSI / SPI)
 *   SK9822 CLOCK → pin 13 (SCK  / SPI)
 *   SK9822 VCC   → 5 V (use a separate 5 V supply for ≥20 LEDs)
 *   SK9822 GND   → GND (common with Nano GND)
 *
 * Note: SK9822 is an RGB + 5-bit-global-brightness LED (not RGBW).
 * "White" is achieved by driving R, G, B equally via the white-intensity
 * slider. FastLED's setBrightness() provides independent overall dimming.
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

// ─── LED state ─────────────────────────────────────────────
CRGB leds[NUM_LEDS];

struct State {
  uint8_t r          = 255;
  uint8_t g          = 255;
  uint8_t b          = 255;
  uint8_t w          = 255;   // white intensity (equal RGB)
  uint8_t brightness = 128;
  bool    whiteOnly  = true;
} state;

// ─── WiFi / server ─────────────────────────────────────────
WiFiServer server(WEB_SERVER_PORT);
bool wifiActive = false;

// ─── BLE ───────────────────────────────────────────────────
// A bare service is enough – connection itself triggers WiFi.
BLEService ledService(BLE_SERVICE_UUID);

// ─── LED helpers ───────────────────────────────────────────
void applyLeds() {
  CRGB color = state.whiteOnly
    ? CRGB(state.w, state.w, state.w)
    : CRGB(state.r, state.g, state.b);
  fill_solid(leds, NUM_LEDS, color);
  FastLED.setBrightness(state.brightness);
  FastLED.show();
}

// ─── HTTP helpers ──────────────────────────────────────────

// Returns the integer value of a query parameter, or -1 if absent.
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

  // Read only the first 256 bytes – enough for any GET request line we need.
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
    // Mode toggle
    if (req.indexOf("mode=white") >= 0) state.whiteOnly = true;
    else if (req.indexOf("mode=color") >= 0) state.whiteOnly = false;

    // Numeric parameters
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

  FastLED.addLeds<LED_TYPE, LED_DATA_PIN, LED_CLOCK_PIN, COLOR_ORDER>(leds, NUM_LEDS);
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
