/*
  IoT 무드등 V1 · 3단계 — 센서 받고 출력하기 (조도·PIR → 네오픽셀, 와이파이 없이)

  보드: Wemos D1 R32 + 센서쉴드 V5  (배선: ../../wiring.md)
    네오픽셀 링 → 실드 6번 (DIN→S, 5V→V, GND→G) = GPIO27   (DIN에 330Ω, 링 5V–GND에 1000µF)
    PIR          → 실드 3번                        = GPIO25
    조도센서     → 실드 A3 (VCC는 Bluetooth 헤더 3V3) = GPIO34
  라이브러리: Adafruit NeoPixel

  규칙 (자동 모드)
    켜짐: 주변 밝기가 DARK_LEVEL 보다 어두움 + (USE_PIR이면) 움직임 감지  → 마지막 색으로 서서히 켜짐
    꺼짐: 기준보다 밝아짐  또는  (USE_PIR이면) 움직임 없이 AUTO_OFF_MS 지남 → 서서히 꺼짐
    1단계에서 정한 DARK_LEVEL · DARK_IS_HIGH 를 아래에 똑같이 적는다.

  시리얼(115200) 명령
    on / off / auto     수동 켜기·끄기 / 자동 모드로
    s                   상태 한 줄
    sensor              센서 값 1초마다 표시 켜기/끄기
    c <색> / b <0~100>  색 (warm white red orange yellow green blue purple pink rainbow FF8800) / 밝기 %
*/

#include <Adafruit_NeoPixel.h>

// ── 핀 ───────────────────────────────────────────
const int LED_PIN   = 27;   // 실드 6번
const int PIR_PIN   = 25;   // 실드 3번
const int LIGHT_PIN = 34;   // 실드 A3

// ── 조도 기준 — 1단계에서 정한 숫자 ───────────────────
const int  DARK_LEVEL   = 30;     // 주변 밝기(%)가 이 숫자보다 낮으면 '어두움' → 켜짐
const int  DARK_MARGIN  = 5;      // 기준 + 5% 이상이면 '밝음' → 꺼짐 (경계에서 깜빡임 방지)
const bool DARK_IS_HIGH = true;   // 어두울수록 값이 커지는 모듈(이번 부품) = true · 가렸는데 %가 내려가면 false
const bool USE_PIR      = true;   // false면 PIR 없이 조도만으로 켜고 끔
const unsigned long AUTO_OFF_MS = 5UL * 60 * 1000;   // 움직임이 없으면 이 시간 뒤 끔 (USE_PIR일 때)

// ── 네오픽셀 ─────────────────────────────────────
const int  LED_COUNT      = 24;    // 24구 링
const bool LED_RGBW       = true;  // RGBW 링이면 true
const int  MAX_BRIGHTNESS = 128;   // 어댑터 보호 상한 (24구 RGBW · 5V 2A 기준 약 1A)
const unsigned long FADE_MS = 1000;

const unsigned long LIGHT_READ_MS   = 200;
const unsigned long SENSOR_PRINT_MS = 1000;

Adafruit_NeoPixel ring(LED_COUNT, LED_PIN, (LED_RGBW ? NEO_GRBW : NEO_GRB) + NEO_KHZ800);

// ── 상태 ─────────────────────────────────────────
bool lampOn = false, autoMode = true, isDark = false;
bool motion = false, sensorPrint = false;
int  lightAvg = 0, lightPct = 0;
unsigned long lastMotionMs = 0, lastLightMs = 0, lastSensorPrintMs = 0;

uint8_t colR = 0, colG = 0, colB = 0, colW = 255;   // 기본: 웜화이트
String  colName = "warm";
bool    rainbow = false;
int     brightPct = 60;
int     fadeFrom = 0, fadeTo = 0, level = 0;
unsigned long fadeStart = 0, lastRainbowMs = 0;
uint16_t rainbowHue = 0;
String inputLine;

// ─────────────────────────────────────────────────
// 센서
// ─────────────────────────────────────────────────
int lightPercent(int raw) {
  int pct = (long)raw * 100 / 4095;
  return DARK_IS_HIGH ? 100 - pct : pct;          // 100 = 밝음
}

void updateDarkness() {
  if (!isDark && lightPct < DARK_LEVEL)                isDark = true;
  if (isDark  && lightPct >= DARK_LEVEL + DARK_MARGIN) isDark = false;
}

// ─────────────────────────────────────────────────
// 네오픽셀
// ─────────────────────────────────────────────────
int  targetLevel() { return lampOn ? (long)MAX_BRIGHTNESS * brightPct / 100 : 0; }
void startFade()   { fadeFrom = level; fadeTo = targetLevel(); fadeStart = millis(); }
void setLamp(bool on) { lampOn = on; startFade(); }

void showRing() {
  for (int i = 0; i < LED_COUNT; i++) {
    if (rainbow)       ring.setPixelColor(i, ring.gamma32(ring.ColorHSV(rainbowHue + i * 65536L / LED_COUNT)));
    else if (LED_RGBW) ring.setPixelColor(i, colR, colG, colB, colW);
    else               ring.setPixelColor(i, colR, colG, colB);
  }
  ring.setBrightness(level);
  ring.show();
}

void updateLights(unsigned long now) {              // 서서히 켜고 끄기 — delay() 없이
  bool dirty = false;
  if (level != fadeTo) {
    unsigned long t = now - fadeStart;
    level = t >= FADE_MS ? fadeTo : fadeFrom + (long)(fadeTo - fadeFrom) * (long)t / (long)FADE_MS;
    dirty = true;
  }
  if (rainbow && level > 0 && now - lastRainbowMs >= 30) { lastRainbowMs = now; rainbowHue += 200; dirty = true; }
  if (dirty) showRing();
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
      if (!LED_RGBW && colW) { bool white = (v == "white"); colR = 255; colG = white ? 255 : 170; colB = white ? 255 : 80; colW = 0; }
      rainbow = false; colName = p.n;
      return true;
    }
  }
  if (v.length() == 6) {
    for (unsigned i = 0; i < 6; i++) if (!isxdigit((unsigned char)v[i])) return false;
    long rgb = strtol(v.c_str(), nullptr, 16);
    colR = (rgb >> 16) & 0xFF; colG = (rgb >> 8) & 0xFF; colB = rgb & 0xFF; colW = 0;
    rainbow = false; v.toUpperCase(); colName = v;
    return true;
  }
  return false;
}

// ─────────────────────────────────────────────────
// 시리얼
// ─────────────────────────────────────────────────
void printStatus() {
  Serial.printf("[상태] 램프=%s 모드=%s 색=%s 밝기=%d%% 출력=%d/%d | 주변=%d%% %s(기준 %d%%) PIR=%d\n",
                lampOn ? "ON" : "OFF", autoMode ? "AUTO" : "MANUAL", colName.c_str(), brightPct, level, MAX_BRIGHTNESS,
                lightPct, isDark ? "어두움" : "밝음", DARK_LEVEL, motion);
}

void printSensors(unsigned long now) {
  unsigned long since = (now - lastMotionMs) / 1000;
  char remain[40] = "";
  if (lampOn && autoMode && USE_PIR)
    snprintf(remain, sizeof(remain), " | 움직임 없으면 %lds 뒤 꺼짐", (long)(AUTO_OFF_MS / 1000) - (long)since);
  Serial.printf("[센서] 조도=%4d 밝기=%3d%% → %s (기준 %d%%) | PIR=%d 마지막 감지 %lus 전%s\n",
                lightAvg, lightPct, isDark ? "어두움" : "밝음  ", DARK_LEVEL, motion, since, remain);
}

void handleCommand(String line) {
  line.trim();
  String lower = line; lower.toLowerCase();
  if (lower == "on")        { autoMode = false; setLamp(true);  Serial.println("수동 켜기"); }
  else if (lower == "off")  { autoMode = false; setLamp(false); Serial.println("수동 끄기"); }
  else if (lower == "auto") { autoMode = true;  lastMotionMs = millis(); Serial.println("자동 모드"); }
  else if (lower == "s")    printStatus();
  else if (lower == "sensor") {
    sensorPrint = !sensorPrint;
    Serial.println(sensorPrint ? "센서 값 1초마다 표시 — 'sensor'를 다시 입력하면 멈춤" : "센서 값 표시 끔");
    if (sensorPrint) printSensors(millis());
  }
  else if (lower.startsWith("c ")) {
    if (!setColor(line.substring(2))) { Serial.println("색: warm white red orange yellow green blue purple pink rainbow 또는 FF8800"); return; }
    showRing();
    Serial.println("색 → " + colName);
  }
  else if (lower.startsWith("b ")) { brightPct = constrain(lower.substring(2).toInt(), 0, 100); startFade(); Serial.printf("밝기 → %d%%\n", brightPct); }
  else if (line.length()) Serial.println("명령: on / off / auto / s / sensor / c <색> / b <0~100>");
}

void readSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') { handleCommand(inputLine); inputLine = ""; }
    else inputLine += c;
  }
}

// ─────────────────────────────────────────────────
void setup() {
  pinMode(PIR_PIN, INPUT);
  Serial.begin(115200);
  ring.begin();
  ring.clear();
  ring.show();                                   // 부팅 직후 모두 끄기
  lightAvg = analogRead(LIGHT_PIN);
  lightPct = lightPercent(lightAvg);
  updateDarkness();
  Serial.printf("IoT 무드등 V1 (네오픽셀) — 자동 모드 · 어두움 기준 %d%% · PIR %s\n", DARK_LEVEL, USE_PIR ? "사용" : "사용 안 함");
}

void loop() {
  unsigned long now = millis();
  readSerial();

  if (now - lastLightMs >= LIGHT_READ_MS) {
    lastLightMs = now;
    lightAvg = (lightAvg * 3 + analogRead(LIGHT_PIN)) / 4;
    lightPct = lightPercent(lightAvg);
    updateDarkness();
  }

  bool m = digitalRead(PIR_PIN) == HIGH;
  if (m) lastMotionMs = now;
  if (m != motion) { motion = m; if (sensorPrint) Serial.println(m ? "[센서] PIR ▲ 감지됨" : "[센서] PIR ▽ 해제"); }
  if (sensorPrint && now - lastSensorPrintMs >= SENSOR_PRINT_MS) { lastSensorPrintMs = now; printSensors(now); }

  if (autoMode) {
    bool present = !USE_PIR || motion;
    if (!lampOn && isDark && present)                                  { setLamp(true);  Serial.println(USE_PIR ? "자동 켜기 (어두움 + 사람)" : "자동 켜기 (어두움)"); }
    else if (lampOn && !isDark)                                        { setLamp(false); Serial.println("자동 끄기 (밝아짐)"); }
    else if (lampOn && USE_PIR && now - lastMotionMs >= AUTO_OFF_MS)   { setLamp(false); Serial.println("자동 끄기 (사람 없음)"); }
  }
  updateLights(now);
}
