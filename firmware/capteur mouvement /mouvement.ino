#!/usr/bin/env python3
# motion_simulator.py
import cv2, time, os, json
from threading import Thread
from flask import Flask, Response, send_file
import paho.mqtt.client as mqtt

VIDEO = "demo.mp4"           # ton fichier vidéo (mettre en boucle)
MQTT_BROKER = "192.168.60.9"
MQTT_TOPIC = "clinic/cam1/motion"
SNAPSHOT_DIR = "snapshots"
PORT_HTTP = 5000

os.makedirs(SNAPSHOT_DIR, exist_ok=True)

# MQTT client
client = mqtt.Client("motion_sim")
client.connect(MQTT_BROKER, 1883, 60)
client.loop_start()

# Flask app pour servir la dernière image
app = Flask(__name__)
last_snapshot = os.path.join(SNAPSHOT_DIR, "last.jpg")

def motion_worker():
    cap = cv2.VideoCapture(VIDEO)
    if not cap.isOpened():
        print("Erreur : impossible d'ouvrir la vidéo", VIDEO)
        return

    ret, prev = cap.read()
    if not ret:
        print("Erreur lecture")
        return
    prev_gray = cv2.cvtColor(prev, cv2.COLOR_BGR2GRAY)
    prev_gray = cv2.GaussianBlur(prev_gray, (21,21), 0)

    while True:
        ret, frame = cap.read()
        if not ret:
            cap.set(cv2.CAP_PROP_POS_FRAMES, 0)  # boucle
            time.sleep(0.1)
            continue

        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        gray = cv2.GaussianBlur(gray, (21,21), 0)

        diff = cv2.absdiff(prev_gray, gray)
        thresh = cv2.threshold(diff, 25, 255, cv2.THRESH_BINARY)[1]
        cnts, _ = cv2.findContours(thresh, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

        motion = False
        for c in cnts:
            if cv2.contourArea(c) > 500:   # seuil area -> ajuster
                motion = True
                x,y,w,h = cv2.boundingRect(c)
                cv2.rectangle(frame,(x,y),(x+w,y+h),(0,255,0),2)

        # sauvegarde snapshot si mouvement
        if motion:
            ts = int(time.time())
            fname = os.path.join(SNAPSHOT_DIR, f"motion_{ts}.jpg")
            cv2.imwrite(fname, frame)
            cv2.imwrite(last_snapshot, frame)
            payload = json.dumps({"camera":"cam1","motion":True,"ts":ts})
            client.publish(MQTT_TOPIC, payload)
            print("Motion detected ->", fname)
        else:
            # met à jour la preview quand meme
            cv2.imwrite(last_snapshot, frame)

        prev_gray = gray
        time.sleep(0.1)

# Flask endpoints pour afficher image
@app.route("/snapshot.jpg")
def snapshot():
    if os.path.exists(last_snapshot):
        return send_file(last_snapshot, mimetype='image/jpeg')
    return "No image", 404

def run_flask():
    app.run(host="0.0.0.0", port=PORT_HTTP, threaded=True)

if __name__ == "__main__":
    t = Thread(target=motion_worker, daemon=True)
    t.start()
    run_flask()
