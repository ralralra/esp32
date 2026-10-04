/*
  바깥 날씨 API(Open-Meteo) — 교실 안 vs 바깥 비교 (4회차 심화)
  ESP32가 인터넷의 무료 날씨 API에서 바깥 온도·습도를 받아와
  교실 안(DHT) 값과 나란히 시리얼 모니터에 표시해요.
  (7세그에는 바깥 온도가 표시됩니다)

  시리얼 표시 예:  In: 25.3C 48%  /  Out: 18.7C 62% (code 200)

  ● Open-Meteo : 회원가입도, API 키도 필요 없는 무료 날씨 API
  ● API란? 서비스가 열어 둔 '데이터 창구' — 주소로 요청하면 JSON으로 응답.
    우리 Apps Script 웹앱 URL도 똑같은 원리예요! (우리도 API를 만든 것)

  배선: 온습도(DHT) 센서 S → D11 핀(GPIO23) / 7세그 DIO → D6(GPIO27), CLK → D7(GPIO14)
        (점퍼선으로 직접 연결, 센서·7세그 V는 3V3)
  ★ 바꿀 곳: WiFi 이름·비번, 그리고 우리 학교의 위도·경도
    (지도 앱에서 학교를 길게 누르면 숫자 두 개가 나와요)
*/

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <TM1637Display.h>
#include <DHT.h>

#define DHTPIN 23  // 온습도 (D11 핀)
#define DIO    27  // D6 핀
#define CLK    14  // D7 핀

// ★ 우리 것으로 교체
const char* WIFI_SSID = "WiFi이름";      // 2.4GHz만!
const char* WIFI_PASS = "비밀번호";
const char* LAT = "37.57";              // 위도 (예: 서울)
const char* LON = "126.98";             // 경도

DHT dht(DHTPIN, DHT11);
TM1637Display display(CLK, DIO);

// 응답 문자열에서 "이름": 뒤의 숫자만 잘라내는 간단 파서
float pickNumber(String body, String key) {
  int i = body.indexOf("\"" + key + "\":");
  if (i < 0) return -999;                        // 못 찾으면 -999
  i += key.length() + 3;                         // "key": 다음 위치로
  int j = i;
  while (j < body.length() &&
         (isDigit(body[j]) || body[j] == '.' || body[j] == '-')) j++;
  return body.substring(i, j).toFloat();
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  display.setBrightness(7);

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" WiFi OK!");
}

void loop() {
  // ── ① 교실 안 : DHT 읽기 ─────────────────────────
  float inT = dht.readTemperature();
  float inH = dht.readHumidity();

  // ── ② 바깥 : Open-Meteo API 요청 ─────────────────
  // 주소 조립 — 4회차의 "?이름=값&이름=값" 원리 그대로!
  String url = String("https://api.open-meteo.com/v1/forecast")
    + "?latitude=" + LAT + "&longitude=" + LON
    + "&current=temperature_2m,relative_humidity_2m";

  WiFiClientSecure client;
  client.setInsecure();               // 학습용: 인증서 검증 생략
  HTTPClient http;
  http.begin(client, url);
  int code = http.GET();              // 요청 발사!

  float outT = -999, outH = -999;
  if (code == 200) {                  // 200 = 정상 응답
    String body = http.getString();   // JSON 문자열 통째로
    outT = pickNumber(body, "temperature_2m");
    outH = pickNumber(body, "relative_humidity_2m");
  }
  http.end();

  // ── ③ 안팎 비교 표시 ─────────────────────────────
  Serial.printf("In: %.1fC %.0f%%  /  Out: %.1fC %.0f%% (code %d)\n",
                inT, inH, outT, outH, code);
  if (outT > -100) {
    display.showNumberDec((int)outT);   // 7세그엔 바깥 온도(정수)
  } else {
    Serial.println("Out: API fail — WiFi·위도경도 확인!");
  }

  delay(600000);  // 10분마다 갱신 (날씨는 자주 안 변해요 — 예의 있는 요청!)
}
