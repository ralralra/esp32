/*
  IoT 무드등 V1 · 2단계 — 출력 테스트 (네오픽셀 스트립 16구)

  보드: Wemos D1 R32 + 센서쉴드 V5  (배선: ../../wiring.md)
    네오픽셀 스트립 → 실드 6번 (DIN→S, 5V→V, GND→G) = GPIO27
    DIN 선 중간에 330Ω, 5V–GND에 1000µF · SEL 점퍼 빼고 EXT PWR에 5V

  설정은 동작이 확인된 라이브러리 샘플(strandtest)과 같다:
    Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);   // WS2812B RGB
    strip.setBrightness(50);

  하는 일
    켜자마자 샘플과 같은 무지개가 스트립을 따라 흐른다 (strip.rainbow 사용).
    시리얼(115200) 명령으로 색·밝기를 바꿔 본다.
      c warm | white | red | orange | yellow | green | blue | purple | pink | FF8800(색 코드)
      rainbow      무지개 흐르기 (샘플의 rainbow)
      chase        샘플의 theaterChaseRainbow 한 번 (약 15초, 끝나면 이전 상태로)
      b 0~255      밝기 (setBrightness 값 그대로 · 샘플 기본 50)
      test         빨강 → 초록 → 파랑 순서 확인 (이름과 색이 다르면 NEO_GRB → NEO_RGB)
      off / on     끄기 / 켜기
*/

#include <Adafruit_NeoPixel.h>

#define LED_PIN    27      // 실드 6번
#define LED_COUNT  16      // 스트립을 자른 LED 개수

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

uint8_t  brightness = 50;                 // 샘플 기본값 (max 255)
uint8_t  colR = 255, colG = 170, colB = 80;   // 웜화이트 (RGB 혼합)
String   colName = "warm";
bool     rainbowOn = true;                // 켜자마자 무지개
bool     lampOn = true;
long     firstPixelHue = 0;
unsigned long lastRainbowMs = 0;
String   inputLine;

void fill(uint8_t r, uint8_t g, uint8_t b) {
  for (int i = 0; i < strip.numPixels(); i++) strip.setPixelColor(i, strip.Color(r, g, b));
  strip.show();
}

void showStrip() {
  strip.setBrightness(lampOn ? brightness : 0);
  if (rainbowOn) strip.rainbow(firstPixelHue);   // 샘플과 같은 무지개
  else           fill(colR, colG, colB);
  strip.show();
}

bool setColor(String v) {
  struct Preset { const char* n; uint8_t r, g, b; };
  static const Preset presets[] = {
    {"warm", 255, 170, 80}, {"white", 255, 255, 255}, {"red", 255, 0, 0},
    {"orange", 255, 90, 0}, {"yellow", 255, 200, 0},  {"green", 0, 255, 0},
    {"blue", 0, 60, 255},   {"purple", 150, 0, 255},  {"pink", 255, 60, 120},
  };
  v.trim();
  v.toLowerCase();
  for (const Preset& p : presets)
    if (v == p.n) { colR = p.r; colG = p.g; colB = p.b; colName = p.n; rainbowOn = false; return true; }
  if (v.length() == 6) {                           // 6자리 색 코드 (# 없이)
    for (unsigned i = 0; i < 6; i++) if (!isxdigit((unsigned char)v[i])) return false;
    long rgb = strtol(v.c_str(), nullptr, 16);
    colR = (rgb >> 16) & 0xFF; colG = (rgb >> 8) & 0xFF; colB = rgb & 0xFF;
    v.toUpperCase(); colName = v; rainbowOn = false;
    return true;
  }
  return false;
}

// 샘플의 theaterChaseRainbow 그대로 (약 15초 동안 멈춰서 돌아감)
void theaterChaseRainbow(int wait) {
  int firstHue = 0;
  for (int a = 0; a < 30; a++) {
    for (int b = 0; b < 3; b++) {
      strip.clear();
      for (int c = b; c < strip.numPixels(); c += 3) {
        int hue = firstHue + c * 65536L / strip.numPixels();
        strip.setPixelColor(c, strip.gamma32(strip.ColorHSV(hue)));
      }
      strip.show();
      delay(wait);
      firstHue += 65536 / 90;
    }
  }
}

void colorTest() {                                 // 색 순서 확인
  strip.setBrightness(brightness);
  Serial.println("빨강"); fill(255, 0, 0); delay(700);
  Serial.println("초록"); fill(0, 255, 0); delay(700);
  Serial.println("파랑"); fill(0, 0, 255); delay(700);
  Serial.println("→ 이름과 색이 다르면 NEO_GRB를 NEO_RGB로 바꾸세요");
  showStrip();
}

void handle(String line) {
  line.trim();
  String lower = line; lower.toLowerCase();
  if (lower == "on")            { lampOn = true;  Serial.println("켜기"); }
  else if (lower == "off")      { lampOn = false; Serial.println("끄기"); }
  else if (lower == "rainbow")  { rainbowOn = true; lampOn = true; colName = "rainbow"; Serial.println("무지개"); }
  else if (lower == "chase")    { Serial.println("theaterChaseRainbow — 약 15초"); theaterChaseRainbow(50); }
  else if (lower == "test")     { colorTest(); return; }
  else if (lower.startsWith("c ")) {
    if (!setColor(line.substring(2))) { Serial.println("색: warm white red orange yellow green blue purple pink 또는 FF8800"); return; }
    lampOn = true; Serial.println("색 → " + colName);
  }
  else if (lower.startsWith("b ")) { brightness = constrain(lower.substring(2).toInt(), 0, 255); Serial.printf("밝기 → %d/255\n", brightness); }
  else { if (line.length()) Serial.println("명령: c <색> / rainbow / chase / b <0~255> / test / on / off"); return; }
  showStrip();
}

void setup() {
  Serial.begin(115200);
  strip.begin();                 // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();                  // Turn OFF all pixels ASAP
  strip.setBrightness(brightness);
  Serial.printf("네오픽셀 테스트 — %d구 RGB (NEO_GRB), 밝기 %d/255 · 무지개 흐르는 중\n", LED_COUNT, brightness);
}

void loop() {
  while (Serial.available()) {
    char ch = Serial.read();
    if (ch == '\n' || ch == '\r') { handle(inputLine); inputLine = ""; }
    else inputLine += ch;
  }
  if (rainbowOn && lampOn && millis() - lastRainbowMs >= 10) {   // 샘플 rainbow(10): 10ms마다 hue += 256
    lastRainbowMs = millis();
    firstPixelHue += 256;
    if (firstPixelHue >= 5 * 65536L) firstPixelHue = 0;
    showStrip();
  }
}
