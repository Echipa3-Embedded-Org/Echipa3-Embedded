#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <MQTT.h>
#include "esp_camera.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"


const char ssid[] = "DIGI-x39P";
const char pass[] = "xxxx";

// MQTT Broker details
const char* mqtt_server = "192.168.100.53";
const int mqtt_port = 8883;
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

// Certificate strings - replace with your actual PEM formatted certificates
const char* serverCA = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDiTCCAnGgAwIBAgIUDDScn5yWrTaVDRJAqO4IlUSSmzswDQYJKoZIhvcNAQEL
BQAwVDELMAkGA1UEBhMCQVUxEzARBgNVBAgMClNvbWUtU3RhdGUxITAfBgNVBAoM
GEludGVybmV0IFdpZGdpdHMgUHR5IEx0ZDENMAsGA1UEAwwEbXlDQTAeFw0yNTA1
MDUxMDA0NDhaFw0zMDA1MDUxMDA0NDhaMFQxCzAJBgNVBAYTAkFVMRMwEQYDVQQI
DApTb21lLVN0YXRlMSEwHwYDVQQKDBhJbnRlcm5ldCBXaWRnaXRzIFB0eSBMdGQx
DTALBgNVBAMMBG15Q0EwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDZ
codVfiG/2uZrJgAZ5idhDx3KdwOwKivy/oTGu8r2oxYm+stWRXIhPQn3aMEUUjSw
UU0t3eSOnTw5VbZqqS7M0xEBUVRjmFxjdufvxdgy246fLXbJkkybUnzJrdZyYkcU
kAM/a4sQj+bYk5MHOScGGOJdVmp9lLCKmb9H6SphKIah0wS3UvRHp+2G3iBtim2H
6PWNm1Vr140RoUeQzSzJG3+PhSCK2MdIT12+cMAdLGMEBc96qpJMbpVhdZxR4fW4
sTjmEwBpOPSU68ZTG+TvMGhPmv37hl0NSHpNlFI6Z3R8me3659nEsKinIAW6Us0G
LdyXLB5BQP6gQlCiPXKZAgMBAAGjUzBRMB0GA1UdDgQWBBRn5skfgcV88DdlS3P6
G1AjYzCw6DAfBgNVHSMEGDAWgBRn5skfgcV88DdlS3P6G1AjYzCw6DAPBgNVHRMB
Af8EBTADAQH/MA0GCSqGSIb3DQEBCwUAA4IBAQAXGQncUOuMFN6Th+VzvnUoCrKc
3jPJLnicA2M6niNzJyNVO6uTcxwsrq79izov7VGWyk6jq4tAGq96Vla9N9N67UeC
b77ZIVbTR2O2mOd9bpKmJQkRzCvyx9YDlPSc3Etr4hI6l4TaCcrnFLCc6weE3CAC
StDCDk5YEORsMssVm/a13vTF+veXZCOvipjjRl+9h3zQZiPUilHhJomiBOparW/t
sl9e6X892bzVVAFB4uyPrelTqnpGMZzeuea7i44dml4SEz9v/xAGgq9sANGAkcrj
kgVFPC99ZRZMgkSZOj4GUaDNwxKU0LQdm4HUUP6BDzrbfAfpONH9LzU8Q9Bx
-----END CERTIFICATE-----
)EOF";

const char* clientCert = R"KEY(
-----BEGIN CERTIFICATE-----
MIIDMzCCAhsCFH0to6NiPfnhVcoliFdZfcvMTDSnMA0GCSqGSIb3DQEBCwUAMFQx
CzAJBgNVBAYTAkFVMRMwEQYDVQQIDApTb21lLVN0YXRlMSEwHwYDVQQKDBhJbnRl
cm5ldCBXaWRnaXRzIFB0eSBMdGQxDTALBgNVBAMMBG15Q0EwHhcNMjUwNTA1MTAw
NzAxWhcNMjYwNDMwMTAwNzAxWjBYMQswCQYDVQQGEwJBVTETMBEGA1UECAwKU29t
ZS1TdGF0ZTEhMB8GA1UECgwYSW50ZXJuZXQgV2lkZ2l0cyBQdHkgTHRkMREwDwYD
VQQDDAhteWNsaWVudDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAO4f
dZzbAmtOvc/oPFJZwXQ9t3CSnssKdwZBpQ4hJecqLKVrUdXn4GU6u5QQjaOdc9qi
nLImzoB8Ozc0PxgU29w+TMxWV2u6hCBN1l64UDiVQEhEnqs3hUzYKH1vHFwMGR0g
RJT86kvqjGVPLTLDBc9QGLDg9Cy+9ddLZk07nxGAz8zjWHKa0KbmWxeV8a0ijY6m
iZJ19c+nqNqZQt7cOpeuNaxyVGM/Vp+usR/5qc0W5QOAh83f7BDYP6IRWcrOaYHH
WTgQw28gfhBe/4ikBhjHpxPYFPf8DWzS93e8vANn5cmxTiPJCEK8lpGzly9SV+DK
T/0RGvE8UflyMzeG28ECAwEAATANBgkqhkiG9w0BAQsFAAOCAQEAJwk1QmRcAr6d
w9NPZL+RTM/GgR8Fo7OXHYY10BgVOnvcxu3CniiGelzhGRn/t/RIQ2rtUajf4Ft8
wUrJM8LeBwbCmkpVH/OJFHsl6V3tMlQjQz0zRaNY8pqNWp8F3nAhMxDRPbp5wIfl
/9kh59diYRtgqZsh9IZ26CPorYFsqNXmzpPIH5XIfZhIr1zObLGO2gpW3xftu7dj
cBM4jfZtc8QKmLJu3OUi8UWmaVMsPeRyt5JL1OMhjwjZ50HEjU6Fd/lYFneB0v1s
BidJZg8AoJ/+JTbC7NvCmYEgoLFmY3tC84ZsTTzSmiBGd2HM5Zhemsu0dj7bkSZU
nJAcmciqPA==
-----END CERTIFICATE-----
)KEY";

const char* clientKey = R"KEY(
-----BEGIN PRIVATE KEY-----
MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQDuH3Wc2wJrTr3P
6DxSWcF0Pbdwkp7LCncGQaUOISXnKiyla1HV5+BlOruUEI2jnXPaopyyJs6AfDs3
ND8YFNvcPkzMVldruoQgTdZeuFA4lUBIRJ6rN4VM2Ch9bxxcDBkdIESU/OpL6oxl
Ty0ywwXPUBiw4PQsvvXXS2ZNO58RgM/M41hymtCm5lsXlfGtIo2OpomSdfXPp6ja
mULe3DqXrjWsclRjP1afrrEf+anNFuUDgIfN3+wQ2D+iEVnKzmmBx1k4EMNvIH4Q
Xv+IpAYYx6cT2BT3/A1s0vd3vLwDZ+XJsU4jyQhCvJaRs5cvUlfgyk/9ERrxPFH5
cjM3htvBAgMBAAECggEASR5n4w5tI74+EsVhIHhnKuZ4mZysHfTMr6Mgp8IN/Xm5
0+Gsc16MTQlH6CNdGsyScqRUMXAxIMsE8+KbLS3ahTqsImKw3Wecgr7kAJngKD/M
SWSoxDJ8QChv5nRj5O/iFkt+Q7GV0FHpYoJ5gojYE9yoeEfbMpJAsyd39mdEZP7t
KVKhO01UmcNGFVW17M9Rnj39tTAMsk4comqSXke3+SIvoc5e0b2N4d4YeTnFUA9u
M1/ADZCCgO6ZRfEbkHgmo7w5w1movgzureAAI2t7hahpVirADHjPEvdKUJRckVtU
unqcPj3v8K4ubIsPTSDMrLTrU/4e5wtD4sCDldsjcQKBgQD7G7IT/jj/4H8XHLjG
Baidx7+gYDsLqEupJI5aCHgi9DwflDOkQo5sIH1ZwNdFrkEKrTPpi85ZitzFL3bU
Og7hhmu5D1O0HmrIBdobr7ctaiC7JIjicZXxISLbrIfPBuqMAjlg7PGIX+gpHKZM
3fkjhQF1f/QMDbYqFlTADt3nRwKBgQDywwEzLiD9aYptv5YLG2Abd4bxgDa32ZJ1
c37qjoP34CyehCU833PO+cYZC/Xiv6v/PlvXB0o0uZCxzZsP84TFWZxH4UDMdOsU
ZWpnizq2kznjjpOo5Gwr64l4jtLbTrhmrkn+gQ34F8YJ5Hhach3msIm0kXLqFZ4i
0u29xrk4twKBgQD60SYlrDfY9a9cMZcqTHqo5t00Xwp7UWYJk/cQXQdKurPXQxv2
BXjm7ejnHqSn+C8FcA27SKcbb4Wm/ArwXvGAONkepv2PmxZDpvy9zNzl4uzAoPSN
YrFHgjakP4gDtT/QC2SuuN1kv49QiUpe2xVAclkLuXvWElgTX+zVTcQ+fwKBgFpr
W5UOh2cb2l2KUH6OkbtpR6/Hy5mqxixM+mau8mRu8O+R1LTZna7nxWsq00jDj3Kg
bWCn7HG69DPlmu3UDA3dlKIJOjNtEOol4/3xE8tRBuzE/CaZ/dhWAHwQ5mSc634D
SLgspWFUqYShvlohyxVTh9bjneOmbaIW4kofLHn9AoGASS2kxMVfgGq1jJHC03Wf
XgYiopLogPrMB3wRyLjpwUBhr3dPmldZCrQoTuRM88+0x9ABuW0jWl9iSIB4ytnH
5LQrnsTGE955Nsr3IXoSdgFtYXgLzHu3IdahWwSF0h7P7wSXU5/NLzA07DXCtVbU
oIkKJl3t9B107nAPxyLfSV4=
-----END PRIVATE KEY-----
)KEY";

WiFiClientSecure net;
MQTTClient client;


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
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
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
  WiFi.begin(ssid, pass);
  
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
  


// ================== Main Functions ==================
void setup() {
  Serial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  connectToWiFi();
  setupCamera();

  // Configure secure client with certificates
  net.setCACert(serverCA);
  net.setCertificate(clientCert);
  net.setPrivateKey(clientKey);

  // Initialize MQTT client
  client.begin(mqtt_server, mqtt_port, net);
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
