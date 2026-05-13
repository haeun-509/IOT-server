#include <WiFi.h>
#include <ArduinoJson.h>

#ifndef STASSID
#define STASSID "bugs"
#define STAPSK "bugs1234"
#endif

const char* ssid = STASSID;
const char* password = STAPSK;
const char* host = "apis.data.go.kr";
const int httpPort = 80;

// 공공데이터포털 서비스키 입력
String serviceKey = "8f2bc89a0bb81252ae1aaa0dd19998c4809670655f36f0c1639873b7c4bdb263";  //인코딩
// String serviceKey = ""; //디코딩

// 조회 시간
String base_date = "20260408";
String base_time = "1200";

// 인천 예시 좌표
String nx = "55";
String ny = "124";

void setup() {

  Serial.begin(115200);
  delay(1000);

  Serial.println("WiFi 연결중...");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi 연결 완료");

  getWeather();
}

void loop() {}

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

  client.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "Connection: close\r\n\r\n");

  String payload = "";

  while (client.connected() || client.available()) {
    if (client.available()) {
      char c = client.read();
      payload += c;
    }
  }

  client.stop();

  Serial.println("===== 서버에서 받은 전체 데이터 =====");
  Serial.println(payload);
  Serial.println("====================================");

  // JSON 시작 위치 찾기
  int jsonStart = payload.indexOf('{');

  if (jsonStart == -1) {
    Serial.println("JSON 데이터를 찾지 못함");
    return;
  }

  String jsonData = payload.substring(jsonStart);

  Serial.println("===== JSON 데이터 =====");
  Serial.println(jsonData);
  Serial.println("======================");

  StaticJsonDocument<4096> doc;

  DeserializationError error = deserializeJson(doc, jsonData);

  if (error) {
    Serial.print("JSON 파싱 실패 : ");
    Serial.println(error.f_str());
    return;
  }

  JsonArray items = doc["response"]["body"]["items"]["item"];

  float temp = 0;
  float humidity = 0;

  Serial.println("===== 데이터 목록 =====");

  for (JsonObject item : items) {

    String category = item["category"];
    float value = item["obsrValue"];

    Serial.print(category);
    Serial.print(" : ");
    Serial.println(value);

    if (category == "T1H") {
      temp = value;
    }

    if (category == "REH") {
      humidity = value;
    }
  }

  Serial.println("======================");

  Serial.println("====== 현재 날씨 ======");

  Serial.print("기온 : ");
  Serial.print(temp);
  Serial.println(" °C");

  Serial.print("습도 : ");
  Serial.print(humidity);
  Serial.println(" %");

  Serial.println("=======================");
}