#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <AsyncMqttClient.h>
#include <Ticker.h>

// WiFi credentials
#define WIFI_SSID "binome_1"
#define WIFI_PASSWORD "tpRT9025"

// MQTT broker config
#define MQTT_HOST IPAddress(192,168,1,100)
#define MQTT_PORT 1883
#define MQTT_USER "lidian"
#define MQTT_PASSWORD "lidian"

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;
Ticker wifiReconnectTimer;

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;

const int tiltPin = 16;  // GPIO16 (D0 sur ESP8266)
int previousTiltState = LOW;  // État précédent du capteur (par défaut, pas de choc)

void connectToWifi() {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void onWifiConnect(const WiFiEventStationModeGotIP& event) {
  Serial.println("Connected to Wi-Fi.");
  connectToMqtt();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  Serial.println("Disconnected from Wi-Fi.");
  mqttReconnectTimer.detach();
  wifiReconnectTimer.once(2, connectToWifi);
}

void onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT.");
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from MQTT.");
  if (WiFi.isConnected()) {
    mqttReconnectTimer.once(2, connectToMqtt);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(tiltPin, INPUT);

  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCredentials(MQTT_USER, MQTT_PASSWORD);

  connectToWifi();
}

void loop() {
  int tiltState = digitalRead(tiltPin); // Lire l'état du capteur

  // Si un choc est détecté (changement d'état du capteur à HIGH)
  if (tiltState == HIGH && previousTiltState == LOW) {
    String message = "Choc détecté!";
    mqttClient.publish("capteur/choc", 1, true, message.c_str());
    Serial.println("Choc détecté! Message envoyé.");

    // Mettre à jour l'état précédent
    previousTiltState = HIGH;
  }
  // Si le capteur retourne à son état initial (pas de choc)
  else if (tiltState == LOW && previousTiltState == HIGH) {
    previousTiltState = LOW; // Mettre à jour l'état précédent
  }
  
  delay(100);  // Petite pause pour éviter une boucle trop rapide
}
