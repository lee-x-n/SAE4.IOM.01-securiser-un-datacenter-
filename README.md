# 🔐 SAÉ 4.IOM.01 – Sécurisation d’un datacenter par technologies IoT

**BUT Réseaux & Télécommunications – Parcours Internet des Objets (IoM)**  
📍 IUT Nord Franche-Comté – Année BUT 3


## 👥 Équipe projet
- **EL AMRANI** Lina  
- **WADE** Mame Diarra  
- **NETO** Anete  


## 🎯 Objectifs du projet

Cette SAÉ vise à concevoir et implémenter un **système de sécurisation d’un datacenter** reposant sur des technologies **IoT**, **réseaux** et **supervision temps réel**.

Les objectifs principaux sont :
- Mettre en œuvre des **réseaux et protocoles sans fil dédiés à l’IoT**
- Assurer un **contrôle d’accès sécurisé** par RFID
- Superviser l’environnement du datacenter (fumée, choc, mouvement, température, humidité)
- Garantir la **continuité de service** par redondance réseau
- Centraliser et visualiser les données en temps réel

---

## 🏗️ Architecture générale du système

Le datacenter est divisé en **trois zones de sécurité** :
- 🟢 **Zone verte** : accès ouvert
- 🟡 **Zone jaune** : accès intermédiaire
- 🔴 **Zone rouge** : zone critique

### Composants principaux
- **ESP8266** : acquisition des données capteurs et RFID
- **Raspberry Pi (x2)** :
  - Broker MQTT
  - Node-RED
  - InfluxDB
  - Grafana
  - MySQL
- **Redondance réseau** via **VRRP**

📌 Une architecture modulaire permettant une supervision centralisée et une haute disponibilité.

---

## 🔄 Communication et supervision

### 📡 MQTT
- Protocole **publish/subscribe**
- Topics dédiés (ex : `capteur/rfid`, `capteur/temperature`)
- **QoS 1** pour garantir la livraison des messages
- Implémentation asynchrone avec `AsyncMqttClient`

---

### 🧠 Node-RED
Node-RED est le cœur logique du système :
- Réception des messages MQTT
- Traitement et vérification des seuils
- Déclenchement d’alertes :
  - Emails
  - Messages WhatsApp
  - LEDs
- Enregistrement des données vers InfluxDB
- Organisation des flows par **zone** et **type de capteur**

---

### 📊 InfluxDB
- Base de données time-series
- Stockage horodaté des mesures :
  - Température
  - Humidité
  - Fumée (PPM)
  - Choc
  - Mouvement
  - Accès RFID
- Historique et traçabilité complète des événements

---

### 📈 Grafana
- Tableaux de bord personnalisés
- Visualisation par zone de sécurité
- Graphiques temps réel et historiques
- Données issues d’InfluxDB et MySQL

---

## 🔌 Capteurs et contrôle d’accès

### 🔑 Lecteur RFID (PN532)
- Communication **I2C**
- Mode utilisateur / mode administrateur (carte maître)
- Ajout / suppression de cartes dynamiques
- **Protection anti-clonage** :
  - Compteur stocké sur la carte
  - Détection d’incohérence → carte bloquée
- Droits stockés en base **MySQL**
- Échanges MQTT en format JSON

---

### 💥 Capteur de choc
- Fonctionnement binaire (0 / 1)
- Détection d’ouverture ou de manipulation
- Déclenchement immédiat d’alertes via Node-RED

---

### 🔥 Capteur de fumée (MQ135)
- Mesure du taux de CO₂ en PPM
- Étallonage automatique basé sur :
  - Température
  - Humidité
- Envoi asynchrone via MQTT
- Simulation d’incendie via bouton Node-RED (800 PPM)

---

### 🚶 Capteur de mouvement (HC-SR501)
- Détection infrarouge de présence
- Sensibilité et durée réglables
- Déclenchement d’alertes en cas d’intrusion

---

### 🌡️ Capteur température / humidité (RHT03 – DHT22)
- Mesures environnementales
- Envoi des données en JSON :
```json
{ "temperature": 24.5, "humidite": 56.2 }
