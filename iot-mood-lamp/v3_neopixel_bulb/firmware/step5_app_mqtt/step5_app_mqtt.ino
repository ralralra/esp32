/*
  IoT 무드등 V3 · 5단계 — 앱·음성 연동 '빠른 응답' 펌웨어 (네오픽셀 + 릴레이 전구 · MQTT)

  3단계(step3_auto_combo)의 자동 동작에 MQTT 통신을 얹은 것입니다. 명령을 브로커가 밀어주어 0.1~0.5초 안에 반응합니다.
  V1·V2 5단계와 같은 토픽·명령을 쓰고, 조명이 둘이라 NP / BULB 명령과 state 항목이 추가됩니다.

  ① 아래 "여기만 바꾸기"를 고친 뒤 업로드하세요.
     WIFI_SSID / WIFI_PASS    : 2.4GHz 와이파이
     MQTT_HOST / PORT / USER / PASS : 선생님이 만든 브로커 (docs/mqtt.md) — 공개 브로커면 USER·PASS 비움
     TOPIC_PREFIX             : 우리 반 고유 문자열 (예: moodlamp/3반-a7x9)
     TEAM_ID                  : 내 팀 번호 TEAM01 ~ TEAM12
  ② 1단계에서 정한 DARK_LEVEL · DARK_IS_HIGH 를 3단계와 똑같이 적으세요.

  보드: Wemos D1 R32 + 센서쉴드 V5  (배선·전원: ../../wiring.md — 와이파이를 쓰므로 9V 건전지 A안은 권하지 않음)
    네오픽셀 12구 → 실드 6번 (GPIO27)   릴레이 → 실드 2번 (GPIO26)   PIR → 실드 3번 (GPIO25)   조도 → A3 (GPIO34, VCC는 3V3)
  라이브러리: Adafruit NeoPixel, PubSubClient

  토픽 (TOPIC_PREFIX/TEAM_ID/… )
    …/cmd     ← 앱이 보냄: ON · OFF (둘 다) · NP ON · NP OFF · BULB ON · BULB OFF · AUTO · MANUAL · COLOR blue · BRIGHT 50 · CONFIG 30 60
                MANUAL = 지금 켜진 것은 그대로 둔 채 자동만 끔 (앱의 [수동] 버튼용)
                CONFIG <어두움%> <초> — 초는 '사람 없으면 네오픽셀 꺼짐' 시간
    …/state   → 보드가 보냄 (retained):
                {"type":"combo","power":"ON","np":"ON","bulb":"OFF","auto":1,"color":"warm","bright":60,"light":32,"motion":1,"dark":30,"off":60}
                power 는 둘 중 하나라도 켜져 있으면 ON (V1·V2용 앱 화면과 호환)
    …/status  → online / offline (retained)

  자동 모드: 사람 → 네오픽셀 (NP_HOLD 뒤 꺼짐) · 어두움 → 전구 (3단계와 같음)
  시리얼(115200): on / off / np on|off / bulb on|off / auto / s / c <색> / b <0~100>
*/

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <Preferences.h>
#include <Adafruit_NeoPixel.h>

// ── ① 여기만 바꾸기 ─────────────────────────────────
const char* WIFI_SSID    = "와이파이이름";
const char* WIFI_PASS    = "와이파이비밀번호";
const char* MQTT_HOST    = "xxxxxxxx.ala.asia-southeast1.emqxsl.com";   // 브로커 주소 (docs/mqtt.md)
const int   MQTT_PORT    = 8883;                 // TLS 8883 · 공개 브로커를 TLS 없이 쓰면 1883 + MQTT_TLS false
const bool  MQTT_TLS     = true;
const char* MQTT_USER    = "moodlamp";           // 브로커에서 만든 계정 (공개 브로커면 "")
const char* MQTT_PASS    = "비밀번호";
const char* TOPIC_PREFIX = "moodlamp/3반-a7x9";  // 우리 반 고유 문자열
const char* TEAM_ID      = "TEAM01";

// ── ② 조도 기준 — 1단계에서 정한 숫자 ───────────────────
const int  DARK_LEVEL   = 30;     // 주변 밝기(%)가 이 숫자보다 낮으면 '어두움' → 전구 켜짐 (앱 CONFIG로 바꿀 수 있음)
const int  DARK_MARGIN  = 5;      // 기준 + 5% 이상이면 '밝음' → 전구 꺼짐
const bool DARK_IS_HIGH = true;   // 어두울수록 값이 커지는 모듈(이번 부품) = true
const int  BULB_GLOW    = 0;      // 전구를 켜면 조도센서가 더 밝게 읽는 %  (1단계 '램프 켬' − '불 끔')
const unsigned long BULB_MIN_ON_MS = 30UL * 1000;   // 전구 최소 유지 시간 (자기 빛으로 바로 꺼지는 것 방지)
const unsigned long NP_HOLD_DEFAULT_MS = 60UL * 1000;   // 사람이 없어진 뒤 네오픽셀 유지 (앱 CONFIG로 바꿀 수 있음)
const bool NP_ONLY_WHEN_DARK = false;                   // true면 어두울 때만 사람에 반응

// ── 핀 ───────────────────────────────────────────
const int LED_PIN   = 27;   // 실드 6번
const int RELAY_PIN = 26;   // 실드 2번
const int PIR_PIN   = 25;   // 실드 3번
const int LIGHT_PIN = 34;   // 실드 A3

// ── 네오픽셀 · 릴레이 ─────────────────────────────────
#define LED_COUNT 12
const int  MAX_BRIGHTNESS = 128;      // 전원 B·C안(5V 1A 이상) 기준. 9V 건전지 A안이면 64
const unsigned long FADE_MS = 1000;
const bool RELAY_ACTIVE_LOW = false;  // KY-019형(HIGH에서 켜짐) = false · 전구만 반대면 코드가 아니라 NC→NO

// ── 통신 간격 ────────────────────────────────────────
const unsigned long HEARTBEAT_MS   = 30000;   // 바뀐 게 없어도 상태 보내기
const unsigned long SOFT_REPORT_MS = 5000;    // 움직임 변화는 5초에 한 번까지
const unsigned long RECONNECT_MS   = 5000;    // 브로커 재연결 간격
const unsigned long WIFI_RETRY_MS  = 10000;
const unsigned long LIGHT_READ_MS  = 200;

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// ── 상태 (loop 전용) ───────────────────────────────
bool npOn = false, bulbOn = false, autoMode = true, isDark = false, motion = false;
int  lightAvg = 0, lightPct = 0;
int  darkPct = DARK_LEVEL;
unsigned long npHoldMs = NP_HOLD_DEFAULT_MS;
unsigned long lastMotionMs = 0, lastLightMs = 0, bulbOnSince = 0;

uint8_t colR = 255, colG = 170, colB = 80;   // 기본: 웜화이트
char    colName[12] = "warm";
bool    rainbow = false;
int     brightPct = 60;
int     fadeFrom = 0, fadeTo = 0, level = 0;
unsigned long fadeStart = 0, lastRainbowMs = 0;
long    rainbowHue = 0;
String  inputLine;
Preferences prefs;

// ── 두 작업(loop ↔ 통신)이 주고받는 것 ─────────────────
struct Command { char cmd[10]; char value[20]; int dark; int off; };
struct Snapshot { bool npOn, bulbOn, autoMode, motion; int lightPct, darkPct, offSec, brightPct; char color[12]; };
QueueHandle_t cmdQueue;
Snapshot shared;
portMUX_TYPE sharedMux = portMUX_INITIALIZER_UNLOCKED;
volatile bool reportNow = true;
volatile bool reportSoon = false;
volatile int  netState = 0;        // 0 와이파이 없음 · 1 브로커 연결 중 · 2 연결됨

String topicCmd, topicState, topicStatus;

// ─────────────────────────────────────────────────
// 네오픽셀 (3단계와 같음)
// ─────────────────────────────────────────────────
int  targetLevel() { return npOn ? (long)MAX_BRIGHTNESS * brightPct / 100 : 0; }
void startFade()   { fadeFrom = level; fadeTo = targetLevel(); fadeStart = millis(); }
void setNp(bool on) { npOn = on; startFade(); }

void showStrip() {
  if (rainbow) strip.rainbow(rainbowHue);
  else for (int i = 0; i < LED_COUNT; i++) strip.setPixelColor(i, colR, colG, colB);
  strip.setBrightness(level);
  strip.show();
}

void updateLights(unsigned long now) {
  bool dirty = false;
  if (level != fadeTo) {
    unsigned long t = now - fadeStart;
    level = t >= FADE_MS ? fadeTo : fadeFrom + (long)(fadeTo - fadeFrom) * (long)t / (long)FADE_MS;
    dirty = true;
  }
  if (rainbow && level > 0 && now - lastRainbowMs >= 10) {
    lastRainbowMs = now; rainbowHue += 256; if (rainbowHue >= 5 * 65536L) rainbowHue = 0; dirty = true;
  }
  if (dirty) showStrip();
}

bool setColor(String v) {
  struct Preset { const char* n; uint8_t r, g, b; };
  static const Preset presets[] = {
    {"warm", 255, 170, 80}, {"white", 255, 255, 255}, {"red", 255, 0, 0},
    {"orange", 255, 90, 0}, {"yellow", 255, 200, 0},  {"green", 0, 255, 0},
    {"blue", 0, 60, 255},   {"purple", 150, 0, 255},  {"pink", 255, 60, 120},
  };
  v.trim(); v.toLowerCase();
  if (v == "rainbow") { rainbow = true; strcpy(colName, "rainbow"); return true; }
  for (const Preset& p : presets) {
    if (v == p.n) { colR = p.r; colG = p.g; colB = p.b; rainbow = false; strncpy(colName, p.n, sizeof(colName) - 1); return true; }
  }
  if (v.length() == 6) {
    for (unsigned i = 0; i < 6; i++) if (!isxdigit((unsigned char)v[i])) return false;
    long rgb = strtol(v.c_str(), nullptr, 16);
    colR = (rgb >> 16) & 0xFF; colG = (rgb >> 8) & 0xFF; colB = rgb & 0xFF;
    rainbow = false; v.toUpperCase(); strncpy(colName, v.c_str(), sizeof(colName) - 1);
    return true;
  }
  return false;
}

// ─────────────────────────────────────────────────
// 전구 · 센서
// ─────────────────────────────────────────────────
void setBulb(bool on) {
  if (on && !bulbOn) bulbOnSince = millis();
  bulbOn = on;
  digitalWrite(RELAY_PIN, (on != RELAY_ACTIVE_LOW) ? HIGH : LOW);
}

int lightPercent(int raw) {              // 원시값을 평균한 뒤 %로 — 완전히 밝으면 정확히 100이 된다
  int pct = (long)raw * 100 / 4095;
  return DARK_IS_HIGH ? 100 - pct : pct;
}

void updateDarkness() {
  int offLevel = min(100, darkPct + DARK_MARGIN + (bulbOn ? BULB_GLOW : 0));   // 기준이 높아도 100%에서는 꺼지게
  if (!isDark && lightPct < darkPct) isDark = true;
  if (isDark  && lightPct >= offLevel) isDark = false;
}

void publish() {
  Snapshot s = { npOn, bulbOn, autoMode, motion, lightPct, darkPct, (int)(npHoldMs / 1000), brightPct, "" };
  strncpy(s.color, colName, sizeof(s.color) - 1);
  portENTER_CRITICAL(&sharedMux);
  shared = s;
  portEXIT_CRITICAL(&sharedMux);
}

void changed() { publish(); reportNow = true; }

void printStatus() {
  Serial.printf("[%s] 네오픽셀=%s(%s %d%%) 전구=%s 모드=%s | 주변=%d%% %s(기준 %d%%) PIR=%d 네오픽셀유지=%lus 브로커=%s\n",
                TEAM_ID, npOn ? "ON" : "OFF", colName, brightPct, bulbOn ? "ON" : "OFF", autoMode ? "AUTO" : "MANUAL",
                lightPct, isDark ? "어두움" : "밝음", darkPct, motion, npHoldMs / 1000,
                netState == 2 ? "연결됨" : (netState == 1 ? "연결 중" : "와이파이 없음"));
}

bool onOffValue(const char* v, bool& out) {
  String s = v; s.trim(); s.toUpperCase();
  if (s == "ON")  { out = true;  return true; }
  if (s == "OFF") { out = false; return true; }
  return false;
}

void applyCommand(const Command& c) {
  String cmd = c.cmd;
  bool v;
  if (cmd == "ON")        { autoMode = false; setNp(true);  setBulb(true);  Serial.println("명령: 둘 다 켜기 (수동)"); }
  else if (cmd == "OFF")  { autoMode = false; setNp(false); setBulb(false); Serial.println("명령: 둘 다 끄기 (수동)"); }
  else if (cmd == "NP")   { if (!onOffValue(c.value, v)) { Serial.println("NP 는 ON/OFF"); return; }   autoMode = false; setNp(v);   Serial.printf("명령: 네오픽셀 %s (수동)\n", v ? "켜기" : "끄기"); }
  else if (cmd == "BULB") { if (!onOffValue(c.value, v)) { Serial.println("BULB 는 ON/OFF"); return; } autoMode = false; setBulb(v); Serial.printf("명령: 전구 %s (수동)\n", v ? "켜기" : "끄기"); }
  else if (cmd == "AUTO") { autoMode = true; lastMotionMs = millis(); Serial.println("명령: 자동 모드 — 사람→네오픽셀 · 어두움→전구"); }
  else if (cmd == "MANUAL") { autoMode = false; Serial.println("명령: 수동 모드 (지금 상태 유지)"); }
  else if (cmd == "COLOR") {
    if (!setColor(c.value)) { Serial.printf("모르는 색: %s\n", c.value); return; }
    showStrip();
    Serial.printf("명령: 색 → %s\n", colName);
  }
  else if (cmd == "BRIGHT") { brightPct = constrain(atoi(c.value), 0, 100); startFade(); Serial.printf("명령: 밝기 → %d%%\n", brightPct); }
  else if (cmd == "CONFIG") {
    darkPct = constrain(c.dark, 0, 100);
    npHoldMs = (unsigned long)constrain(c.off, 10, 3600) * 1000;
    prefs.putInt("dark", darkPct);
    prefs.putInt("off", (int)(npHoldMs / 1000));
    Serial.printf("명령: 설정 — 어두움 기준 %d%%, 네오픽셀 유지 %lu초 (저장됨)\n", darkPct, npHoldMs / 1000);
  }
  else { Serial.println("모르는 명령: " + cmd); return; }
  changed();
}

// "NP ON" · "COLOR blue" · "BRIGHT 50" · "CONFIG 30 60" · "ON" 같은 한 줄을 Command로
bool parseCommand(const String& line, Command& c) {
  c = Command{};
  String s = line; s.trim();
  int sp = s.indexOf(' ');
  String head = sp < 0 ? s : s.substring(0, sp);
  String rest = sp < 0 ? "" : s.substring(sp + 1);
  head.toUpperCase(); rest.trim();
  if (!head.length() || head.length() >= sizeof(c.cmd)) return false;
  strncpy(c.cmd, head.c_str(), sizeof(c.cmd) - 1);
  if (head == "CONFIG") {                 // CONFIG <어두움%> <네오픽셀 유지 초>
    int sp2 = rest.indexOf(' ');
    c.dark = rest.substring(0, sp2 < 0 ? rest.length() : sp2).toInt();
    c.off  = sp2 < 0 ? 60 : rest.substring(sp2 + 1).toInt();
  } else strncpy(c.value, rest.c_str(), sizeof(c.value) - 1);
  return true;
}

void handleSerial(String line) {
  line.trim();
  String lower = line; lower.toLowerCase();
  Command c;
  if (lower == "s") { printStatus(); return; }
  if (lower.startsWith("c ")) line = "COLOR " + line.substring(2);
  else if (lower.startsWith("b ")) line = "BRIGHT " + line.substring(2);
  else if (lower == "on" || lower == "off" || lower == "auto" || lower == "manual" ||
           lower == "np on" || lower == "np off" || lower == "bulb on" || lower == "bulb off") line = lower;
  else { if (line.length()) Serial.println("명령: on / off / np on|off / bulb on|off / auto / manual / s / c <색> / b <0~100>"); return; }
  if (parseCommand(line, c)) applyCommand(c);
}

void readSerial() {
  while (Serial.available()) {
    char ch = Serial.read();
    if (ch == '\n' || ch == '\r') { handleSerial(inputLine); inputLine = ""; }
    else inputLine += ch;
  }
}

// ─────────────────────────────────────────────────
// 통신 (다른 코어) — MQTT  (V1·V2 5단계와 같음)
// ─────────────────────────────────────────────────
WiFiClientSecure tlsClient;
WiFiClient       plainClient;
PubSubClient     mqtt;

void onMessage(char* topic, byte* payload, unsigned int len) {   // 브로커 → 보드 (통신 코어에서 실행)
  String line;
  for (unsigned i = 0; i < len && i < 60; i++) line += (char)payload[i];
  Command c;
  if (parseCommand(line, c)) xQueueSend(cmdQueue, &c, 0);        // 실행은 loop()에서
}

String stateJson() {
  Snapshot s;
  portENTER_CRITICAL(&sharedMux);
  s = shared;
  portEXIT_CRITICAL(&sharedMux);
  char buf[240];
  snprintf(buf, sizeof(buf),
           "{\"type\":\"combo\",\"power\":\"%s\",\"np\":\"%s\",\"bulb\":\"%s\",\"auto\":%d,\"color\":\"%s\",\"bright\":%d,"
           "\"light\":%d,\"motion\":%d,\"dark\":%d,\"off\":%d}",
           (s.npOn || s.bulbOn) ? "ON" : "OFF", s.npOn ? "ON" : "OFF", s.bulbOn ? "ON" : "OFF", s.autoMode ? 1 : 0,
           s.color, s.brightPct, s.lightPct, s.motion ? 1 : 0, s.darkPct, s.offSec);
  return String(buf);
}

bool connectBroker() {
  String clientId = String(TEAM_ID) + "-" + String((uint32_t)ESP.getEfuseMac(), HEX);
  bool ok = mqtt.connect(clientId.c_str(), MQTT_USER[0] ? MQTT_USER : nullptr, MQTT_PASS[0] ? MQTT_PASS : nullptr,
                         topicStatus.c_str(), 0, true, "offline");     // 끊기면 브로커가 대신 'offline'을 남김
  if (!ok) return false;
  mqtt.subscribe(topicCmd.c_str());
  mqtt.publish(topicStatus.c_str(), "online", true);
  return true;
}

String explainMqtt(int st) {
  if (st == -2) return " — 브로커까지 연결 안 됨 (주소·포트·TLS, 와이파이 인터넷 확인)";
  if (st == 5)  return " — 인증 실패 (MQTT_USER·MQTT_PASS, 공개 브로커면 비움)";
  if (st == 4)  return " — 계정 정보가 틀림";
  if (st == -4) return " — 응답 없음 (잠시 후 재시도)";
  return "";
}

void networkStep(unsigned long now) {
  static unsigned long lastReport = 0, lastWiFiTry = 0, lastConnectTry = 0;
  static bool wasUp = false, wasConnected = false, triedOnce = false;

  bool up = WiFi.status() == WL_CONNECTED;
  if (up != wasUp) {
    wasUp = up;
    Serial.println(up ? "와이파이 연결됨 — 브로커 연결 중" : "와이파이 끊김 — 자동 동작과 시리얼 명령은 계속 돼요");
  }
  if (!up) {
    netState = 0;
    if (now - lastWiFiTry >= WIFI_RETRY_MS) { lastWiFiTry = now; WiFi.disconnect(); WiFi.begin(WIFI_SSID, WIFI_PASS); }
    return;
  }

  if (!mqtt.connected()) {
    if (wasConnected) { wasConnected = false; Serial.println("브로커 끊김 — 다시 연결합니다"); }
    netState = 1;
    if (triedOnce && now - lastConnectTry < RECONNECT_MS) return;   // 첫 시도는 바로, 그다음은 5초마다
    triedOnce = true;
    lastConnectTry = now;
    if (connectBroker()) { wasConnected = true; netState = 2; reportNow = true; Serial.println("브로커 연결됨 — 앱 명령 대기 (" + topicCmd + ")"); }
    else Serial.printf("브로커 연결 실패 %d%s\n", mqtt.state(), explainMqtt(mqtt.state()).c_str());
    return;
  }

  mqtt.loop();                                   // 명령 받기 (onMessage) + keepalive
  if (reportNow || (reportSoon && now - lastReport >= SOFT_REPORT_MS) || now - lastReport >= HEARTBEAT_MS) {
    reportNow = false; reportSoon = false;        // 스냅샷을 뜨기 '전에' 요청을 지운다 — 발행 중 생긴 변화는 다음 보고로 이어짐
    if (mqtt.publish(topicState.c_str(), stateJson().c_str(), true)) lastReport = now;
    else reportNow = true;                        // 실패하면 다시 시도
  }
}

void networkTask(void*) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  for (;;) {
    networkStep(millis());
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// ─────────────────────────────────────────────────
void setup() {
  digitalWrite(RELAY_PIN, RELAY_ACTIVE_LOW ? HIGH : LOW);   // 부팅 순간 전구 깜빡임 방지: '꺼짐' 먼저
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(PIR_PIN, INPUT);
  Serial.begin(115200);
  strip.begin();
  strip.clear();
  strip.show();

  prefs.begin("moodlamp", false);
  darkPct  = prefs.getInt("dark", darkPct);
  npHoldMs = (unsigned long)prefs.getInt("off", (int)(npHoldMs / 1000)) * 1000;

  lightAvg = analogRead(LIGHT_PIN);
  lightPct = lightPercent(lightAvg);
  updateDarkness();
  publish();

  String base = String(TOPIC_PREFIX) + "/" + TEAM_ID + "/";
  topicCmd = base + "cmd"; topicState = base + "state"; topicStatus = base + "status";
  if (MQTT_TLS) { tlsClient.setInsecure(); mqtt.setClient(tlsClient); }   // 수업용. 운영에서는 루트 인증서 등록
  else mqtt.setClient(plainClient);
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMessage);
  mqtt.setBufferSize(512);
  mqtt.setKeepAlive(30);

  cmdQueue = xQueueCreate(8, sizeof(Command));
  xTaskCreatePinnedToCore(networkTask, "lamp-mqtt", 8192, nullptr, 1, nullptr, 0);
  Serial.printf("IoT 무드등 V3 네오픽셀+전구 MQTT [%s] 시작 — 어두움 기준 %d%%, 네오픽셀 유지 %lu초\n", TEAM_ID, darkPct, npHoldMs / 1000);
}

void loop() {
  unsigned long now = millis();
  readSerial();

  Command c;
  while (xQueueReceive(cmdQueue, &c, 0) == pdTRUE) applyCommand(c);

  if (now - lastLightMs >= LIGHT_READ_MS) {
    lastLightMs = now;
    lightAvg = (lightAvg * 3 + analogRead(LIGHT_PIN)) / 4;
    lightPct = lightPercent(lightAvg);
    updateDarkness();
  }

  bool m = digitalRead(PIR_PIN) == HIGH;
  if (m) lastMotionMs = now;
  if (m != motion) { motion = m; reportSoon = true; }

  if (autoMode) {
    bool npAllowed = !NP_ONLY_WHEN_DARK || isDark;
    if (!npOn && motion && npAllowed)                       { setNp(true);  Serial.println("자동: 네오픽셀 켜기 (사람)");   changed(); }
    else if (npOn && now - lastMotionMs >= npHoldMs)        { setNp(false); Serial.println("자동: 네오픽셀 끄기 (사람 없음)"); changed(); }
    else if (npOn && NP_ONLY_WHEN_DARK && !isDark)          { setNp(false); Serial.println("자동: 네오픽셀 끄기 (밝아짐)"); changed(); }

    if (!bulbOn && isDark)                                                    { setBulb(true);  Serial.println("자동: 전구 켜기 (어두움)"); changed(); }
    else if (bulbOn && !isDark && now - bulbOnSince >= BULB_MIN_ON_MS)        { setBulb(false); Serial.println("자동: 전구 끄기 (밝아짐)"); changed(); }
  }
  updateLights(now);
  publish();
}
