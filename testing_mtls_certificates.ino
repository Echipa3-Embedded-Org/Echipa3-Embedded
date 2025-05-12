#include <Arduino.h>
#include <MQTT.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

const char ssid[] = "DIGI-x39P";
const char pass[] = "f988c1dc";

// MQTT Broker details
const char* mqtt_server = "192.168.100.53";
const int mqtt_port = 8883;

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

unsigned long lastMillis = 0;

void messageReceived(String &topic, String &payload) {
  Serial.println("Message received on " + topic + ": " + payload);
}

void connectToMQTT() {
  Serial.print("Connecting to MQTT...");
  
  while (!client.connect("myclient")) {
    Serial.print(".");
    delay(1000);
  }
  
  Serial.println("\nConnected!");
  client.subscribe("/test");
}

void setup() {
  Serial.begin(115200);
  
  // Connect to WiFi
  WiFi.begin(ssid, pass);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\nWiFi connected!");

  // Configure secure client with certificates
  net.setCACert(serverCA);
  net.setCertificate(clientCert);
  net.setPrivateKey(clientKey);

  // Initialize MQTT client
  client.begin(mqtt_server, mqtt_port, net);
  client.onMessage(messageReceived);

  connectToMQTT();
}

void loop() {
  client.loop();
  
  if (!client.connected()) {
    connectToMQTT();
  }

  // Publish a message every 10 seconds
  if (millis() - lastMillis > 10000) {
    lastMillis = millis();
    client.publish("/test", "Hello from ESP32");
    Serial.println("Message published");
  }
}