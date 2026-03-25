#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <PubSubClient.h>

#define DHTPIN 4
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

// ================= WIFI =================
const char* ssid = "Wifi";
const char* password = "25mei2005oke";

// ================= MQTT =================
const char* mqttServer = "broker.hivemq.com";
const int mqttPort = 1883;
WiFiClient espClient;
PubSubClient client(espClient);
const char* mqttTopic = "sic/re202/sensor";

// ================= API WEATHER =================
const char* city = "Samarinda";
const char* apiKey = "95be204bd0ec0a72aefbb5d0f298c9d2";

// ================= TIMESTAMP =================
unsigned long startTime = 0;

// ================= LABEL =================
String getLabel(float suhu, float kelembapan) {
  if (suhu < 26.0 && kelembapan > 70.0) return "Dingin";
  else if (suhu > 30.0) return "Panas";
  else return "Normal";
}

// ================= MQTT RECONNECT =================
void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Menghubungkan ke MQTT...");
    if (client.connect("ESP32-Inkubator")) {
      Serial.println("Terhubung!");
    } else {
      Serial.print("Gagal. rc=");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

// ================= API SUHU LUAR =================
float getOutdoorTemp() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "http://api.openweathermap.org/data/2.5/weather?q=" +
                 String(city) + "&appid=" + apiKey + "&units=metric";
    http.begin(url);
    int httpCode = http.GET();

    if (httpCode == 200) {
      String payload = http.getString();
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, payload);
      float temp = doc["main"]["temp"];
      http.end();
      return temp;
    } else {
      http.end();
      return NAN;
    }
  }
  return NAN;
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);
  dht.begin();

  Serial.print("Menghubungkan WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Terhubung!");

  client.setServer(mqttServer, mqttPort);

  startTime = millis(); // mulai timestamp dari 0
}

// ================= LOOP =================
void loop() {
  if (!client.connected()) reconnectMQTT();
  client.loop();

  float suhuInkubator = dht.readTemperature();
  float kelembapan = dht.readHumidity();
  float suhuLuar = getOutdoorTemp();

  if (isnan(suhuInkubator) || isnan(kelembapan) || isnan(suhuLuar)) {
    Serial.println("Gagal membaca sensor / API");
    delay(2000);
    return;
  }

  String label = getLabel(suhuInkubator, kelembapan);
  unsigned long timestamp = millis() - startTime;

  Serial.print("Timestamp: "); Serial.println(timestamp);
  Serial.print("Suhu: "); Serial.println(suhuInkubator);
  Serial.print("Humidity: "); Serial.println(kelembapan);
  Serial.print("Label: "); Serial.println(label);
  Serial.print("Suhu luar: "); Serial.println(suhuLuar);

  // ================= JSON UNTUK EDGE IMPULSE =================
  DynamicJsonDocument dataSend(256);
  dataSend["timestamp"] = timestamp;      // WAJIB
  dataSend["Suhu"] = suhuInkubator;
  dataSend["Humidity"] = kelembapan;
  dataSend["suhu_luar"] = suhuLuar;
  dataSend["label"] = label;

  char buffer[256];
  serializeJson(dataSend, buffer);
  client.publish(mqttTopic, buffer);

  delay(5000); // interval 5 detik
}
