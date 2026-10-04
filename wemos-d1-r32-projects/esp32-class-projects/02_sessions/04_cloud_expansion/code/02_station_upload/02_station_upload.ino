/*
  4회차 · 따라하기 ④⑤ — Station 모드 + 클라우드 업로드 (센서 4종)
  AP 모드(WiFi 만들기) → Station 모드(WiFi 접속하기)로 전환.
  10초마다 온도·습도·조도·소음을 구글 시트에 자동 업로드!

  ★ 바꿀 곳 3군데: WiFi 이름·비번, 웹앱 URL
  ⚠ ESP32는 2.4GHz WiFi만 접속 가능 / setFollowRedirects 빼먹으면 실패!
  배선: DHT → 디지털 핀, 빛·소리 → A2~A5 핀 (★핀맵 문서에서 보드 라벨↔GPIO 확인)
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>

#define DHTPIN 19  // ★핀맵 문서 확인 (보드 라벨 ↔ GPIO)
#define LIGHT  34  // 조도 (A3 핀)
#define SOUND  35  // 소음 (A2 핀)

DHT dht(DHTPIN, DHT11);

// ★ 우리 팀 것으로 교체 (URL 끝이 /exec 인지 확인!)
const char* WIFI_SSID = "WiFi이름";
const char* WIFI_PASS = "비밀번호";
const char* URL = "https://script.google.com/macros/s/XXXX/exec";

void setup() {
  Serial.begin(115200);
  dht.begin();

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("WiFi 연결 중");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);              // 연결될 때까지 대기
    Serial.print(".");
  }
  Serial.println(" 연결 완료!");
}

void loop() {
  // 센서 읽기 — '읽기 한 줄 + 주소 한 조각'이 센서 추가 공식!
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
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS); // 구글의 '재안내' 따라가기 — 필수!
  int code = http.GET();   // 요청 발사!
  Serial.print("업로드 결과: ");
  Serial.println(code);    // 200이면 정상
  http.end();

  delay(10000);  // 10초마다 — 시트에 한 줄씩 자동으로 쌓여요
}
