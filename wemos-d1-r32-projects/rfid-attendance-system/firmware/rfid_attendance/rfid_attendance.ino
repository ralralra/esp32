/*
  스마트 출석체크 — ESP32 완성 펌웨어
  ----------------------------------------------------
  동작:
    1) 학교 와이파이에 접속한다
    2) 2초마다 중계서버(Apps Script)에 "할 일 있나요?" 하고 물어본다 (폴링)
       — 서버 통신은 별도 코어에서 따로 돌아서, 통신 중에도 카드 스캔이 멈추지 않는다
    3) 앱에서 출석체크(또는 등록)를 시작하면 → LED가 깜빡이며 카드 대기
       - 출석 모드: 천천히 깜빡 (0.5초)
       - 등록 모드: 빠르게 깜빡 (0.15초)
       - LED는 내장 파란 LED(GPIO2)와 외부 LED(GPIO16)가 항상 같이 켜지고 꺼진다
    4) 학생증을 태그하면 UID를 중계서버로 보내고, 결과에 따라 부저가 울린다
       - 성공: 삑삑 (짧게 2번)
       - 실패(미등록/카드 불일치 등): 삐— (길게 1번)

  준비:
    - 라이브러리: "MFRC522" (by GithubCommunity)
    - 아래 [설정] 4줄을 자기 것으로 바꾸기
    - 중계서버 배포 방법: apps_script/README.md
    - 배선: docs/wiring.md  (RC522는 반드시 3.3V!)
*/

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <MFRC522.h>

// ── [설정] 여기 4줄만 바꾸면 됩니다 ─────────────────────────
const char* WIFI_SSID  = "와이파이이름";
const char* WIFI_PASS  = "와이파이비밀번호";
const char* RELAY_URL  = "https://script.google.com/macros/s/여기에배포ID/exec";
const char* DEVICE_ID  = "DEV001";   // 앱에 표시되는 리더기 이름 (1-3반 입구)
// ──────────────────────────────────────────────────────────

// 핀 배치 (docs/wiring.md 와 같음)
#define PIN_SS      5    // RC522 SDA(SS)
#define PIN_RST     27   // RC522 RST
#define PIN_LED     2    // 보드 내장 파란 LED
#define PIN_LED_EXT 16   // 외부 LED (긴 다리 → GPIO16, 짧은 다리 → 저항(220Ω쯤) → GND)
#define PIN_BUZZER  4    // 수동부저 (없으면 안 달아도 됨 — 소리만 안 남)

const unsigned long HTTP_TIMEOUT_MS = 25000; // 서버 응답 대기 한도 (25초)
// ↑ Apps Script는 리다이렉트 때문에 요청이 두 번 일어나고, 스크립트가 처음 깨어날 때는
//   시간이 더 걸린다. 핫스팟처럼 느린 회선에서는 10초로는 부족해 -11(시간 초과)이 난다.

const unsigned long POLL_MS = 2000;          // 폴링 간격 (2초)
// ↑ 서버 요청(HTTPS)은 한 번에 2~3초씩 걸리지만, 별도 코어에서 처리하므로
//   그동안에도 카드 스캔은 계속된다. 그래서 세션 중에도 간격을 늘릴 필요가 없다.

const unsigned long RFID_CHECK_MS = 5000;    // 리더기 상태 자가진단 간격 (5초)
const unsigned long SAME_CARD_MS  = 1500;    // 같은 카드 연속 인식 무시 시간 (1.5초)

MFRC522 rfid(PIN_SS, PIN_RST);

// ── 두 개의 일을 각각 다른 코어에서 처리한다 ──────────────────
//   코어 1 (loop)        : 카드 스캔·LED·부저 — 절대 멈추지 않아야 하는 일
//   코어 0 (networkTask) : 서버 통신 — 한 번에 2~3초, 느리면 25초까지 걸리는 일
//   둘은 아래 두 개의 큐로만 주고받는다 (변수를 같이 만지면 위험)
struct TagJob   { char uid[24]; };                  // loop → 통신: "이 카드 보내줘"
struct NetEvent { uint8_t kind; char text[96]; };   // 통신 → loop: 결과 전달
const uint8_t EV_MODE = 0;    // 서버가 알려준 모드 (IDLE/ATTEND/REGISTER)
const uint8_t EV_TAG  = 1;    // 카드 전송 결과

QueueHandle_t tagQueue    = nullptr;
QueueHandle_t resultQueue = nullptr;

// 현재 상태: IDLE(대기) / ATTEND(출석 세션) / REGISTER(등록 세션) / SENDING(전송 중)
String mode = "IDLE";
unsigned long lastBlink = 0;
bool ledOn = false;

unsigned long lastRfidCheck = 0;  // 마지막 리더기 자가진단 시각
String lastUid = "";              // 마지막으로 처리한 카드 UID
unsigned long lastTagMs = 0;      // 그 카드를 처리한 시각

// 아래에서 정의하는 함수들 미리 알리기 (파일 순서와 무관하게 컴파일되도록)
void handleCard();
void checkRfidHealth();
void handleTagResult(String res);
void networkTask(void* param);
String httpGet(String url);
String explainHttpError(int code);
void beep(int ms);

// 내장 LED와 외부 LED(GPIO16)를 항상 같이 켜고 끈다
void setLed(bool on) {
  digitalWrite(PIN_LED,     on ? HIGH : LOW);
  digitalWrite(PIN_LED_EXT, on ? HIGH : LOW);
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_LED_EXT, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  setLed(false);

  SPI.begin();          // SCK=18, MISO=19, MOSI=23
  rfidInit();

  // 리더기가 응답하는지 자가진단 (배선이 틀리면 0x00 또는 0xFF가 나옴)
  byte version = rfid.PCD_ReadRegister(MFRC522::VersionReg);
  Serial.println();
  Serial.print("RC522 버전 레지스터: 0x");
  Serial.println(version, HEX);
  if (version == 0x00 || version == 0xFF) {
    Serial.println("⚠️ 리더기 응답 없음! 배선을 다시 확인하세요 (특히 3.3V, SDA=5, RST=27)");
  }

  Serial.print("와이파이 접속 중");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("접속 완료! IP: ");
  Serial.println(WiFi.localIP());

  diagnoseServer();     // 중계서버가 제대로 응답하는지 먼저 점검

  // 통신 담당을 코어 0에서 따로 돌린다.
  // 이렇게 하면 서버 응답이 늦어도 카드 스캔(코어 1)은 계속된다.
  tagQueue    = xQueueCreate(4, sizeof(TagJob));
  resultQueue = xQueueCreate(8, sizeof(NetEvent));
  if (tagQueue == nullptr || resultQueue == nullptr) {
    Serial.println("메모리 부족 - 보드를 다시 켜주세요");
    while (true) delay(1000);
  }
  if (xTaskCreatePinnedToCore(networkTask, "attend-net", 8192,
                              nullptr, 1, nullptr, 0) != pdPASS) {
    Serial.println("통신 담당 시작 실패 - 카드 인식만 동작합니다");
  }

  Serial.println("앱에서 [출석체크 시작]을 누르면 LED가 깜빡입니다.");
  beep(80); beep(80);   // 준비 완료 알림
}

// RC522 초기화 + 인식 성능 설정을 한 곳에 모았다.
// 리더기를 재초기화할 때마다 이 설정을 다시 넣어야 해서 함수로 분리.
void rfidInit() {
  rfid.PCD_Init();
  delay(50);
  // 안테나 감도를 최대로 (기본값은 중간 — 인식 거리가 짧으면 필수)
  rfid.PCD_SetAntennaGain(MFRC522::RxGain_max);
  // 송신 출력 부스트 — 안테나가 작은 카드(학생증·유스카드 등)도 깨울 수 있게
  // 반송파 구동 세기를 최대로 올린다 (uid_test에서 검증된 설정)
  rfid.PCD_WriteRegister(MFRC522::GsNReg, 0xF8);    // 기본 0x88
  rfid.PCD_WriteRegister(MFRC522::CWGsPReg, 0x3F);  // 기본 0x20
}

// 리더기가 살아있는지 5초마다 확인하고, 이상하면 재초기화한다.
// RC522는 오래 켜두거나 전원이 살짝 흔들리면 말없이 먹통이 되는 일이 흔한데,
// 그 상태에서는 카드를 아무리 대도 인식이 안 된다. 재초기화하면 바로 살아난다.
void checkRfidHealth() {
  if (millis() - lastRfidCheck < RFID_CHECK_MS) return;
  lastRfidCheck = millis();

  byte version = rfid.PCD_ReadRegister(MFRC522::VersionReg);
  byte txControl = rfid.PCD_ReadRegister(MFRC522::TxControlReg);
  // version이 0x00/0xFF면 통신 두절, 안테나 비트(하위 2개)가 꺼져 있으면 전파 송신 중단
  if (version == 0x00 || version == 0xFF || (txControl & 0x03) != 0x03) {
    Serial.println("⚠️ 리더기 응답 이상 → 재초기화");
    rfidInit();
  }
}

// 부팅할 때 중계서버를 한 번 점검한다.
// 리다이렉트를 '따라가지 않고' 어디로 보내는지 직접 본다 —
// 구글 로그인 페이지로 보내고 있으면 배포 설정이 잘못된 것이므로 바로 알 수 있다.
void diagnoseServer() {
  Serial.println();
  Serial.println("── 중계서버 점검 ──");

  String url = String(RELAY_URL);
  if (url.indexOf("/exec") < 0) {
    Serial.println("⚠️ RELAY_URL이 /exec 으로 끝나지 않습니다!");
    Serial.println("   /dev 주소는 구글 로그인이 필요해 ESP32가 쓸 수 없습니다.");
  }

  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(HTTP_TIMEOUT_MS / 1000);

  HTTPClient https;
  https.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);   // 따라가지 않고 확인만
  https.setTimeout(HTTP_TIMEOUT_MS);
  const char* headerKeys[] = { "Location" };
  https.collectHeaders(headerKeys, 1);

  if (!https.begin(client, url + "?action=periods")) {
    Serial.println("❌ 서버 연결 준비 실패 — RELAY_URL을 확인하세요.");
    Serial.println("──────────────────");
    return;
  }

  int code = https.GET();
  String location = https.header("Location");
  Serial.println("응답 코드: " + String(code));

  if (code == HTTP_CODE_OK) {
    String body = https.getString();
    body.trim();
    Serial.println("✅ 서버 정상 응답: " + body.substring(0, 60));
  } else if (code == 302 || code == 301 || code == 307) {
    if (location.indexOf("accounts.google.com") >= 0 ||
        location.indexOf("ServiceLogin") >= 0) {
      Serial.println("❌ 구글 로그인 페이지로 보내고 있습니다!");
      Serial.println("   → 배포 관리에서 액세스 권한을 '모든 사용자'로 바꾸고");
      Serial.println("     '새 버전'으로 다시 배포하세요.");
    } else {
      Serial.println("↪️ 정상적인 리다이렉트입니다 (Apps Script의 일반 동작)");
    }
  } else if (code == -11) {
    Serial.println("⚠️ 응답이 늦습니다 — 회선이 느리거나 서버가 깨어나는 중");
  } else {
    Serial.println("⚠️ 예상 밖의 응답" + explainHttpError(code));
  }

  https.end();
  Serial.println("──────────────────");
}

void loop() {
  // ① 카드 확인이 최우선 — 서버 통신이 다른 코어에서 도는 동안에도 계속 스캔한다
  handleCard();

  // ② 리더기가 먹통이 되지 않았는지 주기적으로 자가진단
  checkRfidHealth();

  // ③ 통신 담당(코어 0)이 보내온 결과 처리
  NetEvent ev;
  while (xQueueReceive(resultQueue, &ev, 0) == pdTRUE) {
    if (ev.kind == EV_TAG) {
      handleTagResult(String(ev.text));
      mode = "IDLE";            // 처리 끝 → 대기 상태로
      setLed(false);
    } else if (ev.kind == EV_MODE) {
      // 카드를 보내는 중이면 서버 모드 변경은 무시 (결과부터 마무리)
      if (mode == "SENDING") continue;
      String next = String(ev.text);
      if (next != mode) {
        Serial.println("모드 변경: " + mode + " -> " + next);
        if (next == "IDLE") setLed(false);
        mode = next;
      }
    }
  }

  // ④ 세션 중이면 LED 깜빡이기 (출석: 0.5초 / 등록: 0.15초)
  //    전송 중(SENDING)에는 깜빡이지 않고 계속 켜 둔다
  if (mode == "ATTEND" || mode == "REGISTER") {
    unsigned long interval = (mode == "REGISTER") ? 150 : 500;
    if (millis() - lastBlink >= interval) {
      lastBlink = millis();
      ledOn = !ledOn;
      setLed(ledOn);
    }
  }

  delay(5);
}

// ── 서버 통신 담당 (코어 0에서 따로 돈다) ──────────────────────
// 여기서만 인터넷을 쓴다. 한 번에 25초가 걸려도 카드 스캔(코어 1)은 멈추지 않는다.
void networkTask(void* param) {
  (void)param;
  unsigned long lastPoll = 0;

  for (;;) {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("와이파이 끊김 -> 재접속");
      WiFi.reconnect();
      vTaskDelay(pdMS_TO_TICKS(2000));
      continue;
    }

    // 보낼 카드가 있으면 폴링보다 먼저 처리한다
    TagJob job;
    if (xQueueReceive(tagQueue, &job, 0) == pdTRUE) {
      String res = httpGet(String(RELAY_URL) + "?action=tag&device=" + DEVICE_ID +
                           "&uid=" + job.uid);
      NetEvent ev{};
      ev.kind = EV_TAG;
      res.toCharArray(ev.text, sizeof(ev.text));
      xQueueSend(resultQueue, &ev, 0);
      lastPoll = millis();       // 방금 통신했으니 폴링은 나중에
    }
    else if (millis() - lastPoll >= POLL_MS) {
      lastPoll = millis();
      String res = httpGet(String(RELAY_URL) + "?action=poll&device=" + DEVICE_ID);
      if (res == "ATTEND" || res == "REGISTER" || res == "IDLE") {
        NetEvent ev{};
        ev.kind = EV_MODE;
        res.toCharArray(ev.text, sizeof(ev.text));
        xQueueSend(resultQueue, &ev, 0);
      }
    }

    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

// 카드가 태그되면 처리한다. 처리했으면 true를 반환
// 세션 중이면 서버로 전송, 대기 중이면 확인용으로 UID만 출력
void handleCard() {
  if (!cardTagged()) return;

  String uid = readUid();
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  // 같은 카드가 1.5초 안에 또 읽히면 무시 (카드를 대고 있는 동안 중복 처리 방지)
  if (uid == lastUid && millis() - lastTagMs < SAME_CARD_MS) return;
  lastUid = uid;
  lastTagMs = millis();

  if (mode == "IDLE") {
    // 세션이 없을 때도 카드가 잘 읽히는지 시리얼로 확인할 수 있게 출력만 한다
    Serial.println("카드 인식 (세션 없음 -> 전송 안 함): UID = " + uid);
    return;
  }
  if (mode == "SENDING") {
    // 앞 카드의 결과를 기다리는 중
    Serial.println("앞 사람 처리 중입니다 - 잠시 후 다시 태그해주세요");
    return;
  }

  Serial.println("태그됨! UID = " + uid + " -> 서버 전송");
  setLed(true);                 // 전송 중에는 계속 켜 둔다

  // 통신 담당(코어 0)에게 넘기고 바로 돌아온다 - 여기서 기다리지 않는다
  TagJob job{};
  uid.toCharArray(job.uid, sizeof(job.uid));
  if (xQueueSend(tagQueue, &job, 0) == pdTRUE) {
    mode = "SENDING";
  } else {
    Serial.println("전송 대기열이 가득 찼습니다 - 다시 태그해주세요");
    setLed(false);
  }
}

// 카드가 리더기 위에 있는지 WUPA(깨우기) 방식으로 확인
// 학생증 같은 보안 스마트카드(ISO 14443-4)는 일반 감지(REQA)에 응답하지 않는
// 경우가 있어서 WUPA 방식이 훨씬 안정적으로 잡는다.
// 카드가 전파 범위 가장자리에 있으면 첫 시도는 실패하고 두 번째에 잡히는 일이
// 많아서, 한 번의 호출에서 두 번까지 시도한다.
bool cardTagged() {
  for (int attempt = 0; attempt < 2; attempt++) {
    byte atqa[2];
    byte atqaSize = sizeof(atqa);
    MFRC522::StatusCode st = rfid.PICC_WakeupA(atqa, &atqaSize);
    if ((st == MFRC522::STATUS_OK || st == MFRC522::STATUS_COLLISION) &&
        rfid.PICC_ReadCardSerial()) {
      return true;
    }
    delay(5);   // 카드가 깨어날 시간을 살짝 주고 한 번 더
  }
  return false;
}

// UID를 "A1B2C3D4" 형태의 대문자 16진수 문자열로
String readUid() {
  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(rfid.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();
  return uid;
}

// 서버 응답에 따라 소리·메시지로 알려주기
// 성공: "OK,이름,출석,08:55:01" 또는 "OK_REG,이름"
// 실패: "UNKNOWN"(미등록 카드) / "DUP,이름"(이미 등록된 카드) / "MISMATCH,이름"(다른 학생 카드)
void handleTagResult(String res) {
  if (res.startsWith("OK_REG")) {
    Serial.println("✅ 등록 완료! → " + res);
    beep(80); beep(80);
  } else if (res.startsWith("OK")) {
    Serial.println("✅ 출석 확인! → " + res);
    beep(80); beep(80);
  } else if (res.startsWith("UNKNOWN")) {
    Serial.println("❌ 등록되지 않은 카드입니다. 먼저 앱에서 등록하세요.");
    beep(600);
  } else if (res.startsWith("DUP")) {
    Serial.println("❌ 이미 다른 학생으로 등록된 카드입니다 → " + res);
    beep(600);
  } else if (res.startsWith("MISMATCH")) {
    Serial.println("❌ 대상 학생의 카드가 아닙니다 → " + res);
    beep(600);
  } else if (res.startsWith("ALREADY")) {
    Serial.println("❌ 이미 이 교시에 출석 처리된 학생입니다 → " + res);
    beep(600);
  } else {
    Serial.println("⚠️ 서버 응답: " + res);
    beep(600);
  }
}

// HTTPS GET 요청 → 응답 본문(문자열) 반환
// Apps Script는 응답할 때 주소를 한 번 이동(redirect)시키므로 따라가기 설정이 꼭 필요.
// 그래서 사실상 요청이 두 번 일어나고, 스크립트가 깨어나는 첫 요청은 더 느리다.
// 타임아웃을 넉넉히 주고, 시간 초과(-11)면 한 번 더 시도한다.
String httpGet(String url) {
  for (int attempt = 1; attempt <= 2; attempt++) {
    WiFiClientSecure client;
    client.setInsecure();          // 수업용: 인증서 검증 생략 (통신 자체는 HTTPS 암호화됨)
    client.setTimeout(HTTP_TIMEOUT_MS / 1000);   // 이 함수는 '초' 단위

    HTTPClient https;
    https.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    https.setTimeout(HTTP_TIMEOUT_MS);
    https.setReuse(false);         // 매 요청마다 새 연결 (Apps Script 리다이렉트에 안전)

    if (!https.begin(client, url)) {
      Serial.println("서버 연결 준비 실패 (URL을 확인하세요)");
      return "";
    }

    int code = https.GET();
    if (code == HTTP_CODE_OK) {
      String body = https.getString();
      body.trim();
      https.end();
      return body;
    }

    https.end();
    Serial.println("HTTP 오류: " + String(code) + explainHttpError(code));
    if (code != -11 || attempt == 2) return "";   // 시간 초과일 때만 한 번 더
    Serial.println("  → 다시 시도합니다...");
    delay(500);
  }
  return "";
}

// 자주 나오는 오류 코드에 설명을 붙여 준다
String explainHttpError(int code) {
  if (code == -11) return " (읽기 시간 초과 — 서버 응답이 느림/네트워크 지연)";
  if (code == -1)  return " (연결 실패 — 인터넷 또는 URL 확인)";
  if (code == -5)  return " (연결 끊김)";
  if (code == -7)  return " (서버 응답 아님 — URL이 /exec 으로 끝나는지 확인)";
  if (code == 401 || code == 403) return " (권한 — 배포 액세스를 '모든 사용자'로)";
  if (code == 404) return " (주소 없음 — 배포 URL 확인)";
  return "";
}

// 수동부저 삑 소리 (ms 만큼)
// 2kHz 사각파를 직접 만든다 — ESP32 코어 버전과 관계없이 동작
void beep(int ms) {
  long cycles = (long)ms * 2;          // 2kHz → 한 주기 0.5ms
  for (long i = 0; i < cycles; i++) {
    digitalWrite(PIN_BUZZER, HIGH); delayMicroseconds(250);
    digitalWrite(PIN_BUZZER, LOW);  delayMicroseconds(250);
  }
  delay(60);
}
