/*
  1단계 — 카드 UID 읽기 (매점 1단계와 동일)
  ----------------------------------------------------
  목표: 학생증과 우산 태그의 UID를 각각 읽어서 적어둔다.
        (2단계에서 학생표/우산표에 입력)

  라이브러리: MFRC522, 배선: store/docs/wiring_and_setup.md
*/

#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN  5
#define RST_PIN 27
MFRC522 rfid(SS_PIN, RST_PIN);

String readUID() {
  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(rfid.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();
  return uid;
}

void setup() {
  Serial.begin(115200);
  delay(300);
  SPI.begin();
  rfid.PCD_Init();
  Serial.println("\n=== 우산 대여: UID 읽기 ===");
  Serial.println("학생증과 우산 태그를 대보고 UID를 적어두세요.");
}

void loop() {
  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;
  Serial.println("UID: " + readUID());
  rfid.PICC_HaltA();
  delay(800);
}

/*
  적어두기:
    학생 김OO 학생증 UID = ____________
    학생 이OO 학생증 UID = ____________
    우산 1번 태그   UID = ____________
    우산 2번 태그   UID = ____________
*/
