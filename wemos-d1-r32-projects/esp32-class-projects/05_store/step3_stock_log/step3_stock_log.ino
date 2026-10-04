/*
  3단계 — 재고 차감 + 판매 기록 저장
  ----------------------------------------------------
  목표: 상품을 팔 때마다 재고를 1개 줄이고, 판매 내역을 보드(SPIFFS)에
        저장한다. 재고가 부족하면 못 팔게 막고, 임계치 이하면 알림.

  ※ Tools → Partition Scheme → "Default 4MB with spiffs"
*/

#include <SPI.h>
#include <MFRC522.h>
#include <FS.h>
#include <SPIFFS.h>

#define SS_PIN  5
#define RST_PIN 27
MFRC522 rfid(SS_PIN, RST_PIN);

const int PRODUCT_COUNT = 3;
String productUID[PRODUCT_COUNT]   = { "A1B2C3D4", "11223344", "55667788" };
String productName[PRODUCT_COUNT]  = { "초코파이",   "콜라",      "사탕"     };
int    productPrice[PRODUCT_COUNT] = { 500,        1000,       300       };
int    productStock[PRODUCT_COUNT] = { 5,          5,          5         };  // 시작 재고
const int LOW_STOCK = 2;   // 이 이하면 보충 알림

String CHECKOUT_UID = "99AABBCC";
int total = 0;
int saleNo = 0;

String readUID() {
  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(rfid.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();
  return uid;
}

// 판매 1건을 파일에 기록 (번호,상품,가격)
void logSale(String name, int price) {
  File f = SPIFFS.open("/sales.csv", FILE_APPEND);
  if (f) {
    saleNo++;
    f.printf("%d,%s,%d\n", saleNo, name.c_str(), price);
    f.close();
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);
  SPI.begin();
  rfid.PCD_Init();
  if (!SPIFFS.begin(true)) Serial.println("SPIFFS 시작 실패!");
  Serial.println("\n=== 매점 (재고/판매기록) ===");
}

void loop() {
  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  String uid = readUID();

  if (uid == CHECKOUT_UID) {
    Serial.printf("===== 합계: %d원 =====\n\n", total);
    total = 0;
  } else {
    bool found = false;
    for (int i = 0; i < PRODUCT_COUNT; i++) {
      if (uid == productUID[i]) {
        found = true;
        if (productStock[i] <= 0) {
          Serial.printf("%s 품절! 판매 불가\n", productName[i].c_str());
          break;
        }
        productStock[i]--;                       // 재고 1개 차감
        total += productPrice[i];
        logSale(productName[i], productPrice[i]); // 판매 기록 저장
        Serial.printf("%s +%d원 (합계 %d, 남은재고 %d)\n",
                      productName[i].c_str(), productPrice[i], total, productStock[i]);
        if (productStock[i] <= LOW_STOCK)
          Serial.printf("  ⚠ %s 재고 부족(%d개) — 보충 필요\n",
                        productName[i].c_str(), productStock[i]);
        break;
      }
    }
    if (!found) Serial.println("등록되지 않은 카드: " + uid);
  }

  rfid.PICC_HaltA();
  delay(800);
}
