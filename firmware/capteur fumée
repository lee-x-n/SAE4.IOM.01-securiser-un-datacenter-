#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define WIFI_SSID "binome_1"
#define WIFI_PASSWORD "tpRT9025"

#define MQTT_SERVER "192.168.1.100"
#define MQTT_PORT 1883
#define MQTT_USER "lidian"
#define MQTT_PASSWORD "lidian"
#define MQTT_TOPIC "capteur/fumee"

#define MQ135_PIN A0

WiFiClient espClient;
PubSubClient mqttClient(espClient);

void connectToWiFi() {
  Serial.print("Connexion au Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("Connecté !");
}

void connectToMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Connexion au broker MQTT...");
    if (mqttClient.connect("ESP8266Fumee", MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("Connecté au broker !");
    } else {
      Serial.print("Échec, code = ");
      Serial.println(mqttClient.state());
      delay(3000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);

  connectToWiFi();

  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  connectToMQTT();
}

void loop() {
  if (!mqttClient.connected()) {
    connectToMQTT();
  }
  mqttClient.loop();

  int valeurBrute = analogRead(MQ135_PIN);
  Serial.print("Valeur MQ135 : ");
  Serial.println(valeurBrute);

  char payload[10];
  snprintf(payload, sizeof(payload), "%d", valeurBrute);

  // Publication avec QoS 1
  mqttClient.publish(MQTT_TOPIC, 1, false, payload);

  delay(3000);
}
