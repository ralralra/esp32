# 스마트 우산 대여

**누가 어떤 우산을 빌리고 반납했는지**를 RFID로 기록하는 대여 시스템입니다.
(위치추적 없음 — 도서관 대출과 똑같은 구조)

> 🧭 **보드 안내**: 이 프로젝트는 **Wemos D1 R32** 기준입니다.
> - 처음 세팅: [`01_docs/setup_wemos_d1_r32.md`](../01_docs/setup_wemos_d1_r32.md) · 핀맵: [`01_docs/pinmap_wemos_d1_r32.md`](../01_docs/pinmap_wemos_d1_r32.md)
> - RC522의 SPI 배선은 D1 R32의 **D10~D13 자리**라 코드 그대로 호환돼요.
>
> ⚠ **기본 센서 세트에 RFID 리더가 없다면** **우회 전략**을 쓰세요:
> **RFID → 웹 QR로 대여·반납 + 초음파·무게센서로 거치 감지**
> 자세한 방법: [5회차 — 우회 전략](../02_sessions/05_team_project_planning/README.md) · [6회차 — 우산팀 핵심 시나리오](../02_sessions/06_core_implementation/README.md)

## 무엇을 만드나

```
학생증 태그 → 누가          ┐
                            ├─▶ ESP32 ─▶ "학생+우산+대여/반납" 기록 ─▶ 폰으로 현황 보기
우산 태그   → 어떤 우산      ┘
```

동작:
1. **학생증** 태그 → 누가 빌리는지 인증
2. **우산 태그** 태그 → 그 우산이 비어있으면 **대여**, 빌려간 상태면 **반납**

## 단계 안내

| 단계 | 폴더 | 무엇을 / 확인 |
|---|---|---|
| 1 | `step1_read_card` | 카드 UID 읽기 (학생증·우산태그 구분용) — 시리얼 |
| 2 | `step2_rent_return` | 학생 → 우산 순서로 태그해 대여/반납 — 시리얼 |
| 3 | `step3_save_log` | 대여 기록을 보드에 저장(SPIFFS) — 시리얼 |
| 4 | `step4_web_status` | 폰으로 대여 현황·미반납 보기 — 폰 |
| 5 | `step5_locker` | **스마트 보관함**: 서보 잠금 4 + 리드센서 4 + 구글 시트 DB — 폰·시트 |

## 준비물
- Wemos D1 R32 + USB
- **RC522 RFID 리더**
- **학생증**(MIFARE Classic) 또는 학생용 카드
- **우산용 RFID 태그** (우산 수만큼)

> 배선·라이브러리는 매점(`05_store/`)과 **완전히 동일**합니다. [`05_store/docs/wiring_and_setup.md`](../05_store/docs/wiring_and_setup.md) 참고.
> 매점을 먼저 했다면 이 프로젝트는 금방 끝나요 (같은 RFID 기술 재활용).

### 보관함(잠금장치) 확장 — 서보 4개 + 리드센서 4개
슬롯 4칸 보관함(서보 잠금 + 우산 거치 감지)을 만드는 팀은
→ 배선표: **[`docs/wiring_servo_reed.md`](docs/wiring_servo_reed.md)** (RFID와 핀 충돌 없음)
→ 코드(펌웨어 + Apps Script): **[`step5_locker/`](step5_locker/README.md)** — 구글 시트를 DB로 사용

## ⏰ 시각 안내
ESP32는 인터넷(시계)이 없어 정확한 날짜·시각 기록은 RTC 모듈이나 WiFi+NTP가 필요합니다.
이 프로젝트는 **순서(누가 무엇을 빌렸나/반납했나)** 중심으로 기록합니다.

시작 → [`step1_read_card/README.md`](step1_read_card/README.md) (매점 1단계와 동일)
