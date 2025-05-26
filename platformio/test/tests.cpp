#include <unity.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <MQTT.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <mbedtls/sha256.h>
#include "esp_camera.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#define SECRETS_NAMESPACE "mqtt-secrets"
#define READ_ONLY_SECRETS_NAMESPACE true

#define LED_BUILTIN 4

// Camera Pins (AI Thinker ESP32-CAM)
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// Credentials
char SSID[64];
char PASSWORD[64];
char MQTT_SERVER[256];
int32_t MQTT_PORT;
char SERVER_CA[2048];
char CLIENT_CRT[2048];
char CLIENT_KEY[2048];
char OTA_BASE_URL[256];

Preferences preferences;

bool creds_read;

bool readCredentials() {
    Serial.print("Reading credentials... ");

    preferences.begin(SECRETS_NAMESPACE, READ_ONLY_SECRETS_NAMESPACE);

    strcpy(SSID, preferences.getString("SSID", "").c_str());
    if (SSID[0] == '\0') {
        Serial.println("fail. SSID.");
        preferences.end();
        return false;
    }
    strcpy(PASSWORD, preferences.getString("PASSWORD", "").c_str());
    if (PASSWORD[0] == '\0') {
        Serial.println("fail. PASSWORD.");
        preferences.end();
        return false;
    }
    strcpy(MQTT_SERVER, preferences.getString("MQTT_SERVER", "").c_str());
    if (MQTT_SERVER[0] == '\0') {
        Serial.println("fail. MQTT_SERVER.");
        preferences.end();
        return false;
    }
    MQTT_PORT = preferences.getInt("MQTT_PORT", 0);
    if (MQTT_PORT == 0) {
        Serial.println("fail. MQTT_PORT.");
        preferences.end();
        return false;
    }
    strcpy(OTA_BASE_URL, preferences.getString("OTA_BASE_URL", "").c_str());
    if (OTA_BASE_URL[0] =='\0') {
        Serial.println("fail. OTA_BASE_URL.");
        preferences.end();
        return false;
    }
    strcpy(SERVER_CA, preferences.getString("SERVER_CA", "").c_str());
    if (SERVER_CA[0] == '\0') {
        Serial.println("fail. SERVER_CA.");
        preferences.end();
        return false;
    }
    strcpy(CLIENT_CRT, preferences.getString("CLIENT_CRT", "").c_str());
    if (CLIENT_CRT[0] == '\0') {
        Serial.println("fail. CLIENT_CRT.");
        preferences.end();
        return false;
    }
    strcpy(CLIENT_KEY, preferences.getString("CLIENT_KEY", "").c_str());
    if (CLIENT_KEY[0] == '\0') {
        Serial.println("fail. CLIENT_KEY.");
        preferences.end();
        return false;
    }

    Serial.println("OK. ");

    preferences.end();
    return true;
}

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

void test_creds() {
    if (!creds_read) {
        TEST_FAIL_MESSAGE("Credentials read failed");
    }
}

void wifi_connect() {
    WiFi.begin(SSID, PASSWORD);
    delay(10000);
    TEST_ASSERT_EQUAL(WL_CONNECTED, WiFi.status());
    TEST_ASSERT_TRUE(WiFi.disconnect(true, true));
    delay(2000);
    TEST_ASSERT_FALSE(WiFi.isConnected());
}

void camera_init() {
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // Disable brownout detector

    // Define Flash as an output
    pinMode(LED_BUILTIN, OUTPUT);

    // Config Camera Settings
    camera_config_t config;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;

    //DRAM-only optimized settings
    config.xclk_freq_hz = 8000000;
    config.ledc_timer = LEDC_TIMER_0;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.pixel_format = PIXFORMAT_JPEG;
    config.fb_location = CAMERA_FB_IN_DRAM;
    config.grab_mode = CAMERA_GRAB_LATEST;

    config.frame_size = FRAMESIZE_HVGA;
    config.fb_count = 1;
    config.jpeg_quality = 15;

    esp_err_t err = esp_camera_init(&config);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    err = esp_camera_deinit();
    TEST_ASSERT_EQUAL(ESP_OK, err);
}

void mqtt_connect() {
    WiFiClientSecure net;
    MQTTClient client;

    net.setCACert(SERVER_CA);
    net.setCertificate(CLIENT_CRT);
    net.setPrivateKey(CLIENT_KEY);

    client.begin(MQTT_SERVER, MQTT_PORT, net);
    WiFi.begin(SSID, PASSWORD);
    delay(10000);
    TEST_ASSERT_EQUAL(WL_CONNECTED, WiFi.status());
    client.begin(MQTT_SERVER, MQTT_PORT, net);
    TEST_ASSERT_TRUE(client.connect("test_client"));
    TEST_ASSERT_TRUE(client.connected());
    TEST_ASSERT_TRUE(client.disconnect());
    TEST_ASSERT_FALSE(client.connected());
    TEST_ASSERT_TRUE(WiFi.disconnect(true, true));
    TEST_ASSERT_FALSE(WiFi.isConnected());
}

void mqtt_publish() {
    WiFiClientSecure net;
    MQTTClient client;

    net.setCACert(SERVER_CA);
    net.setCertificate(CLIENT_CRT);
    net.setPrivateKey(CLIENT_KEY);

    client.begin(MQTT_SERVER, MQTT_PORT, net);
    WiFi.begin(SSID, PASSWORD);
    delay(10000);
    TEST_ASSERT_EQUAL(WL_CONNECTED, WiFi.status());
    TEST_ASSERT_TRUE(client.connect("test_client"));
    TEST_ASSERT_TRUE(client.connected());
    TEST_ASSERT_TRUE(client.publish("/unit-test", "test_message"));
    TEST_ASSERT_TRUE(client.disconnect());
    TEST_ASSERT_FALSE(client.connected());
    TEST_ASSERT_TRUE(WiFi.disconnect(true, true));
    TEST_ASSERT_FALSE(WiFi.isConnected());
}

void mqtt_message_callback(String &topic, String &payload) {
    TEST_ASSERT_EQUAL_STRING("/unit-test", topic.c_str());
    TEST_ASSERT_EQUAL_STRING("/test_message", payload.c_str());
}

void mqtt_subscribe() {
    WiFiClientSecure net;
    MQTTClient client;
    char buf[16];

    net.setCACert(SERVER_CA);
    net.setCertificate(CLIENT_CRT);
    net.setPrivateKey(CLIENT_KEY);
    client.begin(MQTT_SERVER, MQTT_PORT, net);
    client.onMessage(mqtt_message_callback);
    WiFi.begin(SSID, PASSWORD);
    delay(10000);
    TEST_ASSERT_EQUAL(WL_CONNECTED, WiFi.status());
    strcpy(buf, "test_message");
    TEST_ASSERT_TRUE(client.connect("test_client"));
    TEST_ASSERT_TRUE(client.connected());
    TEST_ASSERT_TRUE(client.subscribe("/unit-test"));
    TEST_ASSERT_TRUE(client.publish("/unit-test", "test_message"));
    TEST_ASSERT_TRUE(client.publish("/unit-test", buf, strlen(buf) + 1));
    TEST_ASSERT_TRUE(client.disconnect());
    TEST_ASSERT_FALSE(client.connected());
    TEST_ASSERT_TRUE(WiFi.disconnect(true, true));
    TEST_ASSERT_FALSE(WiFi.isConnected());
}

void camera_capture() {
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // Disable brownout detector

    // Define Flash as an output
    pinMode(LED_BUILTIN, OUTPUT);

    // Config Camera Settings
    camera_config_t config;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;

    //DRAM-only optimized settings
    config.xclk_freq_hz = 8000000;
    config.ledc_timer = LEDC_TIMER_0;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.pixel_format = PIXFORMAT_JPEG;
    config.fb_location = CAMERA_FB_IN_DRAM;
    config.grab_mode = CAMERA_GRAB_LATEST;

    config.frame_size = FRAMESIZE_HVGA;
    config.fb_count = 1;
    config.jpeg_quality = 15;

    esp_err_t err = esp_camera_init(&config);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    camera_fb_t *fb = esp_camera_fb_get();
    TEST_ASSERT_NOT_NULL(fb);
    err = esp_camera_deinit();
    TEST_ASSERT_EQUAL(ESP_OK, err);
}

void camera_quality_change() {
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // Disable brownout detector

    // Define Flash as an output
    pinMode(LED_BUILTIN, OUTPUT);

    // Config Camera Settings
    camera_config_t config;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;

    //DRAM-only optimized settings
    config.xclk_freq_hz = 8000000;
    config.ledc_timer = LEDC_TIMER_0;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.pixel_format = PIXFORMAT_JPEG;
    config.fb_location = CAMERA_FB_IN_DRAM;
    config.grab_mode = CAMERA_GRAB_LATEST;

    config.frame_size = FRAMESIZE_HVGA;
    config.fb_count = 1;
    config.jpeg_quality = 15;

    esp_err_t err = esp_camera_init(&config);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    camera_fb_t *fb = esp_camera_fb_get();
    TEST_ASSERT_NOT_NULL(fb);

    size_t initial_quality = fb->len;

    esp_camera_fb_return(fb);
    
    sensor_t *s = esp_camera_sensor_get();
    TEST_ASSERT_NOT_NULL(s);
    int new_quality = min(s->status.quality + 5, 63); // Reduce quality more aggressively
    s->set_quality(s, new_quality);
    s->set_framesize(s, FRAMESIZE_QVGA);
    s->set_vflip(s, 1);
    s->set_hmirror(s, 1);
    s->set_brightness(s, 1);//up the brightness just a bit
    s->set_contrast(s, 1);
    s->set_saturation(s, 1);//lower the saturation 
    TEST_ASSERT_EQUAL(new_quality, s->status.quality);
    TEST_ASSERT_LESS_THAN(initial_quality, new_quality);
    TEST_ASSERT_EQUAL(FRAMESIZE_QVGA, s->status.framesize);
    TEST_ASSERT_EQUAL(1, s->status.vflip);
    TEST_ASSERT_EQUAL(1, s->status.hmirror);
    TEST_ASSERT_EQUAL(1, s->status.brightness);
    TEST_ASSERT_EQUAL(1, s->status.contrast);
    TEST_ASSERT_EQUAL(1, s->status.saturation);

    err = esp_camera_deinit();
    TEST_ASSERT_EQUAL(ESP_OK, err);
}

// Function to calculate SHA256 checksum of downloaded data
String calculateSHA256(uint8_t* data, size_t len) {
    mbedtls_sha256_context ctx;
    unsigned char digest[32];
    char sha256String[65] = {0};
    
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, 0); // 0 for SHA256
    mbedtls_sha256_update(&ctx, data, len);
    mbedtls_sha256_finish(&ctx, digest);
    mbedtls_sha256_free(&ctx);
    
    for (int i = 0; i < 32; i++) {
        sprintf(&sha256String[i * 2], "%02x", digest[i]);
    }
    
    return String(sha256String);
}

// Function to fetch string from URL
String fetchString(const String& url) {
    HTTPClient http;
    http.begin(url);
    
    int httpCode = http.GET();
    String payload = "";
    
    if (httpCode == HTTP_CODE_OK) {
        payload = http.getString();
        payload.trim(); // Remove any whitespace/newlines
    } else {
        Serial.printf("HTTP GET failed for %s, error: %d\n", url.c_str(), httpCode);
    }
    
    http.end();
    return payload;
}

void download_and_check_ota_file() {
    // Step 0: Connect to Wi-Fi
    WiFi.begin(SSID, PASSWORD);
    delay(10000);
    TEST_ASSERT_EQUAL(WL_CONNECTED, WiFi.status());
    
    // Step 1: Download firmware
    HTTPClient http;
    char firmware_url[256];
    sprintf(firmware_url, "%s/%s", OTA_BASE_URL, "firmware.bin");
    http.begin(firmware_url);
    
    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK) {
        http.end();
        TEST_FAIL_MESSAGE("Firmware download failed.");
    }
    
    int contentLength = http.getSize();
    if (contentLength <= 0) {
        http.end();
        TEST_FAIL_MESSAGE("Invalid firmware size.");
  }
  
  
    // Allocate buffer for firmware
    uint8_t* firmwareBuffer = (uint8_t*)malloc(contentLength);
    if (!firmwareBuffer) {
        http.end();
        TEST_FAIL_MESSAGE("Failed to allocate memory for firmware.");
    }
    
    // Download firmware data
    WiFiClient* stream = http.getStreamPtr();
    size_t bytesRead = 0;
    
    while (bytesRead < contentLength) {
        size_t available = stream->available();
        if (available > 0) {
            size_t toRead = min(available, (size_t)(contentLength - bytesRead));
            stream->readBytes(firmwareBuffer + bytesRead, toRead);
            bytesRead += toRead;
        }
    }
    
    http.end();
    
    // Step 3: Verify checksum
    String calculatedChecksum = calculateSHA256(firmwareBuffer, contentLength);
    char checksum_url[256];
    sprintf(checksum_url, "%s/%s", OTA_BASE_URL, "checksum.txt");
    String expectedChecksum = fetchString(checksum_url);
    
    if (expectedChecksum.length() == 0) {
        free(firmwareBuffer);
        TEST_FAIL_MESSAGE("Failed to fetch expected checksum.");
    }
    
    // Convert to lowercase for comparison
    calculatedChecksum.toLowerCase();
    expectedChecksum.toLowerCase();
    
    if (calculatedChecksum != expectedChecksum) {
        free(firmwareBuffer);
        TEST_FAIL_MESSAGE("Checksum verification failed!");
    }

    free(firmwareBuffer);

    TEST_ASSERT_TRUE(WiFi.disconnect(true, true));
    delay(2000);
    TEST_ASSERT_FALSE(WiFi.isConnected());
  
}

void run_tests() {
    UNITY_BEGIN();

    RUN_TEST(camera_init);
    RUN_TEST(camera_capture);
    RUN_TEST(camera_quality_change);
    RUN_TEST(test_creds);
    if (creds_read) {
        RUN_TEST(wifi_connect);
        RUN_TEST(mqtt_connect);
        RUN_TEST(mqtt_publish);
        RUN_TEST(mqtt_subscribe);
        RUN_TEST(download_and_check_ota_file);
    }
    UNITY_END();
}

/**
  * For Arduino framework
  */
void setup() {
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // disable brownout detector
    creds_read = readCredentials();
    // Wait ~2 seconds before the Unity test runner
    // establishes connection with a board Serial interface
    delay(2000);

    run_tests();
}
void loop() {}
