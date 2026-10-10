# V1 — 네오픽셀 무드등

5V **네오픽셀 스트립(WS2812B RGB, 16구로 잘라 소켓 둘레에 감음)** 이 조명 전체를 맡는 버전입니다.
조도·인체감지 센서로 자동으로 켜고 끄고, 앱·음성으로 **켜기·끄기·색·밝기**를 바꿉니다. 220V를 쓰지 않아 학생이 전부 배선합니다.

## 구성

| 부품 | 실드 자리 | GPIO | 역할 |
|---|:---:|:---:|---|
| Wemos D1 R32 + 센서쉴드 V5 | — | — | 두뇌 + 와이파이 |
| 네오픽셀 스트립 16구 (RGB) | **6번** | 27 | 조명 — 색·밝기 |
| 인체감지 PIR (HC-SR501) | **3번** | 25 | 사람이 있는지 |
| 조도센서 | **A3** (VCC는 3V3) | 34 | 주변이 어두운지 |
| 전원 | SEL 점퍼 **뺌** | — | 2포트 USB 충전기 → 보드 micro USB + 실드 EXT PWR → [`../docs/power.md`](../docs/power.md) |

배선 → **[`wiring.md`](wiring.md)**

## 동작 규칙

| 상황 | 네오픽셀 |
|---|---|
| 자동: 주변 밝기 < `DARK_LEVEL` + 움직임 | 마지막 색으로 **서서히** 켜짐 |
| 자동: 밝아짐 / 움직임 없이 5분 | 서서히 꺼짐 |
| 앱·음성 '켜' / '꺼' | 켜짐 / 꺼짐 (수동 모드로) · '자동'으로 복귀 |
| 색 · 밝기 슬라이더 · "파란색으로" · "밝게" | 색·밝기 변경 |

## 단계

| 단계 | 폴더 | 무엇을 / 확인 | 와이파이 |
|:---:|---|---|:---:|
| 1 | [`firmware/step1_sensor_test/`](firmware/step1_sensor_test/) | 조도·PIR 값 시리얼로 보며 **`DARK_LEVEL` 정하기** | — |
| 2 | [`firmware/step2_neopixel_test/`](firmware/step2_neopixel_test/) | 링 색 순서·밝기·웜화이트 확인 | — |
| 3 | [`firmware/step3_auto_mood_lamp/`](firmware/step3_auto_mood_lamp/) | **센서 받고 출력하기** — 자동 무드등 완성 | — |
| 4 | [`firmware/step4_app_mood_lamp/`](firmware/step4_app_mood_lamp/) | 앱·음성으로 색·밝기까지 제어 + 상태 보고 | ✅ |

서버와 앱은 두 버전 공용 → [`../apps_script/`](../apps_script/README.md) · [`../app/`](../app/ai_studio_prompt.md)
