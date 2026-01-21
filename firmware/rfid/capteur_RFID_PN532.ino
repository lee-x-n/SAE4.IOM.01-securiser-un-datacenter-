#include <Wire.h>
#include <PN532_I2C.h>
#include <PN532.h>
#include <ESP8266WiFi.h>
#include <AsyncMqttClient.h>
#include <Ticker.h>
#include <ArduinoJson.h>

#define WIFI_SSID "binome_1"
#define WIFI_PASSWORD "tpRT9025"

PN532_I2C pn532i2c(Wire);
PN532 nfc(pn532i2c);

String masterCardUID = "c3:1f:15:0d"; //uid de carte admin choisie
String uidList = "";
unsigned long lastMasterCardTime = 0;
const unsigned long masterCardDelay = 10000;
bool adminMode = false;
String lastUIDScanned = ""; // UID en attente de traitement admin

int antiCloneCounter = 0;

const char* mqttServer = "192.168.1.100";
const int mqttPort = 1883;
const char* mqttUser = "lidian";
const char* mqttPassword = "lidian";
const char* mqttTopicDebug = "capteur/rfid";

AsyncMqttClient mqttClient;
WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker mqttReconnectTimer;
Ticker wifiReconnectTimer;

void connectToWifi() {
  Serial.println("Connexion au Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnecté au Wi-Fi !");
    Serial.print("Adresse IP : ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nÉchec de connexion Wi-Fi.");
  }
  delay(1000);
}

void connectToMqtt() {
  Serial.println("Connexion au serveur MQTT...");
  mqttClient.connect();
}

void onWifiConnect(const WiFiEventStationModeGotIP& event) {
  connectToMqtt();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  mqttReconnectTimer.detach();
  wifiReconnectTimer.once(2, connectToWifi);
}

void onMqttConnect(bool sessionPresent) {
  Serial.println("Connecté à MQTT.");
  mqttClient.subscribe("rfid/response", 1);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Déconnecté de MQTT.");
  if (WiFi.isConnected()) {
    mqttReconnectTimer.once(2, connectToMqtt);
  }
}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  String topicStr = String(topic);
  if (topicStr == "rfid/response") {
    DynamicJsonDocument doc(256);
    DeserializationError err = deserializeJson(doc, payload);
    if (err) {
      Serial.println("Erreur de parsing JSON");
      return;
    }

    String uid = doc["uid"];
    String status = doc["status"];
    String salles = doc["salles"];  // Salle sous forme de chaîne
    int counter = doc["antiCloneCounter"];

    if (adminMode) {
      Serial.println(" [Admin] Réponse de Node-RED");
      Serial.println("UID : " + uid);
      Serial.println("Statut : " + status);
      Serial.println("Salle : " + salles);
      Serial.println("Compteur reçu : " + String(counter));

      // Mise à jour du compteur anti-clonage
      DynamicJsonDocument updateDoc(256);
      updateDoc["uid"] = uid;
      updateDoc["antiCloneCounter"] = counter + 1;
      String updateOut;
      serializeJson(updateDoc, updateOut);
      mqttClient.publish("rfid/update", 1, true, updateOut.c_str());

      Serial.println(" Mise à jour envoyée.");
    } else {
      Serial.println(" [Lecture] Réponse de Node-RED");
      Serial.println("UID : " + uid);
      Serial.println("Statut : " + status);
      Serial.println("Salle : " + salles);
      Serial.println("Compteur reçu : " + String(counter));

      if (status == "autorise") {
        Serial.println("✅ Accès autorisé - Ouverture de la porte pour la salle : " + salles);
      } else {
        Serial.println("❌ Accès refusé.");
      }

      // Incrémenter et envoyer le compteur mis à jour
      DynamicJsonDocument updateDoc(256);
      updateDoc["uid"] = uid;
      updateDoc["antiCloneCounter"] = counter + 1;
      String output;
      serializeJson(updateDoc, output);
      mqttClient.publish("rfid/update", 1, true, output.c_str());
    }
  }
}

void setup(void) {
  Serial.begin(115200);
  Serial.println("Initialisation du module PN532...");

  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println("Erreur : Module PN53x non détecté.");
    while (1);
  }

  nfc.SAMConfig();
  Serial.println("Module prêt, en attente de carte...");

  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCredentials(mqttUser, mqttPassword);

  connectToWifi();
}

void loop(void) {
  uint8_t success;
  uint8_t uid[7];
  uint8_t uidLength;

  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

  if (adminMode && millis() - lastMasterCardTime > masterCardDelay) {
    Serial.println("⏱️ Fin du mode admin.");
    adminMode = false;
  }

  if (success) {
    String uidString = "";
    for (uint8_t i = 0; i < uidLength; i++) {
      if (uid[i] < 0x10) uidString += "0";
      uidString += String(uid[i], HEX);
      if (i < uidLength - 1) uidString += ":";
    }

    Serial.print("Carte détectée : ");
    Serial.println(uidString);

    if (uidString == masterCardUID) {
      Serial.println("🔑 Carte maître détectée !");
      lastMasterCardTime = millis();
      adminMode = true;
      return;
    }

    // Envoi vers Node-RED
    DynamicJsonDocument doc(128);
    doc["uid"] = uidString;
    if (adminMode) {
      doc["mode"] = "admin";
    } else {
      doc["mode"] = "lecture";
    }
    String output;
    serializeJson(doc, output);
    mqttClient.publish("rfid/request", 1, false, output.c_str());

    // Sauvegarde temporaire de la carte lue pour traitement dans onMqttMessage()
    lastUIDScanned = uidString;

    delay(1500);
  }
}
