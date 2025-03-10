from fastapi import FastAPI, UploadFile, File
import paho.mqtt.client as mqtt
import cv2
import numpy as np
import os
from pymongo import MongoClient
from datetime import datetime
import boto3

# Configurare MinIO/AWS S3 (dacă folosești cloud storage)
s3 = boto3.client('s3', endpoint_url="http://localhost:9000", aws_access_key_id="minioadmin", aws_secret_access_key="minioadmin")
BUCKET_NAME = "esp32-images"

# Configurare MongoDB
client = MongoClient("mongodb://localhost:27017/")
db = client["image_database"]
image_collection = db["images"]

# Configurare FastAPI
app = FastAPI()

# Configurare MQTT
MQTT_BROKER = "localhost"
MQTT_PORT = 1883
MQTT_TOPIC = "PICTURE"

def on_message(client, userdata, msg):
    print(f"📷 Imagine primită de la {msg.topic}")
    
    # Decodificare imagine
    nparr = np.frombuffer(msg.payload, np.uint8)
    image = cv2.imdecode(nparr, cv2.IMREAD_COLOR)
    
    if image is None:
        print("Eroare la decodificarea imaginii")
        return

    # Salvare imagine local
    timestamp = datetime.now().strftime("%Y%m%d%H%M%S")
    filename = f"images/{timestamp}.jpg"
    cv2.imwrite(filename, image)
    
    # Salvare în MinIO/S3
    s3.upload_file(filename, BUCKET_NAME, filename)

    # Salvare metadate în MongoDB
    image_metadata = {
        "filename": filename,
        "timestamp": timestamp,
        "device": "ESP32-CAM",
        "url": f"http://localhost:9000/{BUCKET_NAME}/{filename}"
    }
    image_collection.insert_one(image_metadata)
    print("Imagine salvată")

mqtt_client = mqtt.Client()
mqtt_client.on_message = on_message
mqtt_client.connect(MQTT_BROKER, MQTT_PORT, 60)
mqtt_client.subscribe(MQTT_TOPIC)
mqtt_client.loop_start()

@app.get("/images")
def get_images():
    """Returnează lista de imagini salvate"""
    images = list(image_collection.find({}, {"_id": 0}))
    return {"images": images}
