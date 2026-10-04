/*
  교실환경 모니터링 — 우리 교실 확정판 (점퍼선 직결 배선 기준)
  빛·소리·온습도를 10초마다 구글 시트에 올리고,
  시트 '설정' 탭 A1의 ON/OFF로 LED를 원격 제어합니다.
  현재 값은 시리얼 모니터(115200)와 구글 시트에서 확인해요.

  ── 배선 (점퍼선으로 보드 핀에 직접: G→GND, V→3V3, S→신호 핀) ──
  빛 센서 S    → A3 핀(GPIO34)   → LIGHT 34
  소리 센서 S  → A2 핀(GPIO35)   → SOUND 35
  온습도 센서 S → D11 핀(GPIO23) → DHTPIN 23
              ⚠ 보드 라벨은 'D11'(우노 라벨), 코드에는 GPIO 번호 23을 씁니다!
  LED 모듈 S   → D9 핀(GPIO13)   → LED 13
              (LED 모듈이 없으면 내장 LED 사용: 13 → 2 로 변경)
  ⚠ 아날로그 센서(빛·소리) V는 반드시 3V3 — 5V에 연결하면 ESP32 입력이 상할 수 있어요.

  ── 준비 (이미 하셨다면 통과!) ───────────────────────────
  시트: '시트1' 탭 1행 = 시각·온도·습도·조도·소음 / '설정' 탭 A1 = ON
  Apps Script: 03_apps_script_full.gs 배포(모든 사용자, /exec)

  ★ 바꿀 곳 3줄: WIFI_SSID · WIFI_PASS · URL
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>

#define LIGHT  34  // 빛   (A3 핀)
#define SOUND  35  // 소리 (A2 핀)
#define DHTPIN 23  // 온습도 (보드 라벨 D11 — '11' 아님!)
#define LED    13  // LED 모듈 (보드 라벨 D9)

const char* WIFI_SSID = "WiFi이름";   // ★ 2.4GHz만!
const char* WIFI_PASS = "비밀번호";   // ★
const char* URL = "https://script.google.com/macros/s/XXXX/exec";  // ★ /exec 확인!

DHT dht(DHTPIN, DHT11);

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  dht.begin();

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" WiFi OK!");
}

void loop() {
  // ── ① 센서 읽기 ──────────────────────────────
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  int   l = analogRead(LIGHT);   // 0~4095
  int   s = analogRead(SOUND);   // 0~4095

  // ── ② 구글 시트에 업로드 ─────────────────────
  HTTPClient http;
  String u = String(URL)
    + "?temp="  + String(t)
    + "&humi="  + String(h)
    + "&light=" + String(l)
    + "&sound=" + String(s);
  http.begin(u);
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);  // 필수!
  int code = http.GET();
  http.end();
  Serial.printf("upload %d | T%.1f H%.0f L%d S%d\n", code, t, h, l, s);

  // ── ③ 명령 폴링: 설정!A1의 ON/OFF → LED ──────
  HTTPClient http2;
  http2.begin(String(URL) + "?mode=cmd");
  http2.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  http2.GET();
  String cmd = http2.getString();
  http2.end();
  cmd.trim();                       // 앞뒤 공백 제거
  Serial.println("cmd: " + cmd);
  if (cmd == "ON")  digitalWrite(LED, HIGH);
  if (cmd == "OFF") digitalWrite(LED, LOW);

  delay(10000);  // 10초마다 반복
}
