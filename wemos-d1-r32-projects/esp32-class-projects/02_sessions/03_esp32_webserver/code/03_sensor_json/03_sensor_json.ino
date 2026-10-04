/*
  3회차 · 따라하기 ④⑤ — /sensor 엔드포인트(JSON) + 웹 자동 갱신(폴링)
  폰 브라우저에서 192.168.4.1/sensor 를 직접 열어 재료(JSON)부터 검사!
  {"temp":25.3,"humi":48.0} 모양 그대로여야 다음 단계 JS가 읽을 수 있어요.

  배선: DHT 센서 S → D12 핀(GPIO19), G→GND, V→3V3 (★다른 핀이면 핀맵 문서 확인)
*/

#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>

#define DHTPIN 19  // D12 핀 (★꽂은 핀의 GPIO로)
DHT dht(DHTPIN, DHT11);

WebServer server(80);

const char PAGE[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <title>미니 기상 스테이션</title>
  <style>
    body { font-family: sans-serif; text-align: center; background: #EDF3FE; }
    h1 { color: #2E6BE6; }
    p { font-size: 28px; }
  </style>
</head>
<body>
  <h1>미니 기상 스테이션</h1>
  <p>온도: <span id="t">--</span> ℃</p>
  <p>습도: <span id="h">--</span> %</p>
  <script>
    async function update() {
      const res = await fetch("/sensor");  // 폴링: 주기적으로 '지금 어때?'
      const d = await res.json();
      document.getElementById("t").innerText = d.temp;
      document.getElementById("h").innerText = d.humi;
    }
    setInterval(update, 2000);  // 2초마다 반복 실행
  </script>
</body>
</html>
)rawliteral";

void setup() {
  dht.begin();
  WiFi.softAP("Team1_IoT", "12345678");

  server.on("/", []() {
    server.send(200, "text/html", PAGE);
  });
  server.on("/sensor", []() {
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    String json = "{\"temp\":" + String(t) + ",\"humi\":" + String(h) + "}";
    server.send(200, "application/json", json);
  });

  server.begin();
}

void loop() {
  server.handleClient();
}
