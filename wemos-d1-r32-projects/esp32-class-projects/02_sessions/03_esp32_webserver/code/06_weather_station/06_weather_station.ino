/*
  3회차 · 미션 ① — 미니 기상 스테이션 (심화 포함 정답)
  폰 화면에 온도·습도·조도가 2초마다 자동 갱신되고,
  온도가 28℃를 넘으면 화면 배경이 빨갛게 변해요!

  배선: 온습도(DHT) 센서 S → D11 핀(GPIO23) — 보드 라벨 'D11', 코드에는 23!
       빛센서 S → A3 핀(GPIO34)
       (모든 센서 G→GND, V→3V3)

  사용법: 업로드 → 폰 WiFi를 Team1_IoT(비번 12345678)에 연결 → 브라우저에서 192.168.4.1
*/

#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>

#define DHTPIN 23  // 온습도 (D11 핀)
#define LIGHT  34  // 빛센서 (A3 핀)

DHT dht(DHTPIN, DHT11);
WebServer server(80);

const char PAGE[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
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
  <p>조도: <span id="l">--</span></p>
  <script>
    async function update() {
      const res = await fetch("/sensor");  // 폴링: 2초마다 '지금 어때?'
      const d = await res.json();
      document.getElementById("t").innerText = d.temp;
      document.getElementById("h").innerText = d.humi;
      document.getElementById("l").innerText = d.light;

      // 심화: 온도 28℃ 초과면 배경 빨갛게, 아니면 원래 색으로
      if (d.temp > 28) {
        document.body.style.background = "#FBECEC";
      } else {
        document.body.style.background = "#EDF3FE";
      }
    }
    setInterval(update, 2000);
    update();
  </script>
</body>
</html>
)rawliteral";

void setup() {
  dht.begin();
  WiFi.softAP("Team1_IoT", "12345678");   // ★ 팀 이름으로!

  server.on("/", []() {
    server.send(200, "text/html", PAGE);
  });
  server.on("/sensor", []() {
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    int   l = analogRead(LIGHT);          // 0(어두움) ~ 4095(밝음)
    String json = "{\"temp\":"  + String(t)
                + ",\"humi\":"  + String(h)
                + ",\"light\":" + String(l) + "}";
    server.send(200, "application/json", json);
  });

  server.begin();
}

void loop() {
  server.handleClient();
}
