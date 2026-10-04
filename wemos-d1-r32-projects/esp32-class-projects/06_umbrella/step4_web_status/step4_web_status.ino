/*
  4단계 — 우산 대여 현황 웹 (완성본)
  ----------------------------------------------------
  목표: 폰으로 접속하면 우산별 상태(사용가능/대여중+누구)와
        미반납 목록을 한눈에 본다.
        RFID 대여/반납 + 웹 조회(출석에서 배운 웹서버) 합체.

  선생님 화면: http://192.168.4.1/
  ※ Partition: Default 4MB with spiffs
*/

#include <SPI.h>
#include <MFRC522.h>
#include <FS.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <WebServer.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#define SS_PIN  5
#define RST_PIN 27
MFRC522 rfid(SS_PIN, RST_PIN);

const int STUDENT_COUNT = 2;
String studentUID[STUDENT_COUNT]  = { "AAAA1111", "BBBB2222" };
String studentName[STUDENT_COUNT] = { "김OO",      "이OO"      };

const int UMBRELLA_COUNT = 3;
String umbrellaUID[UMBRELLA_COUNT] = { "CCCC3333", "DDDD4444", "EEEE5555" };
int    umbrellaNo[UMBRELLA_COUNT]  = { 1,          2,          3          };
int    borrowedBy[UMBRELLA_COUNT]  = { -1,         -1,         -1         };

int currentUser = -1;

const char* AP_SSID = "ESP32-Umbrella";
const char* AP_PASS = "umbrella1234";
WebServer server(80);

String readUID() {
  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(rfid.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();
  return uid;
}
int findStudent(String uid)  { for (int i=0;i<STUDENT_COUNT;i++)  if (uid==studentUID[i])  return i; return -1; }
int findUmbrella(String uid) { for (int i=0;i<UMBRELLA_COUNT;i++) if (uid==umbrellaUID[i]) return i; return -1; }

void saveState() {
  File f = SPIFFS.open("/state.csv", FILE_WRITE);
  if (!f) return;
  for (int i = 0; i < UMBRELLA_COUNT; i++) f.printf("%d\n", borrowedBy[i]);
  f.close();
}
void loadState() {
  File f = SPIFFS.open("/state.csv", FILE_READ);
  if (!f) return;
  for (int i = 0; i < UMBRELLA_COUNT && f.available(); i++)
    borrowedBy[i] = f.readStringUntil('\n').toInt();
  f.close();
}

void handleRoot() {
  int outCount = 0;
  String html = "<!doctype html><html lang='ko'><head><meta charset='utf-8'>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<meta http-equiv='refresh' content='3'>";
  html += "<title>우산 대여 현황</title><style>";
  html += "body{font-family:sans-serif;padding:1rem;max-width:480px;margin:0 auto}";
  html += "table{border-collapse:collapse;width:100%}th,td{border:1px solid #ccc;padding:8px;text-align:center}";
  html += "th{background:#2563eb;color:#fff}.out{color:#d97706;font-weight:bold}.ok{color:#16a34a}";
  html += "</style></head><body><h2>☂ 우산 대여 현황</h2>";
  html += "<table><tr><th>우산</th><th>상태</th><th>빌린 사람</th></tr>";
  for (int i = 0; i < UMBRELLA_COUNT; i++) {
    if (borrowedBy[i] == -1) {
      html += "<tr><td>" + String(umbrellaNo[i]) + "번</td><td class='ok'>사용가능</td><td>-</td></tr>";
    } else {
      outCount++;
      html += "<tr><td>" + String(umbrellaNo[i]) + "번</td><td class='out'>대여중</td><td>"
            + studentName[borrowedBy[i]] + "</td></tr>";
    }
  }
  html += "</table>";
  html += "<p>미반납: <b>" + String(outCount) + "</b>개 / 전체 " + String(UMBRELLA_COUNT) + "개</p>";
  html += "<p style='color:#888;font-size:.9rem'>3초마다 자동 새로고침</p></body></html>";
  server.send(200, "text/html; charset=utf-8", html);
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  Serial.begin(115200);
  delay(300);
  SPI.begin();
  rfid.PCD_Init();
  if (!SPIFFS.begin(true)) Serial.println("SPIFFS 실패!");
  loadState();

  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.print("현황 보기: http://");
  Serial.println(WiFi.softAPIP());
  server.on("/", handleRoot);
  server.begin();
  Serial.println("서버 시작됨!");
}

void loop() {
  server.handleClient();

  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;
  String uid = readUID();
  int s = findStudent(uid);
  int u = findUmbrella(uid);

  if (s >= 0) {
    currentUser = s;
    Serial.printf("학생 인증: %s\n", studentName[s].c_str());
  } else if (u >= 0) {
    if (borrowedBy[u] == -1) {
      if (currentUser == -1) Serial.println("먼저 학생증을 찍어주세요!");
      else {
        borrowedBy[u] = currentUser;
        saveState();
        Serial.printf("대여: %s → 우산 %d번\n", studentName[currentUser].c_str(), umbrellaNo[u]);
        currentUser = -1;
      }
    } else {
      Serial.printf("반납: 우산 %d번 (%s)\n", umbrellaNo[u], studentName[borrowedBy[u]].c_str());
      borrowedBy[u] = -1;
      saveState();
      currentUser = -1;
    }
  } else Serial.println("등록되지 않은 카드: " + uid);

  rfid.PICC_HaltA();
  delay(800);
}
