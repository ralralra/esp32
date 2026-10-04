/*
  첫 센서 — 빛센서 값을 시리얼과 7세그에 표시
  읽고(센서), 판단하고(코드), 보여준다(7세그) — IoT의 최소 단위 완성!
  연결: 빛센서 S → A3 핀(GPIO34), V→3V3 / 7세그 DIO → D6 핀, CLK → D7 핀
*/

#include <TM1637Display.h>

#define LIGHT 34  // 빛센서 (A3 핀 — A2=35 · A4=36 · A5=39)
#define DIO   27  // D6 핀
#define CLK   14  // D7 핀

TM1637Display display(CLK, DIO);

void setup() {
  Serial.begin(115200);
  display.setBrightness(7);
}

void loop() {
  int v = analogRead(LIGHT);   // 0(어두움) ~ 4095(밝음)
  Serial.println(v);           // 시리얼 모니터로도
  display.showNumberDec(v);    // 7세그로도!
  delay(300);
}
