#include <unity.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <MQTT.h>
#include "secrets.h"
#include "esp_camera.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

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

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

void wifi_secrets() {
    #ifndef SSID
    TEST_FAIL_MESSAGE("SSID not defined");
    #endif
    #ifndef PASSWORD
    TEST_FAIL_MESSAGE("WiFi PASSWORD not defined");
    #endif
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

void mqtt_secrets() {
    #ifndef MQTT_SERVER
    TEST_FAIL_MESSAGE("MQTT_SERVER not defined");
    #endif
    #ifndef MQTT_PORT
    TEST_FAIL_MESSAGE("MQTT_PORT not defined");
    #endif
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

void run_tests() {
    UNITY_BEGIN();

    RUN_TEST(wifi_secrets);
    RUN_TEST(wifi_connect);
    RUN_TEST(camera_init);
    RUN_TEST(mqtt_secrets);
    RUN_TEST(mqtt_connect);
    RUN_TEST(mqtt_publish);
    RUN_TEST(mqtt_subscribe);
    RUN_TEST(camera_capture);
    RUN_TEST(camera_quality_change);

    UNITY_END();
}

/**
  * For Arduino framework
  */
void setup() {
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // disable brownout detector
    // Wait ~2 seconds before the Unity test runner
    // establishes connection with a board Serial interface
    delay(2000);

    run_tests();
}
void loop() {}
