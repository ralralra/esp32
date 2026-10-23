# V3 · 3단계 — 네오픽셀 + 전구 동시 제어 (와이파이 없이)

> ✅ 코드: [`step3_auto_combo.ino`](step3_auto_combo.ino) — 동작 검증·ESP32 컴파일 확인 완료
> **1단계에서 정한 `DARK_LEVEL` · `DARK_IS_HIGH` 를 코드 위쪽에 똑같이 적으세요.**
> 라이브러리: Adafruit NeoPixel

## 목표
V1 네오픽셀 코드와 V2 릴레이 코드를 합쳐, **두 조명이 서로 다른 센서에 반응**하게 한다.

## 규칙 (자동 모드)

| 조명 | 켜짐 | 꺼짐 |
|---|---|---|
| **네오픽셀** | 사람이 감지되면 (조도 무관) | 움직임 없이 `NP_HOLD_MS`(60초) |
| **전구** | 주변 밝기 < `DARK_LEVEL` (사람 무관) | 밝기 ≥ `DARK_LEVEL` + 5% + `BULB_GLOW`, 켜진 지 `BULB_MIN_ON_MS`(30초) 이후 |

- 네오픽셀도 어두울 때만 반응하게 하려면 `NP_ONLY_WHEN_DARK = true`.
- 전구가 자기 빛 때문에 꺼졌다 켜졌다 하면 → [`../../wiring.md`](../../wiring.md)의 '조도센서 위치'.

## 설정값 (코드 위쪽)

```cpp
const int  DARK_LEVEL   = 30;     // 1단계에서 정한 숫자
const bool DARK_IS_HIGH = true;   // 1단계에서 확인한 방향
const int  BULB_GLOW    = 0;      // 전구를 켜면 센서가 더 밝게 읽는 %  (램프 켬 − 불 끔)
const unsigned long NP_HOLD_MS = 60UL * 1000;   // 사람이 없어진 뒤 네오픽셀 유지 시간
#define LED_COUNT 12              // V3 링은 12구 (V1 스트립은 16)
const int  MAX_BRIGHTNESS = 64;   // 전원 A안(9V) 기본값. B·C안·5V 2A 충전기면 128
const bool RELAY_ACTIVE_LOW = false;             // KY-019형. 전구만 반대로 동작하면 코드가 아니라 NC→NO
```

## 시리얼 (115200)

| 명령 | 동작 |
|---|---|
| `on` / `off` | 둘 다 켜기 / 끄기 (수동 모드로) |
| `np on` / `np off` | 네오픽셀만 |
| `bulb on` / `bulb off` | 전구만 |
| `auto` | 자동 모드로 복귀 |
| `c <색>` / `b <0~100>` | 네오픽셀 색 (warm white red … rainbow FF8800) / 밝기 % |
| `s` · `sensor` | 상태 한 줄 · 센서 값 1초마다 |

## 확인
- [ ] 밝은 교실에서 PIR 앞을 지나가면 **네오픽셀만** 켜지고, 1분 뒤 꺼짐
- [ ] 불을 끄면(어두워지면) **전구가** 켜지고, 불을 켜면 꺼짐 (사람이 없어도)
- [ ] 어둡고 사람이 있으면 둘 다 켜짐
- [ ] `bulb off` → 전구만 꺼지고 네오픽셀은 그대로 · `auto` 로 복귀
- [ ] 전구가 켜진 뒤 30초 안에 저절로 꺼지지 않음 (센서 위치 / `BULB_GLOW`)
- [ ] 보드 리셋 순간 전구가 켜지지 않음
