/*
  ESP8266 NodeMCU + XKC-Y28 IV Monitor
  - Reads digital sensor state (LOW/HIGH)
  - Sends mapped value to ThingSpeak
  - Sleeps for 3 minutes between transmissions
*/

#include <ESP8266WiFi.h>

// ------------------------------
// USER SETTINGS
// ------------------------------
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* THINGSPEAK_API_KEY = "YOUR_THINGSPEAK_WRITE_API_KEY";

// ThingSpeak host and port
const char* THINGSPEAK_HOST = "api.thingspeak.com";
const int THINGSPEAK_PORT = 80;

// Sensor pin (D5 = GPIO14, change if needed)
const int SENSOR_PIN = D5;

// XKC-Y28 behavior from your description:
// sensor reads LOW when status = 1, HIGH when status = 0
// We map it so transmitted value is:
// 1 = LOW detected (IV event / liquid detected)
// 0 = HIGH detected (no detect / normal by your logic)
int getMappedSensorValue() {
  int raw = digitalRead(SENSOR_PIN);
  return (raw == LOW) ? 1 : 0;
}

bool connectToWiFi(unsigned long timeoutMs = 20000) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < timeoutMs) {
    delay(500);
  }
  return WiFi.status() == WL_CONNECTED;
}

bool sendToThingSpeak(int value) {
  WiFiClient client;
  if (!client.connect(THINGSPEAK_HOST, THINGSPEAK_PORT)) {
    return false;
  }

  // field1 = patient 1 IV status
  String url = "/update?api_key=" + String(THINGSPEAK_API_KEY) + "&field1=" + String(value);

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + THINGSPEAK_HOST + "\r\n" +
               "Connection: close\r\n\r\n");

  unsigned long start = millis();
  while (client.connected() && (millis() - start) < 5000) {
    while (client.available()) {
      client.read();
      start = millis();
    }
    delay(10);
  }

  client.stop();
  return true;
}

void setup() {
  // NPN open-collector outputs usually need pull-up.
  pinMode(SENSOR_PIN, INPUT_PULLUP);
  delay(100);

  int sensorValue = getMappedSensorValue();

  bool wifiOk = connectToWiFi();
  if (wifiOk) {
    sendToThingSpeak(sensorValue);
  }

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(50);

  // ------------------------------
  // DEEP SLEEP DURATION SETTING
  // Change this value to adjust sleep time.
  // 3 minutes = 180 seconds = 180,000,000 microseconds
  // ------------------------------
  const uint64_t SLEEP_TIME_US = 180ULL * 1000000ULL;

  // IMPORTANT HARDWARE NOTE:
  // Connect RST pin to D0 (GPIO16) to wake up from deep sleep.
  ESP.deepSleep(SLEEP_TIME_US);
}

void loop() {
  // Not used because we deep sleep from setup().
}
