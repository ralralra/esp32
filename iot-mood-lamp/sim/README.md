# 시뮬레이터 — 보드 없이 PC에서 무드등 펌웨어 돌려보기

실제 스케치(`.ino`)를 그대로 PC에서 컴파일해, **가상 시계 · 가상 핀 · 가상 센서 · 가상 네오픽셀** 위에서
시나리오대로 실행하고 결과를 자동으로 검사합니다. 배선이 끝나기 전에 로직과 핀 배치를 미리 확인할 수 있어요.

```bash
cd iot-mood-lamp/sim
make run        # 배선 점검 + V1 시뮬레이션 + V2 시뮬레이션
```

## 무엇을 검사하나

| 파일 | 검사 내용 |
|---|---|
| `check_wiring.py` | DevKit 베이스보드·**Wemos D1 R32 + 센서쉴드** 배선표 점검, **펌웨어 핀 = 배선표 핀** 확인 — 입력 전용 핀에 출력하지 않는지, 부팅 관여 핀(0·2·5·12·15)·USB 핀(1·3) 회피, 아날로그는 ADC1, ESP32로 들어오는 신호 ≤ 3.3V, PIR·릴레이 전원 전압, 어댑터 전류 예산 |
| `sim_v1.cpp` | V1 `step3_auto_lamp.ino` — 부팅 깜빡임 없음, 밝으면 안 켜짐, 히스테리시스, 어두움+사람 → 50ms 안에 켜짐, 자기 빛에 안 꺼짐, 5분 뒤 자동 끄기, 시리얼 수동 명령 |
| `sim_v2.cpp` | V2 `step2_auto_mood_lamp.ino` — 부팅 시 전부 꺼짐, 1초 페이드, 웜화이트(W 채널), 색·밝기 명령, 밝기 상한, 링 전류 추정, 5분 뒤 서서히 끄기, 수동 명령 |

실행 중에도 가상 보드가 **전기 규칙 위반**(입력 전용 핀 출력, 플래시 핀 사용, ADC2 아날로그 읽기)을 잡아냅니다.
일부러 릴레이를 GPIO34로 옮기거나 히스테리시스를 틀리게 고치면 실패로 표시되는 것을 확인했습니다.

## 한계 — 시뮬레이션이 대신하지 못하는 것

- 실제 전압·노이즈, 릴레이 접점, 네오픽셀 타이밍(3.3V 신호 경계값)은 **실물에서 확인**해야 합니다.
- 조도 기준값(1200/1500)과 PIR 감도는 교실마다 다르므로 V1 2단계에서 **실측**해 상수를 바꿉니다.
- 와이파이·앱 연동(V1 4단계, V2 3단계)은 아직 포함하지 않았습니다.

## 배선도 다시 그리기

핀을 바꾸면 `check_wiring.py` · 스케치 · 배선도를 함께 고칩니다.

```bash
cd ../docs/images
python3 draw_board_wiring.py      # DevKit 판 SVG 생성
python3 draw_wemos_wiring.py      # Wemos 판 SVG 생성
HS=/opt/pw-browsers/chromium_headless_shell-1194/chrome-linux/headless_shell   # 또는 크롬
$HS --headless --hide-scrollbars --window-size=1500,1190 --screenshot=$PWD/wiring_v1_relay.png file://$PWD/wiring_v1_relay.svg
```
