/*
  3단계 — 대여 상태/기록 저장 (전원 꺼도 유지)
  ----------------------------------------------------
  목표: 전원을 껐다 켜도 "어떤 우산을 누가 빌려갔는지" 가 그대로 남아있게 한다.
        + 대여/반납 내역을 기록(log)에 남긴다.

  ※ Partition Scheme: Default 4MB with spiffs
*/

#include <SPI.h>
#include <MFRC522.h>
#include <FS.h>
#include <SPIFFS.h>

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

// 현재 대여 상태(borrowedBy)를 파일에 저장
void saveState() {
  File f = SPIFFS.open("/state.csv", FILE_WRITE);   // 덮어쓰기
  if (!f) return;
  for (int i = 0; i < UMBRELLA_COUNT; i++) f.printf("%d\n", borrowedBy[i]);
  f.close();
}
// 부팅 시 저장된 상태 불러오기
void loadState() {
  File f = SPIFFS.open("/state.csv", FILE_READ);
  if (!f) return;
  for (int i = 0; i < UMBRELLA_COUNT && f.available(); i++)
    borrowedBy[i] = f.readStringUntil('\n').toInt();
  f.close();
}
// 대여/반납 내역을 로그에 추가
void logEvent(String who, int no, String action) {
  File f = SPIFFS.open("/log.csv", FILE_APPEND);
  if (f) { f.printf("%s,%d,%s\n", who.c_str(), no, action.c_str()); f.close(); }
}

void setup() {
  Serial.begin(115200);
  delay(300);
  SPI.begin();
  rfid.PCD_Init();
  if (!SPIFFS.begin(true)) Serial.println("SPIFFS 실패!");
  loadState();   // 이전 상태 복원
  Serial.println("\n=== 우산 대여 (저장 기능) ===");
  for (int i = 0; i < UMBRELLA_COUNT; i++)
    if (borrowedBy[i] != -1)
      Serial.printf("  복원: 우산 %d번은 %s 가 빌린 상태\n",
                    umbrellaNo[i], studentName[borrowedBy[i]].c_str());
}

void loop() {
  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;
  String uid = readUID();
  int s = findStudent(uid);
  int u = findUmbrella(uid);

  if (s >= 0) {
    currentUser = s;
    Serial.printf("학생 인증: %s\n", studentName[s].c_str());
  }
  else if (u >= 0) {
    if (borrowedBy[u] == -1) {
      if (currentUser == -1) Serial.println("먼저 학생증을 찍어주세요!");
      else {
        borrowedBy[u] = currentUser;
        logEvent(studentName[currentUser], umbrellaNo[u], "대여");
        saveState();
        Serial.printf("대여: %s → 우산 %d번\n", studentName[currentUser].c_str(), umbrellaNo[u]);
        currentUser = -1;
      }
    } else {
      logEvent(studentName[borrowedBy[u]], umbrellaNo[u], "반납");
      Serial.printf("반납: 우산 %d번 (%s)\n", umbrellaNo[u], studentName[borrowedBy[u]].c_str());
      borrowedBy[u] = -1;
      saveState();
      currentUser = -1;
    }
  }
  else Serial.println("등록되지 않은 카드: " + uid);

  rfid.PICC_HaltA();
  delay(800);
}
