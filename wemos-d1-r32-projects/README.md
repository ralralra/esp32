# Wemos D1 R32 프로젝트 모음

**Wemos D1 R32**(우노 모양 ESP32 보드)로 진행한 프로젝트를 한곳에 모았습니다.
센서는 모두 **암-수 점퍼선으로 보드 헤더 핀에 직접** 연결합니다.

| 폴더 | 내용 |
|---|---|
| [`esp32-class-projects/`](esp32-class-projects/README.md) | **웹 기반 IoT 메이커 교육** — 7회차 과정 (회차별 수업자료·PPT·팀 프로젝트·세팅 가이드·핀맵) |
| [`rfid-attendance-system/`](rfid-attendance-system/README.md) | **스마트 출석체크** — RC522로 학생증(NFC) 태그 → 웹앱 실시간 출석 확인 (Apps Script 중계, 책 원고·교육지도서 포함) |

## 공통 자료

- 보드 세팅: [`esp32-class-projects/01_docs/setup_wemos_d1_r32.md`](esp32-class-projects/01_docs/setup_wemos_d1_r32.md)
- 핀맵 (보드 라벨 ↔ GPIO): [`esp32-class-projects/01_docs/pinmap_wemos_d1_r32.md`](esp32-class-projects/01_docs/pinmap_wemos_d1_r32.md)

> 두 프로젝트 모두 ESP32 WROOM-32 DevKit에서도 GPIO 번호가 같아 코드를 그대로 쓸 수 있습니다 (배선 위치만 다름).
