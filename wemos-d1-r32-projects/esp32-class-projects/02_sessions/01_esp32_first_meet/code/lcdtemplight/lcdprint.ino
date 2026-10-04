#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "DHT.h"
#define DHTTYPE DHT11 
LiquidCrystal_I2C lcd(0x27, 16, 2);  // I2C LCD 주소: 보통 0x27 (안 되면 0x3F — 00_i2c_scanner로 확인)


int light = A3;  //빛센서
int temp = 9;
DHT dht(temp, DHTTYPE);
void setup() {
  lcd.init();
  lcd.backlight();
}

void loop() {
  int v = analogRead(light);
  int h = dht.readHumidity();
  float t = dht.readTemperature();

  lcd.setCursor(0, 0);  //글자 위치(열), 줄번호
  lcd.print("now light:");
  lcd.print(v);
  lcd.setCursor(0, 1);  //글자 위치(열), 줄번호
  lcd.print("Hum:");
  lcd.print(h);
  lcd.print(" tem:");
  lcd.print(t);
}
