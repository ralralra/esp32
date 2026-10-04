/*
  3회차 · 따라하기 ① — AP 모드 + 첫 웹서버
  업로드하면 ESP32가 'Team1_IoT'라는 WiFi를 만들어요.
  폰 → WiFi 연결 → 브라우저에서 192.168.4.1 접속!

  ⚠ WiFi 이름은 팀 번호로 바꾸고, 비밀번호는 8자 이상!
*/

#include <WiFi.h>
#include <WebServer.h>

WebServer server(80);  // 80 = 웹 기본 문

void setup() {
  Serial.begin(115200);

  WiFi.softAP("Team1_IoT", "12345678");
  // (WiFi 이름, 비밀번호 8자 이상)
  Serial.print("AP address: ");
  Serial.println(WiFi.softAPIP());  // 192.168.4.1 이 찍혀요

  server.on("/", []() {  // 대문(/) 요청이 오면
    server.send(200, "text/html", "<h1>Hello ESP32!</h1>");
  });
  server.begin();        // 영업 시작!
}

void loop() {
  server.handleClient();  // 손님 왔나 계속 확인
}
