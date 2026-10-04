# 스마트 출석체크 — ESP32 RFID 리더기 × 웹앱 연동

학생증(NFC, 13.56MHz)을 **ESP32 + RC522 리더기**에 태그하면,
배포된 웹앱(스마트 출석체크)에서 실시간으로 출석이 확인되는 시스템입니다.

- 보드: **ESP32 WROOM-32** (DevKit) — **Wemos D1 R32**도 같은 코드로 동작 (배선표 별도 제공)
- 리더기: **RC522 (MFRC522)** — 13.56MHz RFID/NFC ([부품 안내글](https://m.intopion.com/board/view?id=aca_ardutip&seq=1313))
- 앱: AI Studio로 만든 웹앱 (Cloud Run 배포)
- 중계: **Google Apps Script + 구글 시트** (학생명단·출석기록 저장)

## 왜 중계 서버가 필요한가

앱은 인터넷(Cloud Run)에 있고, ESP32는 교실 와이파이 안에 있어서 **서로 직접 접속할 수 없습니다.**
그래서 둘 다 접속할 수 있는 **Apps Script 웹앱**을 가운데 두고 통신합니다.

```
[스마트 출석체크 앱]                       [ESP32 + RC522]
   │ ① 출석체크 시작 (세션 생성)                │ ② 2초마다 "할 일 있나요?" 폴링
   │ ④ 1초마다 결과 폴링                       │ ③ 학생증 태그 → UID 전송
   ▼                                         ▼
   └────────▶  Apps Script 중계 서버  ◀───────┘
                      │
                      ▼
              구글 시트 (학생명단 / 출석기록)
```

## 동작 흐름 (앱 화면 기준)

| 앱 화면 | 뒤에서 일어나는 일 |
|---|---|
| **출석체크 시작** 버튼 | 앱 → 중계서버: `action=start` (교시·대상학생·리더기 DEV001·제한시간) |
| "ESP32 장치 수신 대기중" (30초 카운트다운) | ESP32가 폴링으로 세션을 발견 → **LED 깜빡임** → 카드 대기. 앱은 `action=status` 로 결과 폴링 |
| 학생이 학생증 태그 | ESP32 → 중계서버: `action=tag&uid=...` → 시트에서 학생 조회 → 출석기록 저장 (지각 판정 포함) |
| "출석이 확인되었습니다" (지각 처리) | 앱이 status 폴링으로 결과(이름·시각·출석/지각)를 받아 표시 |

**등록 모드**도 같은 구조입니다. 앱에서 학생 정보를 넣고 등록 세션을 시작하면,
ESP32에 태그된 학생증 UID가 그 학생의 카드로 **학생명단 시트에 저장**됩니다.
이후 출석체크 때 그 UID로 학생을 알아봅니다.

## 폴더 안내

| 폴더 | 내용 |
|---|---|
| [`firmware/uid_test/`](firmware/uid_test/) | **0단계** — 배선 확인 + 학생증 UID 읽기 테스트 (여기서부터 시작!) |
| [`firmware/rfid_attendance/`](firmware/rfid_attendance/) | **완성 펌웨어** — 와이파이 접속, 중계서버 폴링, 태그 전송 |
| [`apps_script/`](apps_script/) | 중계 서버 코드(`relay.gs`) + 시트 만들기·배포 가이드 |
| [`docs/wiring.md`](docs/wiring.md) | RC522 배선표 — WROOM-32 DevKit / Wemos D1 R32 (⚠️ 3.3V 필수) |
| [`docs/app_integration.md`](docs/app_integration.md) | 기존 앱의 "시뮬레이션 버튼"을 실제 장치 연동으로 바꾸는 방법 |
| [`docs/book/`](docs/book/) | **책 원고 (마크다운, 18장 + 부록)** — 슬라이드 내용을 서술형으로 풀어쓴 교재 원고 |
| [`docs/slides/`](docs/slides/) | **교육지도서 슬라이드 (PPTX, 30장)** — 개념·배선·함수별 코드 해설·트러블슈팅, 발표자 노트 포함 |
| [`docs/app_build_prompt.md`](docs/app_build_prompt.md) | **앱을 처음부터 만드는 AI Studio 프롬프트** (출석체크·학생 등록·출결 기록 전체) |

## 진행 순서

1. **배선** — [`docs/wiring.md`](docs/wiring.md) 대로 RC522 연결
2. **UID 테스트** — [`firmware/uid_test/`](firmware/uid_test/) 업로드 → 학생증을 대보고 시리얼 모니터에서 UID가 찍히는지 확인
3. **시트 + 중계서버** — [`apps_script/README.md`](apps_script/README.md) 따라 구글 시트 만들고 웹앱 배포 → 배포 URL 확보
4. **완성 펌웨어** — [`firmware/rfid_attendance/`](firmware/rfid_attendance/) 에 와이파이·배포 URL 넣고 업로드
5. **앱 연동** — [`docs/app_integration.md`](docs/app_integration.md) 대로 앱이 중계서버를 호출하게 수정

## 학생증(NFC) 관련 꼭 알아두기

- RC522는 **ISO 14443 Type A** (MIFARE 계열) 카드를 읽습니다. 대부분의 학생증·교통카드가 여기에 해당하지만,
  **Type B나 FeliCa 방식 카드는 읽지 못합니다.** → 반드시 `uid_test` 로 실제 학생증을 먼저 테스트하세요.
- **스마트폰의 NFC는 사용 불가** — 폰은 보안상 태그할 때마다 UID가 바뀌는 랜덤 UID를 쓰기 때문에
  "이 UID = 이 학생" 방식의 등록이 안 됩니다. 실물 카드(학생증)만 등록하세요.
- UID는 카드의 고유 번호일 뿐, 카드 내부 데이터를 읽거나 쓰지 않습니다 (교통카드 잔액 등과 무관).
