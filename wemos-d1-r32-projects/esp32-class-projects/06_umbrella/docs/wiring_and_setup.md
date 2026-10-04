# 우산 대여 배선 & 설정

RFID 리더(RC522) 배선·라이브러리는 **매점 프로젝트와 완전히 동일**합니다.

→ **[`../../05_store/docs/wiring_and_setup.md`](../../05_store/docs/wiring_and_setup.md)** 를 그대로 보세요.

요약:
| RC522 | ESP32 |
|---|---|
| SDA(SS) | GPIO5 |
| SCK | GPIO18 |
| MOSI | GPIO23 |
| MISO | GPIO19 |
| RST | GPIO27 |
| **3.3V** | **3.3V** (5V 금지!) |
| GND | GND |

라이브러리: **MFRC522 by GithubCommunity**
보드: ESP32 Dev Module / Partition: Default 4MB with spiffs (3·4단계)

## 학생증 vs 우산 태그 구분
이 프로젝트는 두 종류의 카드를 씁니다:
- **학생증/학생 카드** — 누가 빌리는지
- **우산 태그** — 어떤 우산인지

코드에는 두 개의 표(학생표, 우산표)가 있고, 찍힌 UID가 어느 표에 있는지로 구분합니다.
→ 1단계에서 학생증 UID와 우산 태그 UID를 각각 적어두세요.
