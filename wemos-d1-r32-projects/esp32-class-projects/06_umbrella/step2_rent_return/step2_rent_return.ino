/*
  2단계 — 대여 / 반납 처리
  ----------------------------------------------------
  사용법:
    1) 먼저 [학생증] 을 찍는다  → "누가" 가 정해짐
    2) 이어서 [우산 태그] 를 찍는다
         - 그 우산이 비어있으면 → 대여 (그 학생이 빌림)
         - 그 우산이 대여중이면 → 반납 처리

  1단계에서 적은 UID를 학생표/우산표에 넣으세요.
*/

#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN  5
#define RST_PIN 27
MFRC522 rfid(SS_PIN, RST_PIN);

// ── 학생표 ──
const int STUDENT_COUNT = 2;
String studentUID[STUDENT_COUNT]  = { "AAAA1111", "BBBB2222" };
String studentName[STUDENT_COUNT] = { "김OO",      "이OO"      };

// ── 우산표 ──
const int UMBRELLA_COUNT = 3;
String umbrellaUID[UMBRELLA_COUNT] = { "CCCC3333", "DDDD4444", "EEEE5555" };
int    umbrellaNo[UMBRELLA_COUNT]  = { 1,          2,          3          };
int    borrowedBy[UMBRELLA_COUNT]  = { -1,         -1,         -1         };  // -1=비어있음, 아니면 학생 index

int currentUser = -1;   // 방금 찍은 학생 (없으면 -1)

String readUID() {
  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(rfid.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();
  return uid;
}

int findStudent(String uid) {
  for (int i = 0; i < STUDENT_COUNT; i++) if (uid == studentUID[i]) return i;
  return -1;
}
int findUmbrella(String uid) {
  for (int i = 0; i < UMBRELLA_COUNT; i++) if (uid == umbrellaUID[i]) return i;
  return -1;
}

void setup() {
  Serial.begin(115200);
  delay(300);
  SPI.begin();
  rfid.PCD_Init();
  Serial.println("\n=== 우산 대여/반납 ===");
  Serial.println("[학생증] → [우산태그] 순서로 찍으세요.");
}

void loop() {
  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;
  String uid = readUID();

  int s = findStudent(uid);
  int u = findUmbrella(uid);

  if (s >= 0) {
    // 학생증을 찍음 → 현재 사용자 지정
    currentUser = s;
    Serial.printf("학생 인증: %s — 이제 우산을 찍으세요\n", studentName[s].c_str());
  }
  else if (u >= 0) {
    // 우산을 찍음
    if (borrowedBy[u] == -1) {
      // 비어있음 → 대여
      if (currentUser == -1) {
        Serial.println("먼저 학생증을 찍어주세요!");
      } else {
        borrowedBy[u] = currentUser;
        Serial.printf("대여: %s → 우산 %d번\n", studentName[currentUser].c_str(), umbrellaNo[u]);
        currentUser = -1;   // 한 건 끝
      }
    } else {
      // 대여중 → 반납
      Serial.printf("반납: 우산 %d번 (빌렸던 사람: %s)\n",
                    umbrellaNo[u], studentName[borrowedBy[u]].c_str());
      borrowedBy[u] = -1;
      currentUser = -1;
    }
  }
  else {
    Serial.println("등록되지 않은 카드: " + uid);
  }

  rfid.PICC_HaltA();
  delay(800);
}
