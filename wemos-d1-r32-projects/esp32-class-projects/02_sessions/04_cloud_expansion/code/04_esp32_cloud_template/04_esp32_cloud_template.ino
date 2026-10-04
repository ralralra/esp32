/*
  4회차 · 뼈대 템플릿 — ESP32 클라우드 완성판 (팀 프로젝트의 출발점!)
  ① 10초마다 센서 4종을 시트에 업로드
  ② 같은 주기로 명령 셀(설정!A1)을 폴링 → ON/OFF로 LED 제어

  시트 '설정' 탭 A1에 ON 을 쓰면 → 최대 10초 뒤 교실 LED 점등!
  (즉시가 아닌 이유 = 폴링 주기만큼 기다림. 빠르게 하려면 주기를 3~5초로 — 대신 요청 횟수 증가)

  ★ 바꿀 곳: WiFi 이름·비번, 웹앱 URL (끝이 /exec!)
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>

#define LED    2
#define DHTPIN 19  // ★핀맵 문서 확인 (보드 라벨 ↔ GPIO)
#define LIGHT  34  // A3 핀 (A2~A5 중 하나)
#define SOUND  35  // A2 핀 (A2~A5 중 하나)

DHT dht(DHTPIN, DHT11);

const char* WIFI_SSID = "WiFi이름";
const char* WIFI_PASS = "비밀번호";
const char* URL = "https://script.google.com/macros/s/XXXX/exec";

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  dht.begin();

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" WiFi 연결 완료!");
}

void loop() {
  // ── ① 센서 업로드 ──────────────────────────────
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  int   l = analogRead(LIGHT);
  int   s = analogRead(SOUND);

  HTTPClient http;
  String u = String(URL)
    + "?temp="  + String(t)
    + "&humi="  + String(h)
    + "&light=" + String(l)
    + "&sound=" + String(s);
  http.begin(u);
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  http.GET();
  http.end();

  // ── ② 명령 폴링 ───────────────────────────────
  HTTPClient http2;
  http2.begin(String(URL) + "?mode=cmd");
  http2.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  http2.GET();
  String cmd = http2.getString();
  http2.end();

  Serial.print("명령: ");
  Serial.println(cmd);
  if (cmd == "ON")                 // ★ 대소문자까지 시트 값과 일치해야!
    digitalWrite(LED, HIGH);
  else
    digitalWrite(LED, LOW);

  delay(10000);  // 10초마다
}
