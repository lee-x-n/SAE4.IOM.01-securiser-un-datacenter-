#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <AsyncMqttClient.h>

// === Paramètres Wi-Fi ===
#define WIFI_SSID "binome_1"
#define WIFI_PASSWORD "tpRT9025"

// === Paramètres MQTT ===
#define MQTT_HOST IPAddress(192,168,1,100)
#define MQTT_PORT 1883
#define MQTT_USER "lidian"
#define MQTT_PASS "lidian"

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;
WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;

// === Paramètres MQ135 ===
double CorA = 0.00035;
double CorB = 0.02718;
double CorC = 1.39538;
double CorD = 0.0018;

double Temp = 22.3;  // Température simulée
double Hum = 67.0;   // Humidité simulée
double RLOAD = 10;
double RZERO = 18.41;
double ParA = 116.6020682;
double ParB = 2.769034857;

// === Fonctions MQ135 ===
double FacteurCorrection() {
  return (((((CorA * ((Temp * Temp))) - (CorB * Temp))) + CorC)) - (((Hum - 33)) * CorD);
}

double GetResistance() {
  return ((((((1023.0 / analogRead(A0))) * 5) - 1)) * RLOAD);
}

double GetCorrectedppm() {
  return (ParA * pow((GetResistance() / RZERO), (0 - ParB)));
}

// === Fonctions Connexions ===
void connectToWifi() {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

// === Gestion Wi-Fi ===
void onWifiConnect(const WiFiEventStationModeGotIP& event) {
  Serial.println("Connected to Wi-Fi.");
  connectToMqtt();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  Serial.println("Disconnected from Wi-Fi.");
  mqttReconnectTimer.detach();
  wifiReconnectTimer.once(2, connectToWifi);
}

// === Gestion MQTT ===
void onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT.");
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from MQTT.");
  if (WiFi.isConnected()) {
    mqttReconnectTimer.once(2, connectToMqtt);
  }
}

// === Setup principal ===
void setup() {
  Serial.begin(115200);
  analogRead(A0);  // init lecture capteur

  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCredentials(MQTT_USER, MQTT_PASS);

  connectToWifi();
}

// === Loop principal ===
unsigned long previousMillis = 0;
const long interval = 5000;

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    double ppm = GetCorrectedppm();
    String payload = String(ppm, 2);

    if (mqttClient.connected()) {
      mqttClient.publish("capteur/fumee", 1, false, payload.c_str());
      Serial.print("PPM envoyé : ");
      Serial.println(payload);
    } else {
      Serial.println("MQTT non connecté, données non envoyées.");
    }
  }
}
