#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "DHTesp.h"

/* ───────── WIFI ───────── */
const char* ssid = "Wokwi-GUEST";
const char* password = "";

/* ───────── SERVEUR FLASK ───────── */
// ✅ URL LOCALE directe (Idéal pour simuler avec Wokwi pour VS Code)
const char* SERVER_URL = "http://localhost:5000/upload";

/* ───────── BROCHES (VOTRE MONTAGE) ───────── */
#define DHTPIN 15
#define LED_PIN 14

DHTesp dht;

unsigned long lastSend = 0;
const long interval = 2000; // ✅ Plus rapide pour réagir au dashboard

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
/* ... (reste du setup déjà présent) ... */

void loop() {
  if (millis() - lastSend > interval) {
    lastSend = millis();

    TempAndHumidity data = dht.getTempAndHumidity();

    if (isnan(data.temperature) || isnan(data.humidity)) {
      Serial.println("❌ Erreur DHT22");
      return;
    }

    Serial.printf("\n[LOCAL] 🌡 %.1f°C | 💧 %.1f%% (Heap: %d)\n", 
                  data.temperature, data.humidity, ESP.getFreeHeap());

    if (WiFi.status() == WL_CONNECTED) {
      WiFiClient client;
      
      HTTPClient http;
      Serial.print("[HTTP] Tentative de connexion...");
      
      if (http.begin(client, SERVER_URL)) {
        http.addHeader("Content-Type", "application/json");

          StaticJsonDocument<128> doc;
          doc["temperature"] = data.temperature;
          doc["humidity"] = data.humidity;

          String json;
          serializeJson(doc, json);

          int code = http.POST(json);

          if (code > 0) {
            String response = http.getString();
            Serial.printf("✅ HTTP %d | Taille: %d\n", code, response.length());
            
            StaticJsonDocument<1536> respDoc;
            DeserializationError err = deserializeJson(respDoc, response);
            
            if (!err) {
              // 1. État de la LED
              bool ledState = respDoc["led"] | false;
              digitalWrite(LED_PIN, ledState ? HIGH : LOW);
              
              // 2. Données Dashboard
              if (respDoc.containsKey("global_latest")) {
                JsonObject global = respDoc["global_latest"];
                float g_temp = global["temperature"] | 0.0;
                float g_hum = global["humidity"] | 0.0;
                const char* g_time = global["timestamp"] | "??";
                const char* g_src = global["source"] | "esp32";

                Serial.println("----------------------------------------");
                Serial.println(">>> SYNC DASHBOARD (Real-time) <<<");
                Serial.printf(" [SERVER] T=%.1f C | H=%.1f %%\n", g_temp, g_hum);
                Serial.printf(" [TIME]   %s | SOURCE: %s\n", g_time, g_src);
                Serial.printf(" [LED]    %s\n", ledState ? "ON (ACCUMU)" : "OFF");
                Serial.println("----------------------------------------");
              }
            } else {
              Serial.printf("❌ Erreur JSON: %s\n", err.c_str());
            }
          } else {
            Serial.printf("❌ Erreur HTTP: %s\n", http.errorToString(code).c_str());
          }
          http.end();
        } else {
          Serial.println("❌ Impossible de commencer la connexion HTTP");
        }
    } else {
      Serial.println("📡 WiFi déconnecté !");
    }
  }
}
