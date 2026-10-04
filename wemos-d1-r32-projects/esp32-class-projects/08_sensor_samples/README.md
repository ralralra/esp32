# 센서 샘플 모음 — 연결 그림 + 바로 쓰는 코드

> 센서를 **점퍼선으로 D1 R32 핀에 직접 연결** → **코드 업로드 → 시리얼(115200) 관찰**.
> 핀 번호는 **연결한 핀의 GPIO**로 수정하세요 — [핀맵 문서](../01_docs/pinmap_wemos_d1_r32.md) · [보드 핀맵 사진](<../images/wemosD1R32 pinmap.jpg>)

## 연결 그림

| 그림 | 대상 센서 | 연결하는 곳 |
|---|---|---|
| **[아날로그 센서](../images/wiring_analog_sensor.png)** | 빛 · 소리 · 토양습도 | G→GND, V→**3V3**, S→**A2~A5 핀** (35·34·36·39) |
| **[디지털 센서](../images/wiring_digital_sensor.png)** | 진동 · 홀(자석) · 온습도(DHT) | G→GND, V→3V3, S→디지털 핀 아무 곳 (예: D6 = GPIO27) |
| **[초음파](../images/wiring_ultrasonic.png)** | 초음파 거리 (TRIG·ECHO) | TRIG→D6(GPIO27), ECHO→D7(GPIO14) — 5V 모듈이면 ECHO에 분압 저항 |
| **[7세그먼트](../images/wiring_7segment.png)** | TM1637 숫자표시기 (DIO·CLK) | DIO→D6(GPIO27), CLK→D7(GPIO14), VCC→3V3 |
| LCD — 옵션 | I2C LCD (16×2) — **본 과정 미사용** | SDA·SCL 연결 (아래 참고) |

## 샘플 코드

| # | 스케치 | 센서 | 핵심 함수 | 라이브러리 |
|:---:|---|---|---|---|
| 00 | [`00_i2c_scanner`](00_i2c_scanner/00_i2c_scanner.ino) | **I2C 스캐너** — I2C 장치 주소·연결 진단 | `Wire` | — |
| 01 | [`01_light`](01_light/01_light.ino) | 빛 | `analogRead` | — |
| 02 | [`02_sound`](02_sound/02_sound.ino) | 소리 | `analogRead` | — |
| 03 | [`03_soil`](03_soil/03_soil.ino) | 토양습도 (+% 변환) | `analogRead` + `map` | — |
| 04 | [`04_vibration`](04_vibration/04_vibration.ino) | 진동 | `digitalRead` | — |
| 05 | [`05_hall`](05_hall/05_hall.ino) | 홀(자석) — 문 열림 | `digitalRead` | — |
| 06 | [`06_dht`](06_dht/06_dht.ino) | 온습도 | `readTemperature/Humidity` | DHT sensor library |
| 07 | [`07_ultrasonic`](07_ultrasonic/07_ultrasonic.ino) | 초음파 거리 | `pulseIn` | — |
| 08 | [`08_7segment`](08_7segment/08_7segment.ino) | 7세그먼트 기본 | `showNumberDecEx` | **TM1637** (Avishay Orpaz) |
| 09 | [`09_7segment_timer`](09_7segment_timer/09_7segment_timer.ino) | 7세그 **카운트다운 타이머** | 분:초 표시 + 깜빡임 | TM1637 |
| 10 | [`10_lcd_countdown`](10_lcd_countdown/10_lcd_countdown.ino) | (옵션) LCD 카운트다운 — **본 과정 미사용**, 수업용은 06·09! | `lcd.print` | LiquidCrystal I2C |
| 11 | [`11_openmeteo_weather`](11_openmeteo_weather/11_openmeteo_weather.ino) | **바깥 날씨 API** — 안팎 온도 비교 (4회차 심화) | HTTPS GET + JSON | TM1637 · DHT |

## 공통 규칙 4가지

1. **아날로그 센서는 A2~A5 핀에** — A0·A1 핀(GPIO 2·4)은 WiFi를 켜면 아날로그를 못 읽어요
2. 코드의 `#define` 번호는 보드 라벨이 아니라 **GPIO 번호** (예: D6 핀 = GPIO27) — [핀맵 문서](../01_docs/pinmap_wemos_d1_r32.md)의 대응표 확인
3. 센서 전원 V는 **3V3**에 — 5V 전용 초음파 모듈은 ECHO를 **분압 저항(1kΩ 직렬 + 2kΩ to GND)** 을 거쳐 연결
   센서를 여러 개 쓸 때: 보드의 **3V3 핀은 하나**뿐이에요 → **미니 브레드보드**에 3V3·GND를 한 번 끌어온 뒤 센서마다 나눠 연결해요 (GND 핀은 보드에 여러 개 있어요)
4. 신호 2개짜리 센서(초음파·7세그)는 **신호 순서(TRIG·ECHO / DIO·CLK)** 가 바뀌면 동작 안 해요

> ⚠ **LCD(I2C)는 본 과정에서 사용하지 않아요.** 굳이 쓰려면 일반 I2C LCD(PCF8574 백팩, 주소 보통 **0x27** 또는 0x3F)를
> 점퍼선으로 보드의 **SDA(GPIO21)·SCL(GPIO22)** 핀에 연결하면 됩니다 (`Wire.begin(21, 22);`).
> 10번 예제도 이 배선(`Wire.begin(21, 22);`) 기준이에요 — 주소 확인은 00번 스캐너로!
