#include <WiFi.h>
#include <PubSubClient.h>
#include "esp_camera.h"

const char* ssid = "SSID";
const char* password = "PASSWORD";
const char* mqtt_server = "BROKER IP";
const char* mqtt_topic = "PICTURE";

WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }
}

void take_picture_and_send() {
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("❌ Camera capture failed");
        return;
    }
    client.publish(mqtt_topic, fb->buf, fb->len);
    esp_camera_fb_return(fb);
}

void setup() {
    Serial.begin(115200);
    setup_wifi();
    client.setServer(mqtt_server, 1883);
    client.connect("ESP32CAM");
}

void loop() {
    if (!client.connected()) {
        client.connect("ESP32CAM");
    }
    client.loop();
    take_picture_and_send();
    delay(5000); // Trimite o imagine la fiecare 5 secunde
}
