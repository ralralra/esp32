# V2 — 릴레이 전구 무드등

기존 **220V 전구를 릴레이로 켜고 끄는** 버전입니다.
센서·앱·음성 구조는 V1과 같고, 출력 부품만 네오픽셀 → 릴레이로 바뀝니다 (색·밝기 없음).

## 구성

| 부품 | 실드 자리 | GPIO | 역할 |
|---|:---:|:---:|---|
| Wemos D1 R32 + 센서쉴드 V5 | — | — | 두뇌 + 와이파이 |
| 릴레이 KY-019형 (5V · HIGH 트리거) | **2번** | 26 | 220V 전구 켜기/끄기 |
| 인체감지 PIR (HC-SR501) | **3번** | 25 | 사람이 있는지 |
| 조도센서 | **A3** (VCC는 3V3) | 34 | 주변이 어두운지 |
| 전원 | SEL 점퍼 **꽂음** | — | 5V 1A 충전기(micro USB) + 전구 220V → [`../docs/power.md`](../docs/power.md) |

배선 → **[`wiring.md`](wiring.md)** · ⚠️ **220V 쪽은 교사가 직접**

## 동작 규칙

| 상황 | 전구 |
|---|---|
| 자동: 주변 밝기 < `DARK_LEVEL` + 움직임 | 켜짐 |
| 자동: 밝아짐 / 움직임 없이 5분 | 꺼짐 |
| 앱·음성 '켜' / '꺼' | 켜짐 / 꺼짐 (수동 모드로) · '자동'으로 복귀 |

릴레이는 켜기/끄기만 합니다 — 색·밝기는 V1 네오픽셀에서.

## 단계

| 단계 | 폴더 | 무엇을 / 확인 | 와이파이 |
|:---:|---|---|:---:|
| 1 | [`firmware/step1_sensor_test/`](firmware/step1_sensor_test/) | 조도·PIR 값 시리얼로 보며 **`DARK_LEVEL` 정하기** (V1과 같은 코드) | — |
| 2 | [`firmware/step2_relay_test/`](firmware/step2_relay_test/) | 릴레이 딸깍 → (교사) 전구 켜고 끄기 | — |
| 3 | [`firmware/step3_auto_lamp/`](firmware/step3_auto_lamp/) | **센서 받고 출력하기** — 자동 전구 완성 | — |
| 4 | [`firmware/step4_app_lamp/`](firmware/step4_app_lamp/) | 앱·음성으로 켜기·끄기·자동 + 상태 보고 | ✅ |

서버와 앱은 두 버전 공용 → [`../apps_script/`](../apps_script/README.md) · [`../app/`](../app/ai_studio_prompt.md)
