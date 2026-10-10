# IoT 무드등 — Wemos D1 R32 + 센서쉴드 × 폰 앱 · 음성제어

라탄 받침 + 주름 갓 조명에 **Wemos D1 R32(ESP32) + 아두이노 센서쉴드 V5**를 넣어,
**조도센서·인체감지센서**로 자동으로 켜고 끄고, 핸드폰 앱과 음성으로도 제어하는 IoT 조명 프로젝트입니다.

| 위에서 본 모습 | 소켓과 흰 고리 (네오픽셀 자리) | 뒤집힌 모습 |
|:---:|:---:|:---:|
| ![위](docs/images/lamp_top_view.jpg) | ![소켓](docs/images/lamp_socket_ring.jpg) | ![거꾸로](docs/images/lamp_side_view.jpg) |

## 두 가지 버전

| 항목 | [**V1 — 네오픽셀**](v1_neopixel/) | [**V2 — 릴레이 전구**](v2_relay_bulb/) |
|---|---|---|
| 조명 | 5V 네오픽셀 스트립 **16구**(RGB) | 기존 220V 전구 |
| 학습 목표 | LED의 색·밝기 제어 | 릴레이로 전원 제어 |
| 앱·음성으로 | 켜기·끄기·**색·밝기** | 켜기·끄기 |
| 센서 | 조도센서 + PIR(HC-SR501) | 동일 |
| 전원 플러그 | **1개** (2포트 USB 충전기) | **2개** (전구 220V + 5V 충전기) |
| 220V 작업 | 없음 — 학생이 전부 배선 | 있음 — 교사가 직접 |

두 버전의 보드·센서·센서 코드·서버·앱은 **같습니다.** 출력 부품(네오픽셀 ↔ 릴레이)만 다릅니다.

## 자동 동작 규칙 (두 버전 공통)

**기준 숫자 하나(`DARK_LEVEL`, 주변 밝기 %)** 를 정하면, 그보다 **어두워지면 켜지고 밝아지면 꺼집니다.**

| 상황 | 동작 |
|---|---|
| 주변 밝기 < `DARK_LEVEL` **그리고** 움직임 감지 | 켜짐 |
| 주변 밝기 ≥ `DARK_LEVEL` + 5% | 꺼짐 (밝아짐) |
| 움직임 없이 5분 | 꺼짐 (사람 없음) |
| 앱·음성 '켜'/'꺼' | 수동 모드 — '자동'으로 돌아갈 때까지 센서 무시 |

- 1단계 센서 테스트에서 교실 불을 켜고 끄며 `DARK_LEVEL`을 정하고, 3·4단계 코드에 똑같이 적습니다.
- 앱의 자동화 탭에서도 이 기준(%)과 꺼지는 시간을 바꿀 수 있습니다 (보드에 저장됨).
- PIR 없이 조도만으로 켜고 끄려면 코드의 `USE_PIR = false`.

## 코드 — 버전마다 4단계

| 단계 | V1 네오픽셀 | V2 릴레이 전구 | 와이파이 |
|:---:|---|---|:---:|
| 1 **센서 테스트** | [`step1_sensor_test`](v1_neopixel/firmware/step1_sensor_test/) | [`step1_sensor_test`](v2_relay_bulb/firmware/step1_sensor_test/) (같은 코드) | — |
| 2 **출력 테스트** | [`step2_neopixel_test`](v1_neopixel/firmware/step2_neopixel_test/) — 색·밝기 | [`step2_relay_test`](v2_relay_bulb/firmware/step2_relay_test/) — 딸깍 | — |
| 3 **센서 받고 출력하기** | [`step3_auto_mood_lamp`](v1_neopixel/firmware/step3_auto_mood_lamp/) | [`step3_auto_lamp`](v2_relay_bulb/firmware/step3_auto_lamp/) | — |
| 4 **앱·음성 연동** | [`step4_app_mood_lamp`](v1_neopixel/firmware/step4_app_mood_lamp/) | [`step4_app_lamp`](v2_relay_bulb/firmware/step4_app_lamp/) | ✅ |

3단계까지는 와이파이 없이 완성됩니다. 통신이 안 되는 날에도 무드등은 동작합니다.

## 앱 — 구글 시트 + Apps Script + AI Studio (12팀 · 시트 1개)

| 누가 | 할 일 | 문서 |
|---|---|---|
| 선생님 (한 번) | 구글 시트 1개 + [`mood_lamp.gs`](apps_script/mood_lamp.gs) 배포 → `/exec` 주소를 학생에게 | [`apps_script/README.md`](apps_script/README.md) |
| 학생 | 4단계 펌웨어 4줄(와이파이 · 주소 · **TEAM_ID**) 고쳐서 업로드 | V1 [`step4`](v1_neopixel/firmware/step4_app_mood_lamp/) · V2 [`step4`](v2_relay_bulb/firmware/step4_app_lamp/) |
| 학생 | 프롬프트의 `[웹앱 URL]`을 바꿔 AI Studio로 앱 만들기 → 설정 탭에서 내 팀 선택 | [`app/ai_studio_prompt.md`](app/ai_studio_prompt.md) |

- 팀 번호는 **TEAM01 ~ TEAM12**, 학생마다 다르게. 같은 번호를 쓰면 서로의 램프가 같이 움직입니다.
- 앱은 보드가 보고한 종류(`neopixel` / `bulb`)를 보고 **색·밝기 화면**과 **켜기·끄기 화면**을 스스로 고릅니다.
- 음성: 앱의 🎤 버튼 — "켜 줘" · "꺼 줘" · "파란색으로" · "밝기 50" → [`docs/voice_control.md`](docs/voice_control.md)

```
[폰 앱: 버튼 · 슬라이더 · 🎤]                  [Wemos D1 R32 무드등]
   │ ① 명령 보내기 (ON / OFF / COLOR …)           │ ② 2초마다 "새 명령 있나요?"
   │ ④ 현재 상태 읽기                              │ ③ 명령 실행 → 현재 상태 보고
   ▼                                            ▼
   └──────────▶  Apps Script 중계 서버  ◀────────┘
                        │
                        ▼
                구글 시트 (명령 큐 / 상태 / 기록)
```

> 왜 중계 서버인가: AI Studio 앱은 HTTPS에서 실행되고 보드는 교실 와이파이의 HTTP 주소라, 브라우저가 직접 연결을 막습니다(혼합 콘텐츠 차단).
> 폴링 구조라 반응이 **2~3초 늦을 수 있습니다.**

## 폴더 안내

| 폴더 | 내용 |
|---|---|
| [`docs/board.md`](docs/board.md) | **보드** — Wemos D1 R32 + 센서쉴드 V5 · 꽂는 자리 · SEL 점퍼 · 전원 · 배선도 |
| [`docs/parts.md`](docs/parts.md) | 부품 목록과 고른 이유 (버전별 표시) |
| [`docs/power.md`](docs/power.md) | V1은 플러그 1개, V2는 2개인 이유 · 전류 계산 |
| [`docs/voice_control.md`](docs/voice_control.md) | 음성제어 방법 (Web Speech API · Gemini · Siri 단축어) |
| [`apps_script/`](apps_script/README.md) | **중계 서버** — 시트 1개로 12팀 · `mood_lamp.gs` · 주소 약속 |
| [`app/`](app/ai_studio_prompt.md) | **AI Studio 앱 프롬프트** — 화면 시안 + 그대로 붙여넣는 프롬프트 |
| [`v1_neopixel/`](v1_neopixel/) | **V1** — 배선 · 펌웨어 4단계 |
| [`v2_relay_bulb/`](v2_relay_bulb/) | **V2** — 배선 · 펌웨어 4단계 |

## 꼭 알아두기

- ⚠️ **(V2) 220V 배선은 교사가 직접.** 학생 작업은 실드 쪽 5V 저전압까지. V1은 220V를 쓰지 않습니다.
- **SEL 점퍼** — V1은 뺌(EXT PWR 외부 5V), V2는 꽂음(보드 5V). 꽂은 채로 EXT PWR에 전원을 넣으면 합선됩니다.
- **조도센서 VCC는 3V3** (실드 Bluetooth 헤더). 실드 V(5V)에 꽂으면 출력이 3.3V를 넘습니다.
- **조도센서는 램프 빛이 닿지 않는 곳에.** 자기 빛을 보면 켜짐↔꺼짐을 반복합니다.
- **인체감지 기록은 곧 '재실 기록'** 입니다. 시트에는 명령만 남기고 PIR 값은 기록하지 않습니다.
