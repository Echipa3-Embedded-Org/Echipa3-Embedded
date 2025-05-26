#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <MQTT.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <Update.h>
#include <mbedtls/sha256.h>
#include "esp_camera.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_secure_boot.h"

#define SECRETS_NAMESPACE "mqtt-secrets"
#define READ_ONLY_SECRETS_NAMESPACE true

#define OTA_VERSION "@OTA_VERSION@"

// Credentials
char SSID[64];
char PASSWORD[64];
char MQTT_SERVER[256];
int32_t MQTT_PORT;
char SERVER_CA[2048];
char CLIENT_CRT[2048];
char CLIENT_KEY[2048];
char OTA_BASE_URL[256];

// MQTT Broker details
const char* topic_PHOTO = "SMILE";
const char* topic_PUBLISH = "PICTURE";
const char* topic_FLASH = "FLASH";
const char* topic_LIVE= "LIVE";
const char* topic_LIVE_IMAGE = "LIVE_IMAGE";
const char* topic_LIVE_STATUS = "LIVE_STATUS";
const int MAX_PAYLOAD = 25000;  // MQTT max recommended payload

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

// Flash
#define LED_BUILTIN 4

bool flash = false;
bool flashActive = false;
bool liveMode = false;

unsigned long lastMillis = 0;
unsigned long flashStartTime = 0;
unsigned long lastLiveImage = 0;
const int LIVE_INTERVAL = 200; //ms
const int FLASH_DURATION = 500; // Increased from instantaneous to 500ms

WiFiClientSecure net;
MQTTClient client;
Preferences preferences;


void setupCamera() {
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
  
  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    config.frame_size = FRAMESIZE_QVGA;
    ESP_ERROR_CHECK(esp_camera_init(&config));
    delay(1000);
  }

  Serial.printf("Camera initialized at %dx%d resolution\n",
               config.frame_size == FRAMESIZE_HVGA ? 480 : 320,
               config.frame_size == FRAMESIZE_HVGA ? 320 : 240);

  sensor_t *s = esp_camera_sensor_get();
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
  s->set_brightness(s, 1);//up the brightness just a bit
  s->set_contrast(s, 1);
  s->set_saturation(s, 1);//lower the saturation 
  Serial.println("Camera initialized successfully");
}


// ================== MQTT FUNCTIONS ==================

void take_picture() {
  Serial.println("Attempting to capture image...");

  if(flash) {
  digitalWrite(LED_BUILTIN, HIGH);
  flashActive = true;
  flashStartTime = millis();
  delay(50); // Let the flash stabilize
  }

  // 2. Capture with retry mechanism (new)
  camera_fb_t *fb = esp_camera_fb_get();
  if(!fb) {
    Serial.println("Capture failed");
    if(flashActive) {
      digitalWrite(LED_BUILTIN, LOW);
      flashActive = false;
    }
    return;
  }

  if(fb->len > MAX_PAYLOAD) {
    Serial.printf("Image too large (%d bytes). Reducing quality...\n", fb->len);
    esp_camera_fb_return(fb);
    
    sensor_t *s = esp_camera_sensor_get();
    int new_quality = min(s->status.quality + 5, 63); // Reduce quality more aggressively
    s->set_quality(s, new_quality);
    Serial.printf("Quality reduced to %d\n", new_quality);
    return;
  }

  bool published = client.publish(topic_PUBLISH, (const char*)fb->buf, fb->len);
  Serial.printf("Published %d bytes: %s\n", fb->len, published ? "OK" : "FAIL");
  
  // Clean up
  esp_camera_fb_return(fb);
    
  // Flash will be turned off by checkFlashTimeout()
}

void take_live_image() {
  if (!liveMode) return;

  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb || fb->len < 100) {
    if (fb) esp_camera_fb_return(fb);
    return;
  }

  // Debug output
  Serial.printf("Sending live frame: %d bytes\n", fb->len);
  
  if (!client.publish(topic_LIVE_IMAGE, (const char*)fb->buf, fb->len)) {
    Serial.println("Live publish failed");
  }
  
  esp_camera_fb_return(fb);
}

void set_flash() {
  flash = !flash;
  Serial.printf("Flash %s\n", flash ? "ON" : "OFF");
}

void set_live_mode(bool mode) {
  if (liveMode == mode) return;
  
  liveMode = mode;
  Serial.printf("Live mode %s\n", liveMode ? "ON" : "OFF");
  
  // Send status update
  client.publish(topic_LIVE_STATUS, liveMode ? "ON" : "OFF");
  
  // Adjust camera settings if needed
  if (liveMode) {
    // Configure camera for live mode (lower resolution/quality)
    sensor_t *s = esp_camera_sensor_get();
    s->set_framesize(s, FRAMESIZE_QVGA);
    s->set_quality(s, 10);
  }
}

void messageReceived(String &topic, String &payload) {
  Serial.println("Message received on " + topic + ": " + payload);
  
  if (topic == topic_PHOTO) take_picture();
  if (topic == topic_FLASH) set_flash();
  if (topic == topic_LIVE) {
    if (payload == "ON") {
      set_live_mode(true);
    } 
      else if (payload == "OFF") {
        set_live_mode(false);
      }
    }
}

void connectToWiFi() {
  if(WiFi.status() == WL_CONNECTED) return;
  
  Serial.print("Connecting to WiFi...");
  WiFi.begin(SSID, PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    Serial.print(".");
    delay(500);
    attempts++;
  }
  
  if(WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("\nWiFi connection failed");
  }
}

void connectToMQTT() {
  if(client.connected()) return;
  Serial.print("Connecting to MQTT...");
  
  int attempts = 0;
  while (!client.connect("myclient") && attempts < 5) {
    Serial.print(".");
    delay(1000);
    attempts++;
  }
  
  if(client.connected()) {
    Serial.println("\nMQTT connected!");
    client.subscribe(topic_PHOTO);
    client.subscribe(topic_FLASH);
    client.subscribe(topic_LIVE);
  } else {
    Serial.println("\nMQTT connection failed");
  }
}

void checkFlashTimeout() {
  if(flashActive && (millis() - flashStartTime >= FLASH_DURATION)) {
    digitalWrite(LED_BUILTIN, LOW);
    flashActive = false;
  }
}

void readCredentials() {
  Serial.print("Reading credentials... ");

  preferences.begin(SECRETS_NAMESPACE, READ_ONLY_SECRETS_NAMESPACE);

  strcpy(SSID, preferences.getString("SSID", "").c_str());
  if (SSID[0] == '\0') {
    Serial.println("fail. SSID.");
    preferences.end();
    return;
  }
  strcpy(PASSWORD, preferences.getString("PASSWORD", "").c_str());
  if (PASSWORD[0] == '\0') {
    Serial.println("fail. PASSWORD.");
    preferences.end();
    return;
  }
  strcpy(MQTT_SERVER, preferences.getString("MQTT_SERVER", "").c_str());
  if (MQTT_SERVER[0] =='\0') {
    Serial.println("fail. MQTT_SERVER.");
    preferences.end();
    return;
  }
  MQTT_PORT = preferences.getInt("MQTT_PORT", 0);
  if (MQTT_PORT == 0) {
    Serial.println("fail. MQTT_PORT.");
    preferences.end();
    return;
  }
  strcpy(OTA_BASE_URL, preferences.getString("OTA_BASE_URL", "").c_str());
  if (OTA_BASE_URL[0] =='\0') {
    Serial.println("fail. OTA_BASE_URL.");
    preferences.end();
    return;
  }
  strcpy(SERVER_CA, preferences.getString("SERVER_CA", "").c_str());
  if (SERVER_CA[0] == '\0') {
    Serial.println("fail. SERVER_CA.");
    preferences.end();
    return;
  }
  strcpy(CLIENT_CRT, preferences.getString("CLIENT_CRT", "").c_str());
  if (CLIENT_CRT[0] == '\0') {
    Serial.println("fail. CLIENT_CRT.");
    preferences.end();
    return;
  }
  strcpy(CLIENT_KEY, preferences.getString("CLIENT_KEY", "").c_str());
  if (CLIENT_KEY[0] == '\0') {
    Serial.println("fail. CLIENT_KEY.");
    preferences.end();
    return;
  }
  
  Serial.println("OK. ");

  preferences.end();
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

// Main OTA update function
bool performOTAUpdate() {
  Serial.println("Checking for OTA updates...");
  
  // Step 1: Check version
  char version_url[256];
  sprintf(version_url, "%s/%s", OTA_BASE_URL, "version.txt");
  String remoteVersion = fetchString(version_url);
  if (remoteVersion.length() == 0) {
    Serial.println("Failed to fetch remote version");
    return false;
  }
  
  Serial.printf("Local version: %s\n", OTA_VERSION);
  Serial.printf("Remote version: %s\n", remoteVersion.c_str());
  
  if (remoteVersion == OTA_VERSION) {
    Serial.println("Firmware is up to date");
    return true;
  }
  
  Serial.println("New firmware version available, starting download...");
  
  // Step 2: Download firmware
  HTTPClient http;
  char firmware_url[256];
  sprintf(firmware_url, "%s/%s", OTA_BASE_URL, "firmware.bin");
  http.begin(firmware_url);
  
  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("Firmware download failed, error: %d\n", httpCode);
    http.end();
    return false;
  }
  
  int contentLength = http.getSize();
  if (contentLength <= 0) {
    Serial.println("Invalid firmware size");
    http.end();
    return false;
  }
  
  Serial.printf("Firmware size: %d bytes\n", contentLength);
  
  // Allocate buffer for firmware
  uint8_t* firmwareBuffer = (uint8_t*)malloc(contentLength);
  if (!firmwareBuffer) {
    Serial.println("Failed to allocate memory for firmware");
    http.end();
    return false;
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
      
      // Print progress
      if (bytesRead % 1024 == 0 || bytesRead == contentLength) {
        Serial.printf("Downloaded: %d/%d bytes (%d%%)\n", 
                     bytesRead, contentLength, (bytesRead * 100) / contentLength);
      }
    }
  }
  
  http.end();
  
  // Step 3: Verify checksum
  String calculatedChecksum = calculateSHA256(firmwareBuffer, contentLength);
  char checksum_url[256];
  sprintf(checksum_url, "%s/%s", OTA_BASE_URL, "checksum.txt");
  String expectedChecksum = fetchString(checksum_url);
  
  if (expectedChecksum.length() == 0) {
    Serial.println("Failed to fetch expected checksum");
    free(firmwareBuffer);
    return false;
  }
  
  // Convert to lowercase for comparison
  calculatedChecksum.toLowerCase();
  expectedChecksum.toLowerCase();
  
  Serial.printf("Calculated SHA256: %s\n", calculatedChecksum.c_str());
  Serial.printf("Expected SHA256: %s\n", expectedChecksum.c_str());
  
  if (calculatedChecksum != expectedChecksum) {
    Serial.println("Checksum verification failed!");
    free(firmwareBuffer);
    return false;
  }
  
  Serial.println("Checksum verification passed");
  
  // Step 4: Start OTA update
  Serial.println("Starting OTA update...");
  
  if (!Update.begin(contentLength)) {
    Serial.printf("OTA begin failed: %s\n", Update.errorString());
    free(firmwareBuffer);
    return false;
  }
  
  size_t written = Update.write(firmwareBuffer, contentLength);
  if (written != contentLength) {
    Serial.printf("OTA write failed: wrote %d of %d bytes\n", written, contentLength);
    Update.abort();
    free(firmwareBuffer);
    return false;
  }
  
  if (!Update.end()) {
    Serial.printf("OTA end failed: %s\n", Update.errorString());
    free(firmwareBuffer);
    return false;
  }
  
  free(firmwareBuffer);
  
  Serial.println("OTA update completed successfully!");
  Serial.println("Restarting in 3 seconds...");
  
  delay(3000);
  ESP.restart();
  
  return true;
}

// Call this function in your loop() or from a timer
void checkForOTAUpdate() {
  if (WiFi.status() == WL_CONNECTED) {
    performOTAUpdate();
  } else {
    Serial.println("WiFi not connected, skipping OTA check");
  }
}


// ================== Main Functions ==================
void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // disable brownout detector
  Serial.begin(115200);
  
  readCredentials();

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  connectToWiFi();
  checkForOTAUpdate();
  setupCamera();

  // Configure secure client with certificates
  net.setCACert(SERVER_CA);
  net.setCertificate(CLIENT_CRT);
  net.setPrivateKey(CLIENT_KEY);

  // Initialize MQTT client
  client.begin(MQTT_SERVER, MQTT_PORT, net);
  client.onMessage(messageReceived);
  connectToMQTT();

  Serial.println("System initialized");
}

//////////////////////////////////////////////////////////////////////////////////
void loop() {

   // Maintain WiFi connection
  if(WiFi.status() != WL_CONNECTED) {
    connectToWiFi();
  }

  client.loop();
  if (!client.connected()) {
    connectToMQTT();
  }
  
  checkFlashTimeout();
  take_live_image();

  // Publish a message every 10 seconds
  if (millis() - lastMillis > 30000) {
    lastMillis = millis();
    if(client.connected()) {
    client.publish("/test", "Hello from ESP32");
    Serial.println("Message published");
    }
  }
  delay(10); // Small delay to prevent watchdog triggers
}
