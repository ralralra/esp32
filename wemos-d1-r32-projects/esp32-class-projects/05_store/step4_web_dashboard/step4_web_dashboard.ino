/*
  4단계 — 매점 웹 대시보드 (완성본)
  ----------------------------------------------------
  목표: 폰으로 접속하면 재고 현황, 총 매출, 인기상품을 한눈에 본다.
        RFID 셀프 계산(1~3단계) + 웹 조회(출석에서 배운 웹서버)를 합친 완성본.

  선생님 화면: http://192.168.4.1/      (재고/매출/인기상품)
  ※ Partition Scheme: Default 4MB with spiffs
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

const int PRODUCT_COUNT = 3;
String productUID[PRODUCT_COUNT]   = { "A1B2C3D4", "11223344", "55667788" };
String productName[PRODUCT_COUNT]  = { "초코파이",   "콜라",      "사탕"     };
int    productPrice[PRODUCT_COUNT] = { 500,        1000,       300       };
int    productStock[PRODUCT_COUNT] = { 5,          5,          5         };
String CHECKOUT_UID = "99AABBCC";

const char* AP_SSID = "ESP32-Store";
const char* AP_PASS = "store1234";

WebServer server(80);
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

void logSale(String name, int price) {
  File f = SPIFFS.open("/sales.csv", FILE_APPEND);
  if (f) { saleNo++; f.printf("%d,%s,%d\n", saleNo, name.c_str(), price); f.close(); }
}

// 판매기록에서 상품별 판매 수와 총 매출 집계
void readStats(int soldCount[], int &grandTotal) {
  for (int i = 0; i < PRODUCT_COUNT; i++) soldCount[i] = 0;
  grandTotal = 0;
  File f = SPIFFS.open("/sales.csv", FILE_READ);
  if (!f) return;
  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;
    // 형식: 번호,상품,가격
    int c1 = line.indexOf(',');
    int c2 = line.indexOf(',', c1 + 1);
    if (c1 < 0 || c2 < 0) continue;
    String name = line.substring(c1 + 1, c2);
    int price = line.substring(c2 + 1).toInt();
    grandTotal += price;
    for (int i = 0; i < PRODUCT_COUNT; i++)
      if (name == productName[i]) soldCount[i]++;
  }
  f.close();
}

void handleRoot() {
  int sold[PRODUCT_COUNT]; int grand;
  readStats(sold, grand);

  String html = "<!doctype html><html lang='ko'><head><meta charset='utf-8'>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<meta http-equiv='refresh' content='3'>";
  html += "<title>매점 대시보드</title><style>";
  html += "body{font-family:sans-serif;padding:1rem;max-width:480px;margin:0 auto}";
  html += "table{border-collapse:collapse;width:100%}th,td{border:1px solid #ccc;padding:8px;text-align:center}";
  html += "th{background:#2563eb;color:#fff}.low{color:#d97706;font-weight:bold}";
  html += "</style></head><body>";
  html += "<h2>🏪 매점 대시보드</h2>";
  html += "<p>총 매출: <b>" + String(grand) + "원</b></p>";
  html += "<table><tr><th>상품</th><th>가격</th><th>남은재고</th><th>판매수</th></tr>";
  for (int i = 0; i < PRODUCT_COUNT; i++) {
    String stockCell = (productStock[i] <= 2)
      ? "<td class='low'>" + String(productStock[i]) + " ⚠</td>"
      : "<td>" + String(productStock[i]) + "</td>";
    html += "<tr><td>" + productName[i] + "</td><td>" + String(productPrice[i]) + "</td>"
          + stockCell + "<td>" + String(sold[i]) + "</td></tr>";
  }
  html += "</table><p style='color:#888;font-size:.9rem'>3초마다 자동 새로고침</p>";
  html += "</body></html>";
  server.send(200, "text/html; charset=utf-8", html);
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  Serial.begin(115200);
  delay(300);
  SPI.begin();
  rfid.PCD_Init();
  if (!SPIFFS.begin(true)) Serial.println("SPIFFS 실패!");

  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.print("대시보드: http://");
  Serial.println(WiFi.softAPIP());
  server.on("/", handleRoot);
  server.begin();
  Serial.println("서버 시작됨!");
}

void loop() {
  server.handleClient();

  // RFID 셀프 계산 (1~3단계 로직)
  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;
  String uid = readUID();

  if (uid == CHECKOUT_UID) {
    Serial.printf("===== 합계: %d원 =====\n", total);
    total = 0;
  } else {
    for (int i = 0; i < PRODUCT_COUNT; i++) {
      if (uid == productUID[i]) {
        if (productStock[i] <= 0) { Serial.printf("%s 품절!\n", productName[i].c_str()); break; }
        productStock[i]--;
        total += productPrice[i];
        logSale(productName[i], productPrice[i]);
        Serial.printf("%s +%d (합계 %d, 재고 %d)\n",
                      productName[i].c_str(), productPrice[i], total, productStock[i]);
        break;
      }
    }
  }
  rfid.PICC_HaltA();
  delay(800);
}
