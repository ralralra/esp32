# 중계 서버 만들기 (Apps Script + 구글 시트)

앱과 ESP32가 만나는 곳입니다. 학생명단·출석기록 **데이터베이스**도 이 시트에 저장됩니다.
10분이면 됩니다.

## 구글 시트 구성 (자동 생성됨)

| 시트 | 열 구성 | 용도 |
|---|---|---|
| **학생명단** | UID · 이름 · 학년 · 반 · 번호 · 등록일시 | 학생증 등록 DB (등록 모드에서 자동 추가) |
| **출석기록** | 날짜 · 시각 · 교시 · UID · 이름 · 상태 · 리더기 | 출석/지각 기록 (태그할 때마다 자동 추가) |
| **교시설정** | 교시 · 시작시각 · 지각기준 | **지각 판정 기준** — 기본 1~7교시가 채워지며, 학교 시간표에 맞게 시트에서 직접 수정 |
| **설정** | 항목 · 값 | **교사 비밀번호**(기본 `1234`)와 **비밀번호보호**(기본 `OFF`) — 시트에서 직접 수정, 즉시 반영 |

네 시트 모두 **첫 요청이 들어올 때 한 번에 자동 생성**되므로 미리 만들 필요 없습니다.
(배포 후 `배포URL?action=periods` 를 한 번만 열어도 네 탭이 모두 생깁니다)
지각 판정: 태그 시각이 해당 교시의 `지각기준`(HH:MM)을 넘으면 상태가 `지각`으로 기록됩니다.
같은 날 같은 교시에 같은 학생이 또 태그하면 중복 기록하지 않고 `already`로 응답합니다.

## 1. 시트 만들고 스크립트 붙이기

1. [sheets.new](https://sheets.new) → 새 시트 생성, 이름은 예: **스마트출석체크**
2. 메뉴 **확장 프로그램 → Apps Script**
3. 기본 `Code.gs` 내용을 전부 지우고 [`relay.gs`](relay.gs) 내용을 붙여넣기 → 저장

## 2. 웹앱으로 배포

1. 우측 상단 **배포 → 새 배포**
2. 유형: **웹 앱**
   - 실행 계정: **나**
   - 액세스 권한: **모든 사용자** ← 중요! (ESP32는 구글 로그인을 못 하므로)
3. **배포** → 권한 승인 → **웹 앱 URL 복사** (`https://script.google.com/macros/s/.../exec`)

이 URL이 **중계서버 주소**입니다. 두 곳에 넣습니다:
- ESP32 펌웨어의 `RELAY_URL`
- 앱 연동 코드의 `RELAY_URL` ([docs/app_integration.md](../docs/app_integration.md))

> ⚠️ 코드를 수정한 뒤에는 **배포 → 배포 관리 → 연필 아이콘 → 버전: 새 버전 → 배포** 를 해야 반영됩니다.
> "새 배포"를 또 만들면 URL이 바뀌어 버리니 주의!

## 3. 브라우저로 동작 테스트 (ESP32 없이)

배포 URL을 `URL` 이라고 할 때, 브라우저 주소창에 차례로 넣어보세요.

| 순서 | 주소 | 기대 응답 |
|---|---|---|
| ① 등록 세션 시작 | `URL?action=start&mode=register&device=DEV001&name=김세이프&grade=1&klass=3&number=7` | `{"ok":true,"timeout":30}` |
| ② ESP32인 척 폴링 | `URL?action=poll&device=DEV001` | `REGISTER` |
| ③ ESP32인 척 태그 | `URL?action=tag&device=DEV001&uid=A1B2C3D4` | `OK_REG,김세이프` |
| ④ 앱인 척 결과 조회 | `URL?action=status&device=DEV001` | `{"state":"done",...}` |
| ⑤ 출석 세션 시작 | `URL?action=start&mode=attend&device=DEV001&name=김세이프&period=1&lateAfter=09:00` | `{"ok":true,"timeout":30}` |
| ⑥ 다시 태그 | `URL?action=tag&device=DEV001&uid=A1B2C3D4` | `OK,김세이프,지각,21:22:58` |

⑥까지 되면 시트에 **학생명단 1줄, 출석기록 1줄**이 생겨 있어야 합니다.

## 통신 규칙 정리

### ESP32용 (한 줄 텍스트 응답)

| 요청 | 응답 | 뜻 |
|---|---|---|
| `?action=poll&device=` | `IDLE` / `ATTEND` / `REGISTER` | 대기 / 출석 세션 중 / 등록 세션 중 |
| `?action=tag&device=&uid=` | `OK,이름,상태,시각` | 출석 기록 완료 |
| | `OK_REG,이름` | 카드 등록 완료 |
| | `UNKNOWN` | 미등록 카드 |
| | `DUP,이름` | 이미 등록된 카드 (등록 모드) |
| | `MISMATCH,이름` | 대상 학생이 아닌 카드 |
| | `ALREADY,이름,시각` | 같은 교시에 이미 출석한 학생 |
| | `IDLE` | 세션이 이미 끝났거나 만료됨 |

### 앱용 (JSON 응답)

| 요청 | 설명 |
|---|---|
| `?action=start&mode=attend&device=&period=&name=&lateAfter=HH:MM&timeout=30` | 출석 세션 시작. `name`을 비우면 아무 학생이나 태그 가능 |
| `?action=start&mode=register&device=&name=&grade=&klass=&number=` | 등록 세션 시작 |
| `?action=status&device=` | `{state: none/waiting/done, mode, result}` — 1초 간격 폴링 권장 |
| `?action=cancel&device=` | 세션 취소 |
| `?action=students` | 등록된 학생 목록 `{students:[{uid,name,grade,klass,number,registeredAt}]}` |
| `?action=records&date=&period=` | 출석기록 조회 (date 기본 오늘) `{records:[{date,time,period,uid,name,status,device}]}` |
| `?action=periods` | 교시설정 목록 `{periods:[{period,start,lateAfter}]}` |
| `?action=attended&period=&date=` | **출석 현황** — 등록 학생 전체에 출석 여부 표시 `{total, attended, late, absent, students:[{name,checked,status,time}]}` |
| `?action=login&pin=` | 교사 비밀번호 확인 `{ok, protected}` (pin 없이 호출하면 보호 여부만 반환) |

- 세션은 리더기 1대당 1개이며 `timeout`(기본 30초) 후 자동 만료됩니다.
  등록(`mode=register`) 세션은 카드 준비 시간이 필요해 **최소 120초**를 보장합니다
  (앱이 더 짧은 값을 보내도 서버가 120초로 늘립니다. 실제 적용값은 응답의 `timeout`).
- **비밀번호 보호**: `설정` 시트의 `비밀번호보호`를 `ON`으로 두면 `start`·`cancel`·`students`·`records`에 `&pin=` 이 필요합니다. ESP32가 쓰는 `poll`·`tag`와 `periods`·`attended`·`status`는 잠기지 않습니다.
- 지각 기준: `start`에 `lateAfter`(HH:MM)를 직접 넘기거나, 생략하면 `period`로 **교시설정 시트**에서 자동으로 찾습니다.
