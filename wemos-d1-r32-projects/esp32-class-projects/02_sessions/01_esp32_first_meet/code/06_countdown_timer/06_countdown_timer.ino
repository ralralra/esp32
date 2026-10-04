/*
  1회차 · 종합 미션 — 카운트다운 타이머 (예시 답안)
  7세그에 10부터 0까지 1초씩 카운트다운 → 0이 되면 표시 깜빡 + 내장 LED 5번 점멸
  심화: 빛센서를 손으로 가리면(어두우면) 타이머 일시정지!
  연결: 7세그 DIO → D6 핀, CLK → D7 핀 / 빛센서 S → A3 핀(GPIO34), V→3V3
*/

#include <TM1637Display.h>

#define LIGHT 34  // 빛센서 (A3 핀)
#define DIO   27  // D6 핀
#define CLK   14  // D7 핀
#define LED   2   // 내장 LED

TM1637Display display(CLK, DIO);

void setup() {
  pinMode(LED, OUTPUT);
  display.setBrightness(7);

  for (int i = 10; i >= 0; i--) {
    // 심화: 어두우면(가리면) 밝아질 때까지 기다리기 — 뺄 거면 아래 1줄 삭제
    while (analogRead(LIGHT) < 500) delay(100);

    display.showNumberDec(i);
    delay(1000);
  }

  // TIME OVER! — 표시 깜빡 + LED 점멸 5번
  for (int k = 0; k < 5; k++) {
    display.clear();
    digitalWrite(LED, HIGH);
    delay(250);
    display.showNumberDec(0);
    digitalWrite(LED, LOW);
    delay(250);
  }
}

void loop() { }   // 다시 보려면 보드의 EN(리셋) 버튼!
