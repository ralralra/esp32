# 웹 기반 IoT 메이커 교육 — ESP32 × 센서 모듈

고등학교 프로젝트 수업용 실습 자료 모음입니다.
**Wemos D1 R32 (ESP32)** + **기본 센서 세트(점퍼선으로 보드 핀에 직접 연결)** 기준, **7회차(회당 4교시)** 과정이에요.

> 🔁 **보드 변경 안내**: 이 과정은 ESP32 DevKit → **Wemos D1 R32**로 보드가 바뀌었습니다.
> 우노 모양에 WiFi 내장 ESP32라 기존 코드 대부분이 호환되고, 센서는 점퍼선으로 보드 핀에 직접 연결해요.
> **세팅부터 다시**: [`01_docs/setup_wemos_d1_r32.md`](01_docs/setup_wemos_d1_r32.md) · 핀맵: [`01_docs/pinmap_wemos_d1_r32.md`](01_docs/pinmap_wemos_d1_r32.md)

## 수업 설계 — 기초를 차근차근, 그다음 팀 응용

```
[기초 다지기 — 전원 공통 실습]
1회차  LED·7세그·첫 센서        →  카운트다운 타이머
2회차  센서 입력 + HTML·CSS·JS  →  접근 경보기 · 내 리모컨 웹페이지
3회차  ESP32 웹서버 (AP 모드)   →  내 손안의 리모컨
4회차  클라우드 (시트·Apps Script·AI Studio) → 교실 환경 모니터 = 공통 뼈대 템플릿!
            │
            │  "배운 걸 응용해서, 우리 팀이 만들고 싶은 걸 만든다"
            ▼
[팀 프로젝트 — 5개 중 택1]
5회차  기획·설계 (명세서·스케치·시트 설계) + 웹앱 1차
6회차  핵심 기능 구현 (엔드 투 엔드) + 통계 + 기능 동결
7회차  교차 테스트 · 최종 발표 · 시상
```

- 1~4회차에서 **모든 팀이 같은 기초**(출력→입력→웹→서버→클라우드)를 한 층씩 쌓고,
- 4회차의 결과물(웹앱 ↔ Apps Script ↔ 시트 ↔ ESP32)이 **모든 팀 프로젝트의 공통 뼈대**가 되고,
- 5회차부터 팀별로 **하고 싶은 프로젝트를 골라** 그 뼈대에 자기 내용물을 채워요.

> 📌 **모든 자료는 '오늘 처음 해보는 사람' 기준**으로 작성되어 있습니다.
> 각 회차 폴더의 **`따라하기.md`** 가 실습 진행 문서예요 (배선 그림 → 코드 → 클릭 단위 순서 → 성공 확인 → 안 될 때).

## 회차별 수업 자료 (활동 형식)

| 회차 | 폴더 | 주제 | 산출물 |
|:---:|---|---|---|
| 1 | [`02_sessions/01_esp32_first_meet`](02_sessions/01_esp32_first_meet/README.md) | ESP32 첫 만남 — LED·7세그·첫 센서 | 카운트다운 타이머 |
| 2 | [`02_sessions/02_sensors_and_web`](02_sessions/02_sensors_and_web/README.md) | 센서 입력 + 웹 기본기 | 접근 경보기 · 리모컨 페이지 |
| 3 | [`02_sessions/03_esp32_webserver`](02_sessions/03_esp32_webserver/README.md) | ESP32 웹서버 — 폰으로 보드 제어 | 내 손안의 리모컨 |
| 4 | [`02_sessions/04_cloud_expansion`](02_sessions/04_cloud_expansion/README.md) | 클라우드 확장 — 시트·Apps Script·AI Studio | 교실 환경 모니터 (뼈대 템플릿) |
| 5 | [`02_sessions/05_team_project_planning`](02_sessions/05_team_project_planning/README.md) | 팀 프로젝트 기획·설계 | 명세서·스케치·시트 설계·웹앱 1차 |
| 6 | [`02_sessions/06_core_implementation`](02_sessions/06_core_implementation/README.md) | 핵심 기능 구현 + 통계 | 엔드 투 엔드 시스템 |
| 7 | [`02_sessions/07_final_presentation`](02_sessions/07_final_presentation/README.md) | 테스트·발표·시연 | 최종 발표 |

> 원본 참고자료: 각 회차 PPT(`○회차_*.pptx`)는 클로드 프로젝트에서 만든 수업 설계 원본이며,
> 위 [`02_sessions/`](02_sessions) 폴더가 그 내용을 깃허브 활동 형식으로 옮긴 것입니다.

## 팀 프로젝트 4가지 (5~7회차에서 팀별 택1)

| 폴더 | 프로젝트 | 난이도 | 핵심 센서 | 우회 전략 |
|---|---|:---:|---|---|
| [`03_plant/`](03_plant/README.md) | 스마트 화분 | ★☆☆ | 토양습도·빛·DHT·물온도 | 펌프 → 급수 알림 |
| [`05_store/`](05_store/README.md) | 스마트 매점 | ★★☆ | 7세그·무게·진동 | RFID → 웹 버튼+무게 감지 |
| [`06_umbrella/`](06_umbrella/README.md) | 스마트 우산 대여 | ★★☆ | 초음파·무게·7세그 | RFID → 웹 QR+거치 감지 |
| [`07_kiosk/`](07_kiosk/README.md) | 창업 키오스크 | ★★☆ | 없음 — 앱 + DB(시트)만 | 실결제 → 가상 결제 연출 |

- 각 프로젝트 폴더는 **단계별(step) 실습** 구조 — 한 단계씩 확인하며 진행
- **우회는 미완성이 아니라 설계 전략** — 발표에서 가점 요소예요! ([7회차 평가 기준](02_sessions/07_final_presentation/README.md))
- 원본 기획: [`PROJECT_SPEC.md`](PROJECT_SPEC.md)

## 심화 과정 (7회차 이후 보너스)

| 폴더 | 주제 | 내용 |
|---|---|---|
| [`09_sheet_chatbot/`](09_sheet_chatbot/README.md) | 🤖 시트 기록을 답해주는 AI 챗봇 | 4회차 시트에 **OpenAI Function Calling**을 연결 — "교실 온도 어때?"에 실제 데이터로 답하는 챗봇 (파이썬 + gradio). 도전과제: 말로 ESP32 제어하기 |

## 공통 준비물

- **Wemos D1 R32** 보드 — 우노 모양 + WiFi 내장 ESP32 (⚠ CH340 드라이버 필요)
- **기본 센서 세트** + **암-수 점퍼선** + **미니 브레드보드**(3V3·GND 나눠 쓰기) — 센서를 D1 R32 헤더 핀에 직접 연결 (G→GND, V→3V3, S→신호 핀)
- USB 케이블 (데이터 전송용 — 충전 전용 X)
- Arduino IDE + esp32 보드 패키지 + 블록코딩 도구
- 팀당 노트북 1대 이상 + 스마트폰

> 🔑 딱 하나만 기억: **아날로그 센서는 A2~A5 핀에!** (WiFi 충돌 회피)
> 이유와 전체 핀맵 → [`01_docs/pinmap_wemos_d1_r32.md`](01_docs/pinmap_wemos_d1_r32.md)

## 시작하기

1. 보드 세팅이 처음이라면 → [`01_docs/setup_wemos_d1_r32.md`](01_docs/setup_wemos_d1_r32.md)
0. 센서별 연결 그림 + 바로 쓰는 샘플 코드 → [`08_sensor_samples/`](08_sensor_samples/README.md)
2. 수업 진행은 → [`02_sessions/01_esp32_first_meet`](02_sessions/01_esp32_first_meet/README.md) 부터 차례대로
3. 팀 프로젝트 단계(5회차~)에서 → 위 표에서 팀 프로젝트 폴더 선택
4. 웹앱 화면은 AI Studio(New App)로 생성 → [`01_docs/ai_studio_webapp_guide.md`](01_docs/ai_studio_webapp_guide.md)
   (프롬프트 샘플 + 팀별 교체 가이드 + **모두의 폰에 설치하는 PWA 방법**까지)

## 폴더 구성

```
esp32-class-projects/
├── 01_docs/               ← 보드 세팅·핀맵·AI Studio 웹앱 가이드·트러블슈팅 (공통)
├── 02_sessions/           ← 회차별 수업 자료 (1~7회차, 활동 형식)
├── 03_plant/ 05_store/ 06_umbrella/ 07_kiosk/   ← 팀 프로젝트 4종 (권장 진행 순서, 단계별 실습)
├── 08_sensor_samples/     ← 센서별 연결 그림 + 샘플 코드 (빛·소리·토양습도·진동·홀·DHT·초음파·7세그 …)
├── images/                ← 보드 사진·핀맵 이미지
├── PROJECT_SPEC.md        ← 원본 기획 명세
└── ○회차_*.pptx           ← 수업 설계 원본 PPT (참고용)
```
