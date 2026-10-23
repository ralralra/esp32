# V3 — 네오픽셀 + 릴레이 전구 (둘 다)

V1의 네오픽셀과 V2의 220V 전구를 **한 보드에 같이** 달고, 자동 모드에서 **서로 다른 센서**에 반응하게 한 버전입니다.

| 조명 | 자동 모드에서 | 수동 |
|---|---|---|
| 네오픽셀 12구 (색·밝기) | **사람이 오면** 켜지고, 1분 동안 없으면 꺼짐 | `np on` / `np off` |
| 220V 전구 | **어두워지면** 켜지고, 밝아지면 꺼짐 | `bulb on` / `bulb off` |

## 구성

| 부품 | 실드 자리 | GPIO | 역할 |
|---|:---:|:---:|---|
| Wemos D1 R32 + 센서쉴드 V5 | — | — | 두뇌 |
| 네오픽셀 **12구** | **6번** | 27 | 색·밝기 조명 (사람 감지) · 코드 `LED_COUNT 12` |
| 릴레이 KY-019형 (5V · HIGH 트리거) | **2번** | 26 | 220V 전구 (어두움) · 전구는 **COM–NO** |
| 인체감지 PIR (HC-SR501) | **3번** | 25 | 사람이 있는지 |
| 조도센서 | **A3** (VCC는 3V3) | 34 | 주변이 어두운지 |
| 전원 | A안 SEL **꽂음** / B·C안 SEL **뺌** | — | A안 DC 잭에 9V 건전지 · B안 9V + 벅 컨버터 → EXT PWR · C안 220V 분기 → 5V 모듈(플러그 1개) · 전구 220V |

![V3 배선도](../docs/images/wiring_v3_combo.png)

배선과 전원 계산(A안 DC 잭만 / B안 벅 컨버터 두 갈래 / C안 220V 분기 → 5V 모듈) → **[`wiring.md`](wiring.md)** · ⚠️ **220V 쪽은 교사가 직접** · 9V 건전지는 네오픽셀 상한을 64로 (코드 기본값)

## 단계

1·2단계는 V1·V2 것을 그대로 씁니다 (센서 테스트는 같은 코드, 출력 테스트는 각각).

| 단계 | 폴더 | 무엇을 / 확인 |
|:---:|---|---|
| 1 | [`../v1_neopixel/firmware/step1_sensor_test/`](../v1_neopixel/firmware/step1_sensor_test/) | 조도·PIR 값 보며 `DARK_LEVEL` 정하기 |
| 2 | [`../v1_neopixel/firmware/step2_neopixel_test/`](../v1_neopixel/firmware/step2_neopixel_test/) · [`../v2_relay_bulb/firmware/step2_relay_test/`](../v2_relay_bulb/firmware/step2_relay_test/) | 네오픽셀 무지개 · 릴레이 딸깍 (한 보드에 둘 다 꽂은 채 차례로 업로드) |
| 3 | [`firmware/step3_auto_combo/`](firmware/step3_auto_combo/) | **사람 → 네오픽셀 · 어두움 → 전구** 자동 + 시리얼 수동 제어 |
| 5 | [`firmware/step5_app_mqtt/`](firmware/step5_app_mqtt/) | 앱·음성으로 네오픽셀·전구 **따로** 켜기·끄기, 색·밝기, 자동 — MQTT 0.1~0.5초 (와이파이 → 전원 B·C안) |

4단계(구글시트 폴링)는 V3에 없습니다. 앱은 V1·V2와 같은 MQTT판 프롬프트를 쓰고, 보드가 `type:"combo"`를 보고하면 스위치가 둘인 화면이 나옵니다 → [`../app/ai_studio_prompt_mqtt.md`](../app/ai_studio_prompt_mqtt.md)
