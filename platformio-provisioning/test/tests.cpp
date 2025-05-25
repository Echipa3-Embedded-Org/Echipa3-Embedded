#include <unity.h>
#include <Preferences.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#include "secrets.h"

#define SECRETS_NAMESPACE "mqtt-secrets"
#define READ_ONLY_SECRETS_NAMESPACE false

Preferences preferences;

void setUp(void) {
    preferences.begin(SECRETS_NAMESPACE, READ_ONLY_SECRETS_NAMESPACE);
}

void tearDown(void) {
    preferences.end();
}

void wifi_secrets() {
    #ifndef SSID
    TEST_FAIL_MESSAGE("SSID not defined");
    #endif
    #ifndef PASSWORD
    TEST_FAIL_MESSAGE("WiFi PASSWORD not defined");
    #endif
}

void mqtt_secrets() {
    #ifndef MQTT_SERVER
    TEST_FAIL_MESSAGE("MQTT_SERVER not defined");
    #endif
    #ifndef MQTT_PORT
    TEST_FAIL_MESSAGE("MQTT_PORT not defined");
    #endif
}

void read_ssid() {
    TEST_ASSERT_EQUAL_STRING(SSID, preferences.getString("SSID", "").c_str());
}

void read_password() {
    TEST_ASSERT_EQUAL_STRING(PASSWORD, preferences.getString("PASSWORD", "").c_str());
}

void read_mqtt_server() {
    TEST_ASSERT_EQUAL_STRING(MQTT_SERVER, preferences.getString("MQTT_SERVER", "").c_str());
}

void read_mqtt_port() {
    TEST_ASSERT_EQUAL(MQTT_PORT, preferences.getInt("MQTT_PORT", 0));
}

void read_ota() {
    TEST_ASSERT_EQUAL_STRING(OTA_BASE_URL, preferences.getString("OTA_BASE_URL", "").c_str());
}

void read_server_ca() {
    TEST_ASSERT_EQUAL_STRING(SERVER_CA, preferences.getString("SERVER_CA", "").c_str());
}

void read_client_crt() {
    TEST_ASSERT_EQUAL_STRING(CLIENT_CRT, preferences.getString("CLIENT_CRT", "").c_str());
}

void read_client_key() {
    TEST_ASSERT_EQUAL_STRING(CLIENT_KEY, preferences.getString("CLIENT_KEY", "").c_str());
}

void read_public_sig_key() {
    TEST_ASSERT_EQUAL_STRING(PUB_SIGN_KEY, preferences.getString("PUB_SIGN_KEY", "").c_str());
}

void write_ssid() {
    TEST_ASSERT_EQUAL(strlen(SSID), preferences.putString("SSID", SSID));
}

void write_password() {
    TEST_ASSERT_EQUAL(strlen(PASSWORD), preferences.putString("PASSWORD", PASSWORD));
}

void write_mqtt_server() {
    TEST_ASSERT_EQUAL(strlen(MQTT_SERVER), preferences.putString("MQTT_SERVER", MQTT_SERVER));
}

void write_mqtt_port() {
    TEST_ASSERT_EQUAL(sizeof(int32_t), preferences.putInt("MQTT_PORT", MQTT_PORT));
}

void write_ota() {
    TEST_ASSERT_EQUAL(strlen(OTA_BASE_URL), preferences.putString("OTA_BASE_URL", OTA_BASE_URL));
}

void write_server_ca() {
    TEST_ASSERT_EQUAL(strlen(SERVER_CA), preferences.putString("SERVER_CA", SERVER_CA));
}

void write_client_crt() {
    TEST_ASSERT_EQUAL(strlen(CLIENT_CRT), preferences.putString("CLIENT_CRT", CLIENT_CRT));
}

void write_client_key() {
    TEST_ASSERT_EQUAL(strlen(CLIENT_KEY), preferences.putString("CLIENT_KEY", CLIENT_KEY));
}

void write_public_sig_key() {
    TEST_ASSERT_EQUAL(strlen(PUB_SIGN_KEY), preferences.putString("PUB_SIGN_KEY", PUB_SIGN_KEY));
}

void run_tests() {
    UNITY_BEGIN();

    RUN_TEST(wifi_secrets);
    RUN_TEST(mqtt_secrets);
    RUN_TEST(read_ssid);
    RUN_TEST(read_password);
    RUN_TEST(read_mqtt_server);
    RUN_TEST(read_mqtt_port);
    RUN_TEST(read_ota);
    RUN_TEST(read_server_ca);
    RUN_TEST(read_client_crt);
    RUN_TEST(read_client_key);
    RUN_TEST(read_public_sig_key);
    RUN_TEST(write_ssid);
    RUN_TEST(write_password);
    RUN_TEST(write_mqtt_server);
    RUN_TEST(write_mqtt_port);
    RUN_TEST(write_ota);
    RUN_TEST(write_server_ca);
    RUN_TEST(write_client_crt);
    RUN_TEST(write_client_key);
    RUN_TEST(write_public_sig_key);

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
