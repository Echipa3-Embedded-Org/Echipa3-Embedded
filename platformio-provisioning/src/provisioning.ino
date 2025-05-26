/*
 * Provision ESP32-CAM with credentials and store to NVS to avoid including
 * those in firmware.
 */

#include <Preferences.h>
#include "secrets.h"

#define NAMESPACE "mqtt-secrets"
#define READ_ONLY_NAMESPACE false

Preferences preferences;

void setup() {
    size_t ret;
    Serial.begin(115200);
    preferences.begin(NAMESPACE, READ_ONLY_NAMESPACE);
    preferences.clear();

    /* Should be defined in secrets.h */
    ret = preferences.putString("SSID", SSID);
    if (ret < strlen(SSID)) {
        Serial.println("Write SSID failed. The board will now reset.");
        preferences.end();
        ESP.restart();
    }
    ret = preferences.putString("PASSWORD", PASSWORD);
    if (ret < strlen(PASSWORD)) {
        Serial.println("Write PASSWORD failed. The board will now reset.");
        preferences.end();
        ESP.restart();
    }
    ret = preferences.putString("MQTT_SERVER", MQTT_SERVER);
    if (ret < strlen(MQTT_SERVER)) {
        Serial.println("Write MQTT_SERVER failed. The board will now reset.");
        preferences.end();
        ESP.restart();
    }
    ret = preferences.putInt("MQTT_PORT", MQTT_PORT);
    if (ret < sizeof(int32_t)) {
        Serial.println("Write MQTT_PORT failed. The board will now reset.");
        preferences.end();
        ESP.restart();
    }
    ret = preferences.putString("OTA_BASE_URL", OTA_BASE_URL);
    if (ret < strlen(OTA_BASE_URL)) {
        Serial.println("Write OTA_BASE_URL failed. The board will now reset.");
        preferences.end();
        ESP.restart();
    }
    ret = preferences.putString("SERVER_CA", SERVER_CA);
    if (ret < strlen(SERVER_CA)) {
        Serial.println("Write SERVER_CA failed. The board will now reset.");
        preferences.end();
        ESP.restart();
    }
    ret = preferences.putString("CLIENT_CRT", CLIENT_CRT);
    if (ret < strlen(CLIENT_CRT)) {
        Serial.println("Write CLIENT_CRT failed. The board will now reset.");
        preferences.end();
        ESP.restart();
    }
    ret = preferences.putString("CLIENT_KEY", CLIENT_KEY);
    if (ret < strlen(CLIENT_KEY)) {
        Serial.println("Write CLIENT_KEY failed. The board will now reset.");
        preferences.end();
        ESP.restart();
    }

    preferences.end();
    Serial.println("Credentials successfully saved to NVS.");
    Serial.println("You can now poweroff the board.");
}

void loop() {}
