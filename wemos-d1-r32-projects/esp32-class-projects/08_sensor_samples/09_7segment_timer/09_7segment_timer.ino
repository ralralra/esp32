/*
  7세그먼트 카운트다운 타이머 — 분:초 표시 (1회차 미션의 7세그 버전!)
  01:30부터 00:00까지 줄어들고, 0이 되면 표시가 깜빡이며 내장 LED 점멸.
  연결: images/wiring_7segment.png (DIO→D6 핀(GPIO27), CLK→D7 핀(GPIO14) 예시)
*/

#include <TM1637Display.h>

#define DIO 27  // D6 핀
#define CLK 14  // D7 핀
#define LED 2   // 내장 LED

#define START_SEC 90  // 시작 시간(초) — 원하는 값으로! (90 = 1분 30초)

TM1637Display display(CLK, DIO);

void setup() {
  pinMode(LED, OUTPUT);
  display.setBrightness(7);

  for (int s = START_SEC; s >= 0; s--) {
    int mm = s / 60;                 // 분
    int ss = s % 60;                 // 초
    // 앞 두 자리=분, 뒤 두 자리=초, 가운데 콜론(:) 켜기
    display.showNumberDecEx(mm * 100 + ss, 0b01000000, true);
    delay(1000);
  }

  // TIME OVER! — 표시 깜빡 + LED 점멸 5번
  for (int i = 0; i < 5; i++) {
    display.clear();
    digitalWrite(LED, HIGH);
    delay(250);
    display.showNumberDecEx(0, 0b01000000, true);  // 00:00
    digitalWrite(LED, LOW);
    delay(250);
  }
}

void loop() { }
