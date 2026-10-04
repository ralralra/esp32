/*
  4단계 — 폰으로 수분값 보기 + 수동 급수 (웹 대시보드)
  ----------------------------------------------------
  목표: 앞서 배운 웹서버를 재활용해서,
        폰으로 접속하면 현재 수분값과 상태가 보이고(자동 새로고침),
        "지금 물주기" 버튼으로 수동 급수도 할 수 있다.

  자동 급수(3단계)도 그대로 동작 + 화면으로 모니터링.

  배선: 센서 → GPIO34, 릴레이 IN → GPIO26
  보드: ESP32 Dev Module
*/

#include <WiFi.h>
#include <WebServer.h>
#include "soc/soc.h"            // Brownout 안전장치
#include "soc/rtc_cntl_reg.h"

#define SOIL_PIN  34
#define RELAY_PIN 26

int DRY_THRESHOLD = 3000;   // ★ 내 센서 기준값
int WATER_MS = 2000;

const int PUMP_ON  = LOW;    // 릴레이 active-LOW 가정
const int PUMP_OFF = HIGH;

const char* AP_SSID = "ESP32-Plant";
const char* AP_PASS = "plant1234";

WebServer server(80);

unsigned long lastAuto = 0;   // 자동 급수 주기 체크용

// 펌프를 켰다가 일정 시간 후 끄기
void waterOnce() {
  digitalWrite(RELAY_PIN, PUMP_ON);
  delay(WATER_MS);
  digitalWrite(RELAY_PIN, PUMP_OFF);
}

// 메인 페이지 — 현재 수분값 표시 + 2초마다 자동 새로고침
void handleRoot() {
  int raw = analogRead(SOIL_PIN);
  bool dry = raw > DRY_THRESHOLD;
  String state = dry ? "🏜 말랐어요" : "💧 촉촉해요";
  String color = dry ? "#d97706" : "#2563eb";

  String html = "<!doctype html><html lang='ko'><head><meta charset='utf-8'>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<meta http-equiv='refresh' content='2'>";   // 2초마다 자동 새로고침
  html += "<title>스마트 화분</title><style>";
  html += "body{font-family:sans-serif;text-align:center;padding:2rem;max-width:400px;margin:0 auto}";
  html += "h1{color:#16a34a}.val{font-size:3rem;font-weight:bold;color:" + color + "}";
  html += ".state{font-size:1.5rem;margin:1rem;color:" + color + "}";
  html += "button{font-size:1.2rem;padding:.7rem 2rem;background:#16a34a;color:#fff;border:none;border-radius:8px}";
  html += "</style></head><body>";
  html += "<h1>🪴 스마트 화분</h1>";
  html += "<div class='val'>" + String(raw) + "</div>";
  html += "<div class='state'>" + state + "</div>";
  html += "<p>마름 기준: " + String(DRY_THRESHOLD) + "</p>";
  html += "<form action='/water' method='get'><button>💦 지금 물주기</button></form>";
  html += "<p style='color:#888;font-size:.9rem'>2초마다 자동 새로고침</p>";
  html += "</body></html>";
  server.send(200, "text/html; charset=utf-8", html);
}

// 수동 급수 버튼
void handleWater() {
  Serial.println("수동 급수!");
  waterOnce();
  server.sendHeader("Location", "/");   // 급수 후 메인으로 돌아가기
  server.send(303);
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);   // Brownout 안전장치
  Serial.begin(115200);
  delay(300);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, PUMP_OFF);

  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.println("\n=== 스마트 화분 대시보드 ===");
  Serial.print("접속: http://");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/water", handleWater);
  server.begin();
  Serial.println("서버 시작됨!");
}

void loop() {
  server.handleClient();

  // 자동 급수: 3초마다 한 번 측정해서 마르면 급수 (웹 응답 막지 않게 간격 둠)
  if (millis() - lastAuto > 3000) {
    lastAuto = millis();
    int raw = analogRead(SOIL_PIN);
    if (raw > DRY_THRESHOLD) {
      Serial.printf("자동 급수 (값 %d)\n", raw);
      waterOnce();
    }
  }
}
