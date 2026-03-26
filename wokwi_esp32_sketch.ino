#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "DHTesp.h"

/* ───────── WIFI ───────── */
const char* ssid = "Wokwi-GUEST";
const char* password = "";

/* ───────── SERVEUR FLASK ───────── */
// ✅ NOUVELLE URL PUBLIQUE GÉNÉRÉE :
const char* SERVER_URL = "https://sixty-hairs-stick.loca.lt/upload";

/* ───────── BROCHES (VOTRE MONTAGE) ───────── */
#define DHTPIN 15
#define LED_PIN 14

DHTesp dht;

unsigned long lastSend = 0;
const long interval = 5000;

/* ───────── WIFI ───────── */
void setup_wifi() {
  Serial.print("Connexion WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi connecté !");
  Serial.println(WiFi.localIP());
}

/* ───────── SETUP ───────── */
void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  dht.setup(DHTPIN, DHTesp::DHT22);

  setup_wifi();
}

/* ───────── LOOP ───────── */
void loop() {
  if (millis() - lastSend > interval) {
    lastSend = millis();

    TempAndHumidity data = dht.getTempAndHumidity();

    if (isnan(data.temperature) || isnan(data.humidity)) {
      Serial.println("❌ Erreur DHT22");
      return;
    }

    Serial.printf("🌡 %.1f°C | 💧 %.1f%%\n", data.temperature, data.humidity);

    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(SERVER_URL);
      http.addHeader("Content-Type", "application/json");
      http.addHeader("Bypass-Tunnel-Reminder", "true"); // Pour Localtunnel

      StaticJsonDocument<128> doc;
      doc["temperature"] = data.temperature;
      doc["humidity"] = data.humidity;

      String json;
      serializeJson(doc, json);

      int code = http.POST(json);

      if (code > 0) {
        String response = http.getString();
        Serial.printf("✅ HTTP %d | Réponse: %s\n", code, response.c_str());

        // --- NOUVEAU : On récupère l'état de la LED ---
        StaticJsonDocument<256> respDoc;
        DeserializationError err = deserializeJson(respDoc, response);
        if (!err) {
          bool ledState = respDoc["led"];
          digitalWrite(LED_PIN, ledState ? HIGH : LOW);
          Serial.printf("💡 LED State: %s\n", ledState ? "ON" : "OFF");
        }
      } else {
        Serial.printf("❌ Erreur HTTP: %s\n", http.errorToString(code).c_str());
      }
      http.end();
    }
  }
}
