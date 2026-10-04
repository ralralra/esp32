# 매점 배선 & 설정 (RC522 RFID)

## RC522 ↔ ESP32 배선 (SPI)

| RC522 핀 | ESP32 |
|---|---|
| SDA (SS) | GPIO5 |
| SCK | GPIO18 |
| MOSI | GPIO23 |
| MISO | GPIO19 |
| RST | GPIO27 |
| **3.3V** | **3.3V** ← ⚠ 절대 5V 아님! |
| GND | GND |
| IRQ | 연결 안 함 |

> RC522는 **3.3V 전용**입니다. 5V에 꽂으면 모듈이 손상될 수 있어요.

## 라이브러리

Arduino IDE → `Sketch` → `Include Library` → `Manage Libraries`
→ **"MFRC522"** 검색 → **MFRC522 by GithubCommunity** 설치

## 보드 설정
- Board: **ESP32 Dev Module**
- Upload Speed: 115200
- (4단계만) 와이파이 — 추가 설정 없음

## 카드/태그 종류

| 종류 | RC522로 읽힘? |
|---|---|
| MIFARE Classic 1K (가장 흔한 흰 카드/키링) | ✅ |
| 학생증 (MIFARE Classic 계열이면) | ✅ — step1에서 먼저 확인 |
| 학생증 (DESFire/FeliCa) | ❌ → PN532 리더 필요 |
| 교통카드(티머니 등) | 일부만, 권장 안 함 |

> 가장 안전한 건 **MIFARE Classic 카드/키링을 따로 구입**하는 것 (개당 수백 원, 100% 동작).
> 상품마다 1장 + "결제(checkout)"용 1장이 필요합니다.

## UID 란?
모든 RFID 카드에는 공장에서 찍힌 **고유번호(UID)** 가 있어요. 이걸 상품 ID로 씁니다.
- step1에서 각 카드의 UID를 읽어 적어두고
- step2 코드의 상품 표에 그 UID를 넣습니다

## 자주 막히는 곳
| 증상 | 해결 |
|---|---|
| 카드 대도 반응 없음 | 배선 확인(특히 SDA=5, RST=27), 3.3V 연결, 라이브러리 설치 |
| `MIFARE_Read() failed` | 카드가 MIFARE Classic 아님 → 다른 카드/PN532 |
| 가끔만 읽힘 | 카드를 리더에 가까이·평평하게, 점퍼선 짧게 |
| ESP32 리셋 반복 | RC522를 5V에 연결했는지 확인(3.3V여야 함) |
