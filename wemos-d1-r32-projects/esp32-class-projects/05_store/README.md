# 스마트 매점 — RFID 셀프 계산

상품에 붙은 **RFID 태그를 한 개씩 찍어서 가격을 합산**하고, 판매·재고를 자동으로 관리하는 무인 매점입니다.

> 🧭 **보드 안내**: 이 프로젝트는 **Wemos D1 R32** 기준입니다.
> - 처음 세팅: [`01_docs/setup_wemos_d1_r32.md`](../01_docs/setup_wemos_d1_r32.md) · 핀맵: [`01_docs/pinmap_wemos_d1_r32.md`](../01_docs/pinmap_wemos_d1_r32.md)
> - RC522의 SPI 배선(SS=GPIO5·SCK=18·MOSI=23·MISO=19)은 D1 R32의 **D10~D13 자리**라 코드 그대로 호환돼요.
>
> ⚠ **기본 센서 세트에 RFID 리더가 없다면** **우회 전략**을 쓰세요:
> **RFID → 웹 버튼 판매 + 무게센서로 실물 재고 감지** — 우회는 미완성이 아니라 설계 전략!
> 자세한 방법: [5회차 — 우회 전략](../02_sessions/05_team_project_planning/README.md) · [6회차 — 매점팀 핵심 시나리오](../02_sessions/06_core_implementation/README.md)

## 무엇을 만드나

```
상품 RFID 태그  ──찍기──▶  ESP32  ──▶  가격 합산 / 재고 차감 / 판매 기록
                          (RC522)         │
                                          └──▶ 폰으로 매출·인기상품·재고 보기
```

## 단계 안내

| 단계 | 폴더 | 무엇을 / 확인 |
|---|---|---|
| 1 | `step1_read_card` | RFID 카드의 고유번호(UID) 읽기 — 시리얼 |
| 2 | `step2_price_sum` | UID를 상품으로 인식해서 가격 합산 — 시리얼 |
| 3 | `step3_stock_log` | 재고 차감 + 판매 기록 저장(SPIFFS) — 시리얼 |
| 4 | `step4_web_dashboard` | 폰으로 매출·인기상품·재고 보기 — 폰 |

## 준비물
- Wemos D1 R32 + USB
- **RC522 RFID 리더** (13.56MHz)
- **MIFARE Classic 카드/태그** 여러 장 (상품 수만큼 + 결제용 1장)
  - 학생증이 MIFARE Classic이면 그대로도 가능 (step1에서 읽히는지 먼저 확인)
- (선택) 부저, OLED

## 라이브러리 설치
Arduino IDE → `Sketch` → `Include Library` → `Manage Libraries` → **"MFRC522"** 검색 → *MFRC522 by GithubCommunity* 설치

## ⚠ 중요
- RC522는 **3.3V 전용**! 5V에 연결하면 망가질 수 있어요
- 이 매점은 기획대로 **"한 번에 한 개씩 찍는 셀프 계산대"** 입니다 (장바구니 동시 인식 X)

## 문서
- [`docs/wiring_and_setup.md`](docs/wiring_and_setup.md) — RC522 배선, 라이브러리, 카드 종류

> 우산 대여(`06_umbrella/`)도 같은 RFID + 기록 구조예요. 이 프로젝트를 하면 그건 훨씬 빨라집니다.

시작 → [`step1_read_card/README.md`](step1_read_card/README.md)
