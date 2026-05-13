#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "DHT.h"

#define DHTPIN 4
#define DHTTYPE DHT11
#define LED_PIN 2
#define SERIAL_BAUD 115200

DHT dht(DHTPIN, DHTTYPE);

const char* ssid = "bugs";
const char* password = "bugs1234";

const char* serverExecUrl = "https://script.google.com/macros/s/AKfycbz7_t8xStoQTf2lpGCaossulyQiK_z9tEYfgA1xYmczouOJoSVKCxVhTcUE0FYAQJJr/exec";

unsigned long previousMillis = 0;
const unsigned long interval = 120000;

void sendData();
int getWithOptionalRedirect(WiFiClientSecure& client, const String& url, String& responseOut, String& locationOut);

// WiFi 재연결
void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  Serial.println("WiFi 재연결 시도...");
  WiFi.disconnect(true);
  delay(200);
  WiFi.begin(ssid, password);

  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) {
    delay(500);
    Serial.print(".");
    retry++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi 연결 성공");
    digitalWrite(LED_PIN, HIGH);
  } else {
    Serial.println("\nWiFi 연결 실패 -> 재부팅");
    ESP.restart();
  }
}

// LED 점멸
void blinkLED(int count) {
  for (int i = 0; i < count; i++) {
    digitalWrite(LED_PIN, LOW);
    delay(150);
    digitalWrite(LED_PIN, HIGH);
    delay(150);
  }
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(1000);
  Serial.println();
  Serial.println("=== ESP32 BOOT ===");
  Serial.printf("Baud: %d\n", SERIAL_BAUD);

  dht.begin();
  Serial.println("DHT init 완료");

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  WiFi.begin(ssid, password);
  Serial.print("WiFi 연결 중");

  unsigned long wifiStart = millis();
  const unsigned long wifiTimeoutMs = 20000;

  while (WiFi.status() != WL_CONNECTED && millis() - wifiStart < wifiTimeoutMs) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi 연결 완료");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    digitalWrite(LED_PIN, HIGH);
  } else {
    Serial.println("\nWiFi 연결 타임아웃");
  }

  Serial.println("초기 전송 시작");
  sendData();
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    sendData();
  }
}

// 핵심 전송
void sendData() {
  connectWiFi();

  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  if (isnan(temp) || isnan(hum)) {
    Serial.println("센서 읽기 실패");
    return;
  }

  Serial.printf("\n온도: %.1f / 습도: %.1f\n", temp, hum);
  blinkLED(2);

  // Apps Script의 JSON.parse(e.postData.contents)와 맞추기 위해 JSON으로 전송
  String requestUrl = String(serverExecUrl)
    + "?temperature=" + String(temp, 1)
    + "&humidity=" + String(hum, 1);

  Serial.println("요청 URL:");
  Serial.println(requestUrl);

  WiFiClientSecure client;
  client.setInsecure();

  const int maxRetry = 3;

  for (int i = 1; i <= maxRetry; i++) {
    Serial.printf("요청 시도 %d\n", i);

    String response;
    String redirectLocation;
    int httpCode = getWithOptionalRedirect(client, requestUrl, response, redirectLocation);

    Serial.print("HTTP Code: ");
    Serial.println(httpCode);

    Serial.println("응답:");
    Serial.println(response);

    if (redirectLocation.length() > 0) {
      Serial.print("Redirect Location: ");
      Serial.println(redirectLocation);
    }

    if ((httpCode == 405 || httpCode == 400) && response.indexOf("Google Drive") >= 0) {
      Serial.println("오류 힌트: Apps Script 웹앱 URL 또는 배포 권한을 확인하세요.");
    }

    if (httpCode > 0 && httpCode < 300) {
      StaticJsonDocument<512> resDoc;
      DeserializationError error = deserializeJson(resDoc, response);

      if (!error) {
        const char* result = resDoc["result"] | "응답 없음";
        Serial.println("AI 응답:");
        Serial.println(result);
      } else {
        Serial.println("JSON 파싱 실패 (응답이 JSON이 아닐 수 있음)");
      }
      return;
    }

    if (i < maxRetry) {
      Serial.println("재시도 대기...");
      delay(2000 * i);
    }
  }

  Serial.println("최대 재시도 실패");
}

int getWithOptionalRedirect(WiFiClientSecure& client, const String& url, String& responseOut, String& locationOut) {
  const char* responseHeaderKeys[] = {"Location"};
  const size_t responseHeaderCount = 1;

  HTTPClient http;
  if (!http.begin(client, url)) {
    responseOut = "HTTP begin 실패";
    return -1;
  }

  http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
  http.setTimeout(15000);
  http.collectHeaders(responseHeaderKeys, responseHeaderCount);
  http.addHeader("Accept", "application/json");

  int httpCode = http.GET();
  responseOut = http.getString();
  locationOut = http.header("Location");
  http.end();

  if ((httpCode == 302 || httpCode == 307 || httpCode == 308) && locationOut.length() > 0) {
    HTTPClient redirected;
    if (!redirected.begin(client, locationOut)) {
      responseOut = "리다이렉트 begin 실패";
      return -2;
    }

    redirected.setTimeout(15000);
    redirected.addHeader("Accept", "application/json");

    int redirectedCode = redirected.GET();
    responseOut = redirected.getString();
    redirected.end();
    return redirectedCode;
  }

  return httpCode;
}
