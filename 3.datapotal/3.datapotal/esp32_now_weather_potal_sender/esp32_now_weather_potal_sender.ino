#include <WiFi.h>
#include <ArduinoJson.h>
#include <esp_now.h>
#include <time.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define STASSID "bugs"
#define STAPSK  "bugs1234"

const char* ssid = STASSID;
const char* password = STAPSK;

const char* host = "apis.data.go.kr";
const int httpPort = 80;

/* Receiver MAC */
uint8_t receiverMAC[] = {0xFC,0xE8,0xC0,0x74,0x90,0xEC};

/* 공공데이터 서비스키 */
String serviceKey = "8f2bc89a0bb81252ae1aaa0dd19998c4809670655f36f0c1639873b7c4bdb263";//인코딩

/* 언양읍 좌표 */
String nx = "102";
String ny = "84";

/* 날씨 데이터 구조체 */
typedef struct {
  float temp;
  float humidity;
} WeatherData;

WeatherData weather;

esp_now_peer_info_t peerInfo;

String base_date;
String base_time;

void setup() {
/* OLED 초기화 */
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED 초기화 실패");
    while(true);
  }

  display.clearDisplay();

  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(10,20);
  display.println("IoT Start");

  display.display();

  delay(2000);

  Serial.begin(115200);

  Serial.println("WiFi 연결중...");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi 연결 완료");

  /* NTP 시간 동기화 */
  configTime(9 * 3600, 0, "pool.ntp.org");

  struct tm timeinfo;

  while (!getLocalTime(&timeinfo)) {
    Serial.println("시간 동기화 중...");
    delay(1000);
  }

  char dateBuffer[9];
  char timeBuffer[5];

  strftime(dateBuffer, sizeof(dateBuffer), "%Y%m%d", &timeinfo);
  strftime(timeBuffer, sizeof(timeBuffer), "%H00", &timeinfo);

  base_date = String(dateBuffer);
  base_time = String(timeBuffer);

  Serial.print("현재 날짜: ");
  Serial.println(base_date);

  Serial.print("조회 시간: ");
  Serial.println(base_time);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW 초기화 실패");
    return;
  }

  memcpy(peerInfo.peer_addr, receiverMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  esp_now_add_peer(&peerInfo);
}

void loop() {

  getWeather();

  delay(60000);
}

void getWeather() {

  WiFiClient client;

  if (!client.connect(host, httpPort)) {
    Serial.println("서버 연결 실패");
    return;
  }

  String url = "/1360000/VilageFcstInfoService_2.0/getUltraSrtNcst?";
  url += "serviceKey=" + serviceKey;
  url += "&pageNo=1";
  url += "&numOfRows=10";
  url += "&dataType=JSON";
  url += "&base_date=" + base_date;
  url += "&base_time=" + base_time;
  url += "&nx=" + nx;
  url += "&ny=" + ny;

  Serial.println("요청 URL:");
  Serial.println(url);

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");

  String payload = "";

  while (client.connected() || client.available()) {
    if (client.available()) {
      payload += (char)client.read();
    }
  }

  client.stop();

  int jsonStart = payload.indexOf('{');

  if (jsonStart == -1) {
    Serial.println("JSON 없음");
    return;
  }

  String jsonData = payload.substring(jsonStart);

  StaticJsonDocument<4096> doc;

  DeserializationError error = deserializeJson(doc, jsonData);

  if (error) {
    Serial.println("JSON 파싱 실패");
    return;
  }

  JsonArray items = doc["response"]["body"]["items"]["item"];

  float temp = 0;
  float humidity = 0;

  for (JsonObject item : items) {

    String category = item["category"];
    float value = item["obsrValue"];

    if (category == "T1H") temp = value;
    if (category == "REH") humidity = value;
  }

  Serial.println("===== 언양읍 현재 날씨 =====");

  Serial.print("기온 : ");
  Serial.println(temp);

  Serial.print("습도 : ");
  Serial.println(humidity);

/* OLED 화면 초기화 */
  display.clearDisplay();

  display.setTextSize(2);
  display.setTextColor(WHITE);

  display.setCursor(0,0);
  display.print("Temp:");
  display.print(temp);
  display.print("C");

  display.setCursor(0,30);
  display.print("Hum:");
  display.print(humidity);
  display.print("%");

  display.display();
  weather.temp = temp;
  weather.humidity = humidity;

  esp_now_send(receiverMAC, (uint8_t *) &weather, sizeof(weather));

  Serial.println("ESP-NOW 전송 완료");
}