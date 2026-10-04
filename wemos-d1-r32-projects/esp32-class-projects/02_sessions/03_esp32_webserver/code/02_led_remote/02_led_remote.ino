/*
  3회차 · 따라하기 ③ — 내 HTML 심기 + LED 엔드포인트
  지난주(2회차) 만든 리모컨.html을 PAGE[] 안에 붙여넣고,
  onclick을 fetch('/led/on')으로 바꾸면 — 진짜 LED가 켜져요!

  코드 4구역: ① 맨 위 선언 ② setup() 준비·등록 ③ loop() 한 줄 ④ HTML은 PAGE 안에서만
*/

// ── 구역 1 · 맨 위: include와 전역 선언 ──────────────────
#include <WiFi.h>
#include <WebServer.h>

#define LED 2  // 내장 LED

WebServer server(80);

// HTML 그릇 — rawliteral 사이에 지난주 HTML을 통째로!
const char PAGE[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <title>내 리모컨</title>
  <style>
    body { font-family: sans-serif; text-align: center; background: #EDF3FE; }
    h1 { color: #2E6BE6; }
    button {
      font-size: 24px; padding: 20px 40px; margin: 10px;
      border-radius: 12px; border: none; background: #2E6BE6; color: white;
    }
  </style>
</head>
<body>
  <h1>Team1 리모컨</h1>
  <p id="status">대기 중</p>
  <button onclick="ledOn()">LED 켜기</button>
  <button onclick="ledOff()">LED 끄기</button>
  <script>
    function ledOn() {
      fetch('/led/on');   // ← 지난주의 '빈칸'이 채워졌어요!
      document.getElementById("status").innerText = "LED 켜짐!";
    }
    function ledOff() {
      fetch('/led/off');
      document.getElementById("status").innerText = "LED 꺼짐";
    }
  </script>
</body>
</html>
)rawliteral";

// ── 구역 2 · setup(): 준비 → 등록 → 시작 ────────────────
void setup() {
  pinMode(LED, OUTPUT);

  WiFi.softAP("Team1_IoT", "12345678");

  server.on("/", []() {
    server.send(200, "text/html", PAGE);
  });
  server.on("/led/on", []() {
    digitalWrite(LED, HIGH);              // 요청이 오면 진짜 LED가 켜져요
    server.send(200, "text/plain", "OK");
  });
  server.on("/led/off", []() {
    digitalWrite(LED, LOW);
    server.send(200, "text/plain", "OK");
  });

  server.begin();
}

// ── 구역 3 · loop(): 단 한 줄 (delay 넣지 말 것!) ────────
void loop() {
  server.handleClient();
}
