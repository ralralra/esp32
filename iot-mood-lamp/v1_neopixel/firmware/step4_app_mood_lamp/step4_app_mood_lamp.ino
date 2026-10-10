/*
  IoT 무드등 V1 · 4단계 — 앱·음성 연동 완성 펌웨어 (네오픽셀)

  ① 아래 "여기만 바꾸기" 4줄을 고친 뒤 업로드하세요.
     WIFI_SSID / WIFI_PASS  : 2.4GHz 와이파이 (5GHz는 ESP32가 못 붙어요)
     SERVER_URL             : 선생님이 배포한 Apps Script 웹앱 주소 (끝이 /exec)
     TEAM_ID                : 내 팀 번호 TEAM01 ~ TEAM12 (반 전체가 시트 하나를 같이 써서 꼭 달라야 해요)
  ② 1단계에서 정한 DARK_LEVEL · DARK_IS_HIGH 를 3단계와 똑같이 적으세요.

  보드: Wemos D1 R32 + 센서쉴드 V5  (배선: ../../wiring.md)
    네오픽셀 → 실드 6번 (GPIO27, DIN에 330Ω · 링 5V–GND에 1000µF)   PIR → 실드 3번 (GPIO25)   조도 → A3 (GPIO34, VCC는 3V3)
  라이브러리: Adafruit NeoPixel

  동작
    - 자동 모드: 3단계와 같은 규칙 (어두움 + 사람 → 켜짐 / 밝아짐·사람 없음 → 꺼짐)
    - 2초마다 서버에서 내 팀 명령을 하나씩 가져와 실행 (ON · OFF · AUTO · COLOR · BRIGHT · CONFIG)
    - 상태가 바뀌면 바로, 아니어도 30초마다 서버에 보고 → 앱 화면에 표시
    - 통신은 다른 코어에서 돌아서, 와이파이가 느리거나 끊겨도 자동 동작·페이드가 멈추지 않음
    - 앱 자동화 탭의 CONFIG(어두움 기준 % · 자동 꺼짐 초)는 보드에 저장되어 껐다 켜도 유지

  시리얼(115200) 명령: on / off / auto / s(상태) / n(서버 확인) / c <색> / b <0~100>
*/

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <Adafruit_NeoPixel.h>

// ── ① 여기만 바꾸기 ─────────────────────────────────
const char* WIFI_SSID  = "와이파이이름";
const char* WIFI_PASS  = "와이파이비밀번호";
const char* SERVER_URL = "https://script.google.com/macros/s/배포ID/exec";
const char* TEAM_ID    = "TEAM01";

// ── ② 조도 기준 — 1단계에서 정한 숫자 ───────────────────
const int  DARK_LEVEL   = 30;     // 주변 밝기(%)가 이 숫자보다 낮으면 '어두움' → 켜짐 (앱 CONFIG로 바꿀 수 있음)
const int  DARK_MARGIN  = 5;      // 기준 + 5% 이상이면 '밝음' → 꺼짐
const bool DARK_IS_HIGH = true;   // 어두울수록 값이 커지는 모듈(이번 부품) = true · 가렸는데 %가 내려가면 false
const bool USE_PIR      = true;   // false면 PIR 없이 조도만으로 켜고 끔

// ── 핀 ───────────────────────────────────────────
const int LED_PIN   = 27;   // 실드 6번
const int PIR_PIN   = 25;   // 실드 3번
const int LIGHT_PIN = 34;   // 실드 A3

// ── 네오픽셀 ─────────────────────────────────────
const int  LED_COUNT      = 24;    // 24구 링
const bool LED_RGBW       = true;  // RGBW 링이면 true
const int  MAX_BRIGHTNESS = 128;   // 어댑터 보호 상한 (24구 RGBW · 5V 2A 기준 약 1A)
const unsigned long FADE_MS = 1000;

// ── 통신 간격 ────────────────────────────────────────
const unsigned long POLL_MS         = 2000;
const unsigned long HEARTBEAT_MS    = 30000;
const unsigned long SOFT_REPORT_MS  = 5000;
const unsigned long HTTP_TIMEOUT_MS = 8000;
const unsigned long WIFI_RETRY_MS   = 10000;
const unsigned long LIGHT_READ_MS   = 200;

Adafruit_NeoPixel ring(LED_COUNT, LED_PIN, (LED_RGBW ? NEO_GRBW : NEO_GRB) + NEO_KHZ800);

// ── 램프 상태 (loop 전용) ─────────────────────────────
bool lampOn = false;
bool autoMode = true;
bool isDark = false;
bool motion = false;
int  lightPct = 0;
int  darkPct = DARK_LEVEL;            // 앱 CONFIG로 바뀜 (보드에 저장)
unsigned long autoOffMs = 300000;     // 앱 CONFIG로 바뀜
unsigned long lastMotionMs = 0;
unsigned long lastLightMs = 0;

uint8_t colR = 0, colG = 0, colB = 0, colW = 255;   // 기본: 웜화이트(W 채널)
char    colName[12] = "warm";
bool    rainbow = false;
int     brightPct = 60;
int     fadeFrom = 0, fadeTo = 0, level = 0;
unsigned long fadeStart = 0, lastRainbowMs = 0;
uint16_t rainbowHue = 0;
String inputLine;
Preferences prefs;

// ── 두 작업(loop ↔ 통신)이 주고받는 것 ─────────────────
struct Command { char cmd[10]; char value[20]; int dark; int off; };
struct Snapshot { bool lampOn, autoMode, motion; int lightPct, darkPct, offSec, brightPct; char color[12]; };
QueueHandle_t cmdQueue;
Snapshot shared;
portMUX_TYPE sharedMux = portMUX_INITIALIZER_UNLOCKED;
volatile bool reportNow = true;
volatile bool reportSoon = false;
volatile bool pingNow = false;
volatile int  netLastCode = 0;

// ─────────────────────────────────────────────────
// 네오픽셀
// ─────────────────────────────────────────────────
int targetLevel() { return lampOn ? (long)MAX_BRIGHTNESS * brightPct / 100 : 0; }
void startFade() { fadeFrom = level; fadeTo = targetLevel(); fadeStart = millis(); }

void showRing() {
  for (int i = 0; i < LED_COUNT; i++) {
    if (rainbow)       ring.setPixelColor(i, ring.gamma32(ring.ColorHSV(rainbowHue + i * 65536L / LED_COUNT)));
    else if (LED_RGBW) ring.setPixelColor(i, colR, colG, colB, colW);
    else               ring.setPixelColor(i, colR, colG, colB);
  }
  ring.setBrightness(level);
  ring.show();
}

void updateLights(unsigned long now) {
  bool dirty = false;
  if (level != fadeTo) {
    unsigned long t = now - fadeStart;
    level = t >= FADE_MS ? fadeTo : fadeFrom + (long)(fadeTo - fadeFrom) * (long)t / (long)FADE_MS;
    dirty = true;
  }
  if (rainbow && level > 0 && now - lastRainbowMs >= 30) {   // 색상환을 천천히 한 바퀴 (약 10초)
    lastRainbowMs = now;
    rainbowHue += 200;
    dirty = true;
  }
  if (dirty) showRing();
}

void setLamp(bool on) { lampOn = on; startFade(); }

bool setColor(String v) {
  struct Preset { const char* n; uint8_t r, g, b, w; };
  static const Preset presets[] = {
    {"warm", 0, 0, 0, 255},    {"white", 120, 120, 120, 255}, {"red", 255, 0, 0, 0},
    {"orange", 255, 90, 0, 0}, {"yellow", 255, 200, 0, 0},    {"green", 0, 255, 0, 0},
    {"blue", 0, 60, 255, 0},   {"purple", 150, 0, 255, 0},    {"pink", 255, 60, 120, 0},
  };
  v.trim();
  v.toLowerCase();
  if (v == "rainbow") { rainbow = true; strcpy(colName, "rainbow"); return true; }
  for (const Preset& p : presets) {
    if (v == p.n) {
      colR = p.r; colG = p.g; colB = p.b; colW = p.w;
      if (!LED_RGBW && colW) {                 // RGB 링에는 흰 칩이 없어서 섞어서 흉내
        bool white = (v == "white");
        colR = 255; colG = white ? 255 : 170; colB = white ? 255 : 80; colW = 0;
      }
      rainbow = false;
      strncpy(colName, p.n, sizeof(colName) - 1);
      return true;
    }
  }
  if (v.length() == 6) {                       // 6자리 색 코드 (# 없이)
    for (unsigned i = 0; i < 6; i++) if (!isxdigit((unsigned char)v[i])) return false;
    long rgb = strtol(v.c_str(), nullptr, 16);
    colR = (rgb >> 16) & 0xFF; colG = (rgb >> 8) & 0xFF; colB = rgb & 0xFF; colW = 0;
    rainbow = false;
    v.toUpperCase();
    strncpy(colName, v.c_str(), sizeof(colName) - 1);
    return true;
  }
  return false;
}

// ─────────────────────────────────────────────────
// 센서·상태
// ─────────────────────────────────────────────────
int readLightPct() {
  int pct = (long)analogRead(LIGHT_PIN) * 100 / 4095;
  return DARK_IS_HIGH ? 100 - pct : pct;        // 100 = 밝음
}

void updateDarkness() {
  if (!isDark && lightPct < darkPct)                isDark = true;
  if (isDark  && lightPct >= darkPct + DARK_MARGIN) isDark = false;
}

void publish() {
  Snapshot s = { lampOn, autoMode, motion, lightPct, darkPct, (int)(autoOffMs / 1000), brightPct, "" };
  strncpy(s.color, colName, sizeof(s.color) - 1);
  portENTER_CRITICAL(&sharedMux);
  shared = s;
  portEXIT_CRITICAL(&sharedMux);
}

void changed() { publish(); reportNow = true; }

void printStatus() {
  Serial.printf("[%s] 램프=%s 모드=%s 색=%s 밝기=%d%% 출력=%d/%d | 주변=%d%% %s(기준 %d%%) PIR=%d 자동꺼짐=%lus 서버=%d\n",
                TEAM_ID, lampOn ? "ON" : "OFF", autoMode ? "AUTO" : "MANUAL", colName, brightPct, level,
                MAX_BRIGHTNESS, lightPct, isDark ? "어두움" : "밝음", darkPct, motion, autoOffMs / 1000, netLastCode);
}

void applyCommand(const Command& c) {
  String cmd = c.cmd;
  if (cmd == "ON")        { autoMode = false; setLamp(true);  Serial.println("명령: 켜기 (수동)"); }
  else if (cmd == "OFF")  { autoMode = false; setLamp(false); Serial.println("명령: 끄기 (수동)"); }
  else if (cmd == "AUTO") { autoMode = true; lastMotionMs = millis(); Serial.println("명령: 자동 모드"); }
  else if (cmd == "COLOR") {
    if (!setColor(c.value)) { Serial.printf("모르는 색: %s\n", c.value); return; }
    showRing();
    Serial.printf("명령: 색 → %s\n", colName);
  }
  else if (cmd == "BRIGHT") {
    brightPct = constrain(atoi(c.value), 0, 100);
    startFade();
    Serial.printf("명령: 밝기 → %d%%\n", brightPct);
  }
  else if (cmd == "CONFIG") {
    darkPct = constrain(c.dark, 0, 100);
    autoOffMs = (unsigned long)constrain(c.off, 10, 3600) * 1000;
    prefs.putInt("dark", darkPct);
    prefs.putInt("off", (int)(autoOffMs / 1000));
    Serial.printf("명령: 설정 — 어두움 기준 %d%%, 자동 꺼짐 %lu초 (저장됨)\n", darkPct, autoOffMs / 1000);
  }
  else { Serial.println("모르는 명령: " + cmd); return; }
  changed();
}

void handleSerial(String line) {
  line.trim();
  String lower = line;
  lower.toLowerCase();
  Command c = {};
  if (lower == "on")        strcpy(c.cmd, "ON");
  else if (lower == "off")  strcpy(c.cmd, "OFF");
  else if (lower == "auto") strcpy(c.cmd, "AUTO");
  else if (lower == "s")    { printStatus(); return; }
  else if (lower == "n")    { pingNow = true; return; }
  else if (lower.startsWith("c ")) { strcpy(c.cmd, "COLOR");  strncpy(c.value, line.substring(2).c_str(), sizeof(c.value) - 1); }
  else if (lower.startsWith("b ")) { strcpy(c.cmd, "BRIGHT"); strncpy(c.value, line.substring(2).c_str(), sizeof(c.value) - 1); }
  else { if (line.length()) Serial.println("명령: on / off / auto / s / n / c <색> / b <0~100>"); return; }
  applyCommand(c);
}

void readSerial() {
  while (Serial.available()) {
    char ch = Serial.read();
    if (ch == '\n' || ch == '\r') { handleSerial(inputLine); inputLine = ""; }
    else inputLine += ch;
  }
}

// ─────────────────────────────────────────────────
// 통신 (다른 코어에서 실행)
// ─────────────────────────────────────────────────
int httpsGet(const String& url, String& body) {
  WiFiClientSecure client;
  HTTPClient http;
  client.setInsecure();                  // 수업용. 운영에서는 루트 인증서를 등록
  http.setTimeout(HTTP_TIMEOUT_MS);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);   // Apps Script는 한 번 다른 주소로 넘겨줌
  if (!http.begin(client, url)) return -1000;
  int code = http.GET();
  if (code > 0) body = http.getString();
  http.end();
  return code;
}

String explainError(int code) {
  if (code == -1)  return " — 서버까지 연결 안 됨 (와이파이가 인터넷 되는지 확인)";
  if (code == -11) return " — 응답이 늦음 (잠시 후 자동 재시도)";
  if (code == 404) return " — 주소 없음 (SERVER_URL이 /exec 로 끝나는지)";
  if (code == 401 || code == 403) return " — 권한 (배포 액세스를 '모든 사용자'로)";
  return "";
}

// {"cmd":"CONFIG","value":"","dark":30,"off":300} 에서 값 하나 꺼내기
String jsonStr(const String& body, const char* key) {
  String k = String("\"") + key + "\"";
  int p = body.indexOf(k);
  if (p < 0) return "";
  int colon = body.indexOf(':', p + k.length());
  int q1 = body.indexOf('"', colon + 1);
  int comma = body.indexOf(',', colon + 1);
  if (q1 < 0 || (comma >= 0 && comma < q1)) return "";
  int q2 = body.indexOf('"', q1 + 1);
  return q2 < 0 ? "" : body.substring(q1 + 1, q2);
}

int jsonInt(const String& body, const char* key, int fallback) {
  String k = String("\"") + key + "\"";
  int p = body.indexOf(k);
  if (p < 0) return fallback;
  int colon = body.indexOf(':', p + k.length());
  return body.substring(colon + 1).toInt();
}

String reportQuery() {
  Snapshot s;
  portENTER_CRITICAL(&sharedMux);
  s = shared;
  portEXIT_CRITICAL(&sharedMux);
  return String(SERVER_URL) + "?mode=report&team=" + TEAM_ID + "&type=neopixel" +
         "&power=" + (s.lampOn ? "ON" : "OFF") + "&auto=" + (s.autoMode ? 1 : 0) +
         "&color=" + s.color + "&bright=" + s.brightPct +
         "&light=" + s.lightPct + "&motion=" + (s.motion ? 1 : 0) +
         "&dark=" + s.darkPct + "&off=" + s.offSec;
}

void networkStep(unsigned long now) {
  static unsigned long lastPoll = 0, lastReport = 0, lastWiFiTry = 0, failUntil = 0;
  static bool wasUp = false;

  bool up = WiFi.status() == WL_CONNECTED;
  if (up != wasUp) {
    wasUp = up;
    Serial.println(up ? "와이파이 연결됨 — 앱 명령 대기" : "와이파이 끊김 — 자동 동작과 시리얼 명령은 계속 돼요");
    if (up) reportNow = true;
  }
  if (!up) {
    if (now - lastWiFiTry >= WIFI_RETRY_MS) { lastWiFiTry = now; WiFi.disconnect(); WiFi.begin(WIFI_SSID, WIFI_PASS); }
    return;
  }
  if (now < failUntil) return;           // 실패 직후 잠깐 쉬기 (서버에 재시도 홍수 방지)

  if (pingNow) {
    pingNow = false;
    String body;
    int code = httpsGet(String(SERVER_URL) + "?mode=state&team=" + TEAM_ID, body);
    Serial.printf("서버 확인: %d%s\n", code, explainError(code).c_str());
  }

  if (reportNow || (reportSoon && now - lastReport >= SOFT_REPORT_MS) || now - lastReport >= HEARTBEAT_MS) {
    String body;
    int code = httpsGet(reportQuery(), body);
    netLastCode = code;
    if (code == 200) { reportNow = false; reportSoon = false; lastReport = now; }
    else { Serial.printf("보고 실패 %d%s\n", code, explainError(code).c_str()); failUntil = now + 5000; return; }
  }

  if (now - lastPoll >= POLL_MS && uxQueueSpacesAvailable(cmdQueue) > 0) {
    lastPoll = now;
    String body;
    int code = httpsGet(String(SERVER_URL) + "?mode=next&team=" + TEAM_ID, body);
    netLastCode = code;
    if (code != 200) { Serial.printf("명령 확인 실패 %d%s\n", code, explainError(code).c_str()); failUntil = now + 5000; return; }
    String cmd = jsonStr(body, "cmd");
    if (cmd.length()) {
      Command c = {};
      strncpy(c.cmd, cmd.c_str(), sizeof(c.cmd) - 1);
      strncpy(c.value, jsonStr(body, "value").c_str(), sizeof(c.value) - 1);
      c.dark = jsonInt(body, "dark", darkPct);
      c.off = jsonInt(body, "off", 300);
      xQueueSend(cmdQueue, &c, 0);
    }
  }
}

void networkTask(void*) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  for (;;) {
    networkStep(millis());
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

// ─────────────────────────────────────────────────
void setup() {
  pinMode(PIR_PIN, INPUT);
  Serial.begin(115200);
  ring.begin();
  ring.clear();
  ring.show();                                   // 부팅 직후 모든 LED 끄기

  prefs.begin("moodlamp", false);
  darkPct = prefs.getInt("dark", darkPct);
  autoOffMs = (unsigned long)prefs.getInt("off", (int)(autoOffMs / 1000)) * 1000;

  lightPct = readLightPct();
  updateDarkness();
  publish();

  cmdQueue = xQueueCreate(8, sizeof(Command));
  xTaskCreatePinnedToCore(networkTask, "lamp-net", 8192, nullptr, 1, nullptr, 0);   // 통신은 코어 0
  Serial.printf("IoT 무드등 V1 네오픽셀 [%s] 시작 — 어두움 기준 %d%%, 자동 꺼짐 %lu초, PIR %s\n",
                TEAM_ID, darkPct, autoOffMs / 1000, USE_PIR ? "사용" : "사용 안 함");
}

void loop() {
  unsigned long now = millis();
  readSerial();

  Command c;
  while (xQueueReceive(cmdQueue, &c, 0) == pdTRUE) applyCommand(c);

  if (now - lastLightMs >= LIGHT_READ_MS) {
    lastLightMs = now;
    lightPct = (lightPct * 3 + readLightPct()) / 4;     // 이동평균
    updateDarkness();
  }

  bool m = digitalRead(PIR_PIN) == HIGH;
  if (m) lastMotionMs = now;
  if (m != motion) { motion = m; reportSoon = true; }

  if (autoMode) {
    bool present = !USE_PIR || motion;
    if (!lampOn && isDark && present)                                { setLamp(true);  Serial.println("자동 켜기 (어두움)"); changed(); }
    else if (lampOn && !isDark)                                      { setLamp(false); Serial.println("자동 끄기 (밝아짐)"); changed(); }
    else if (lampOn && USE_PIR && now - lastMotionMs >= autoOffMs)   { setLamp(false); Serial.println("자동 끄기 (사람 없음)"); changed(); }
  }
  updateLights(now);
  publish();
}
