#include <WiFi.h>
#include <esp_now.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define LED 2

/* 수신 데이터 구조체 */
typedef struct {
  float temp;
  float humidity;
} WeatherData;

WeatherData incoming;

/* 데이터 수신 콜백 */
void OnDataRecv(const esp_now_recv_info *info, const uint8_t *data, int len) {

  memcpy(&incoming, data, sizeof(incoming));

  Serial.println("===== 날씨 수신 =====");

  Serial.print("기온 : ");
  Serial.print(incoming.temp);
  Serial.println(" °C");

  Serial.print("습도 : ");
  Serial.print(incoming.humidity);
  Serial.println(" %");

  Serial.println("====================");

  /* OLED 화면 초기화 */
  display.clearDisplay();

  display.setTextSize(2);
  display.setTextColor(WHITE);

  display.setCursor(0,0);
  display.print("Temp:");
  display.print(incoming.temp);
  display.print("C");

  display.setCursor(0,30);
  display.print("Hum:");
  display.print(incoming.humidity);
  display.print("%");

  display.display();

  /* 습도 기준 LED 제어 */
  if(incoming.humidity > 70){
    digitalWrite(LED, HIGH);
  }else{
    digitalWrite(LED, LOW);
  }
}

void setup() {

  Serial.begin(115200);

  pinMode(LED, OUTPUT);

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

  /* ESP-NOW 시작 */
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW 초기화 실패");
    return;
  }
    Serial.println("ESP-NOW 초기화 성공");
  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {

}