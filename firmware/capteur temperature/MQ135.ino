#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <AsyncMqttClient.h>

#define WIFI_SSID "binome_1"
#define WIFI_PASSWORD "tpRT9025"

#define MQTT_HOST IPAddress(192,168,1,100) //MQTT BROKER IP ADDRESS
#define MQTT_PORT 1883
#define BROKER_USER "lidian"
#define BROKER_PASS "lidian"

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;


unsigned long previousMillis = 0;   // Stores last time a message was published
const long interval = 10000;        // Interval at which to publish values

void connectToWifi() {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
   int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) { 
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to Wi-Fi!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect. Check credentials and signal strength.");
  }
  delay(5000);
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
  mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
  wifiReconnectTimer.once(2, connectToWifi);
}

void onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT.");
  Serial.print("Session present: ");
  Serial.println(sessionPresent);

  // Subscribe to topic "led" when it connects to the broker
  uint16_t packetIdSub = mqttClient.subscribe("led", 2);
  Serial.print("Subscribing at QoS 2, packetId: ");
  Serial.println(packetIdSub);

  // Publish on the "test" topic with qos 0
  mqttClient.publish("test", 0, true, "test 1");
  Serial.println("Publishing at QoS 0");
 // Publish on the "test" topic with qos 1
  uint16_t packetIdPub1 = mqttClient.publish("test", 1, true, "test 2");
  Serial.print("Publishing at QoS 1, packetId: ");
  Serial.println(packetIdPub1);
  // Publish on the "test" topic with qos 2
  uint16_t packetIdPub2 = mqttClient.publish("test", 2, true, "test 3");
  Serial.print("Publishing at QoS 2, packetId: ");
  Serial.println(packetIdPub2);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from MQTT.");

  if (WiFi.isConnected()) {
    mqttReconnectTimer.once(2, connectToMqtt);
  }
}

void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  Serial.println("Subscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
  Serial.print("  qos: ");
  Serial.println(qos);
}

void onMqttUnsubscribe(uint16_t packetId) {
  Serial.println("Unsubscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties 
properties, size_t len, size_t index, size_t total) {
  
  //Ecrire votre fonction  ici ce que vous voulez lorsque vous recevez un message ( test du topic/ action en fonction du contenu du message)
  //.....
  //fin de fonction
  Serial.println("Publish received.");
  Serial.print("  Topic: ");
  Serial.println(topic);
  Serial.print("  Message: ");

  String message = "";  // Initialisation d'une chaîne pour stocker le message reçu
  for (size_t i = 0; i < len; i++) {
    Serial.print(payload[i]);  
    message += payload[i];  // Construction du message sous forme de String
  }
  Serial.println();  // Nouvelle ligne pour plus de lisibilité

  // Vérification du topic et action sur la LED
  if (String(topic) == "led") {
    if (message == "true") {
      digitalWrite(LED_BUILTIN, LOW);  // Allume la LED (active en LOW sur ESP8266)
      Serial.println("LED ALLUMÉE !");
    } 
    else if (message == "false") {
      digitalWrite(LED_BUILTIN, HIGH); // Éteint la LED
      Serial.println("LED ÉTEINTE !");
    } 
    else {
      Serial.println("Message non reconnu !");
    }
  }
}

void onMqttPublish(uint16_t packetId) {
  Serial.println("Publish acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  pinMode (LED_BUILTIN, OUTPUT);

  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCredentials(BROKER_USER, BROKER_PASS);


  connectToWifi();
}

void loop() {

  unsigned long currentMillis = millis();
  // Every X number of seconds (interval = 10 seconds) 
  // it publishes a new MQTT message
  if (currentMillis - previousMillis >= interval) {
    // Save the last time a new reading was published
    previousMillis = currentMillis;

  // Publish an MQTT message on topic esp/bme680/temperature
  uint16_t packetIdPub1 = mqttClient.publish("counter", 1, true, String(random(0,20)).c_str());
  Serial.printf("Publishing on topic %s at QoS 1, packetId: %i", "counter", packetIdPub1);
  }
}
