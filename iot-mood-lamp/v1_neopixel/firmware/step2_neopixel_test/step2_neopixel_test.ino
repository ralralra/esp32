/*
  IoT 무드등 V1 · 2단계 — 출력 테스트 (네오픽셀 스트립 16구)

  보드: Wemos D1 R32 + 센서쉴드 V5  (배선: ../../wiring.md)
    네오픽셀 스트립 → 실드 6번 (DIN→S, 5V→V, GND→G) = GPIO27
    DIN 선 중간에 330Ω, 링 5V–GND에 1000µF
    SEL 점퍼 빼고 EXT PWR에 5V (2포트 충전기) — 업로드할 때 PC USB만 꽂았다면 밝기를 낮게

  하는 일
    켜자마자 빨강 → 초록 → 파랑 → 흰색(W) 순서로 한 바퀴 보여 주고, 웜화이트로 켜 둔다.
    색 순서가 다르게 나오면 LED_RGBW 설정이 스트립과 다른 것.
    (초록·빨강·파랑·꺼짐이 한 칸씩 번갈아 켜지면 RGB 스트립에 RGBW 데이터를 보낸 것 → LED_RGBW = false)

  시리얼(115200) 명령
    c warm | white | red | orange | yellow | green | blue | purple | pink | rainbow | FF8800(색 코드)
    b 0~100     밝기 %
    on / off    켜기 / 끄기
    test        색 순서 한 바퀴 다시
*/

#include <Adafruit_NeoPixel.h>

const int  LED_PIN        = 27;    // 실드 6번
const int  LED_COUNT      = 16;    // 스트립을 자른 LED 개수 (이번 부품: 16구)
const bool LED_RGBW       = false; // 이번 부품은 RGB(WS2812B) 스트립 = false · 흰 칩이 따로 있는 RGBW(SK6812)면 true
const int  MAX_BRIGHTNESS = 128;   // 0~255 — 어댑터 보호 상한 (16구 RGB · 상한 128에서 약 0.5A)

Adafruit_NeoPixel ring(LED_COUNT, LED_PIN, (LED_RGBW ? NEO_GRBW : NEO_GRB) + NEO_KHZ800);

uint8_t colR = 0, colG = 0, colB = 0, colW = 255;   // 기본: 웜화이트(W 채널)
String  colName = "warm";
bool    rainbow = false;
bool    lampOn = true;
int     brightPct = 50;
uint16_t rainbowHue = 0;
unsigned long lastRainbowMs = 0;
String inputLine;

void fill(uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
  for (int i = 0; i < LED_COUNT; i++) {
    if (LED_RGBW) ring.setPixelColor(i, r, g, b, w);
    else          ring.setPixelColor(i, r, g, b);
  }
  ring.show();
}

void showRing() {
  if (!lampOn) { ring.clear(); ring.show(); return; }
  ring.setBrightness((long)MAX_BRIGHTNESS * brightPct / 100);
  if (rainbow) {
    for (int i = 0; i < LED_COUNT; i++)
      ring.setPixelColor(i, ring.gamma32(ring.ColorHSV(rainbowHue + i * 65536L / LED_COUNT)));
    ring.show();
  } else fill(colR, colG, colB, colW);
}

bool setColor(String v) {
  struct Preset { const char* n; uint8_t r, g, b, w; };
  static const Preset presets[] = {
    {"warm", 0, 0, 0, 255},    {"white", 120, 120, 120, 255}, {"red", 255, 0, 0, 0},
    {"orange", 255, 90, 0, 0}, {"yellow", 255, 200, 0, 0},    {"green", 0, 255, 0, 0},
    {"blue", 0, 60, 255, 0},   {"purple", 150, 0, 255, 0},    {"pink", 255, 60, 120, 0},
  };
  v.trim();
  v.toLowerCase();
  if (v == "rainbow") { rainbow = true; colName = "rainbow"; return true; }
  for (const Preset& p : presets) {
    if (v == p.n) {
      colR = p.r; colG = p.g; colB = p.b; colW = p.w;
      if (!LED_RGBW && colW) {                     // RGB 스트립에는 흰 칩이 없어서 섞어서 흉내
        bool white = (v == "white");
        colR = 255; colG = white ? 255 : 170; colB = white ? 255 : 80; colW = 0;
      }
      rainbow = false; colName = p.n;
      return true;
    }
  }
  if (v.length() == 6) {                           // 6자리 색 코드 (# 없이)
    for (unsigned i = 0; i < 6; i++) if (!isxdigit((unsigned char)v[i])) return false;
    long rgb = strtol(v.c_str(), nullptr, 16);
    colR = (rgb >> 16) & 0xFF; colG = (rgb >> 8) & 0xFF; colB = rgb & 0xFF; colW = 0;
    rainbow = false; v.toUpperCase(); colName = v;
    return true;
  }
  return false;
}

void colorTest() {                                 // 색 순서 확인 — 이름과 색이 맞아야 함
  ring.setBrightness(MAX_BRIGHTNESS / 2);
  Serial.println("빨강"); fill(255, 0, 0, 0); delay(700);
  Serial.println("초록"); fill(0, 255, 0, 0); delay(700);
  Serial.println("파랑"); fill(0, 0, 255, 0); delay(700);
  if (LED_RGBW) { Serial.println("흰색(W 채널)"); fill(0, 0, 0, 255); delay(700); }
  Serial.println("→ 이름과 색이 다르거나 LED마다 색이 다르면 LED_RGBW 설정을 스트립에 맞추세요");
  showRing();
}

void handle(String line) {
  line.trim();
  String lower = line; lower.toLowerCase();
  if (lower == "on")        { lampOn = true;  Serial.println("켜기"); }
  else if (lower == "off")  { lampOn = false; Serial.println("끄기"); }
  else if (lower == "test") { colorTest(); return; }
  else if (lower.startsWith("c ")) {
    if (!setColor(line.substring(2))) { Serial.println("색: warm white red orange yellow green blue purple pink rainbow 또는 FF8800"); return; }
    lampOn = true; Serial.println("색 → " + colName);
  }
  else if (lower.startsWith("b ")) { brightPct = constrain(lower.substring(2).toInt(), 0, 100); Serial.printf("밝기 → %d%%\n", brightPct); }
  else { if (line.length()) Serial.println("명령: c <색> / b <0~100> / on / off / test"); return; }
  showRing();
}

void setup() {
  Serial.begin(115200);
  ring.begin();
  ring.clear();
  ring.show();                                     // 부팅 직후 모두 끄기
  setColor("warm");                                // 기본 색을 스트립 종류(RGB/RGBW)에 맞춰 준비
  Serial.printf("네오픽셀 테스트 — %d구 %s, 밝기 상한 %d/255\n", LED_COUNT, LED_RGBW ? "RGBW" : "RGB", MAX_BRIGHTNESS);
  colorTest();
}

void loop() {
  while (Serial.available()) {
    char ch = Serial.read();
    if (ch == '\n' || ch == '\r') { handle(inputLine); inputLine = ""; }
    else inputLine += ch;
  }
  if (rainbow && lampOn && millis() - lastRainbowMs >= 30) {   // 색상환이 천천히 한 바퀴 (약 10초)
    lastRainbowMs = millis();
    rainbowHue += 200;
    showRing();
  }
}
