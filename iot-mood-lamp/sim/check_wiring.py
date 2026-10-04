"""배선 점검 — 베이스보드 / Wemos D1 R32+센서쉴드 배선표를 ESP32 핀 규칙·전압·전류 예산에 비춰 자동 검사한다.

전류 값은 부품 데이터시트의 대표값(최악 쪽으로 반올림)이며, 실제 제품마다 다를 수 있다.
"""

JUMPER_V = 5.0   # 베이스보드 전압 점퍼 (G·V·S 헤더의 V 핀 전압)

INPUT_ONLY = {34, 35, 36, 39}
STRAPPING = {0, 2, 5, 12, 15}          # 부팅 모드에 영향
UART0 = {1, 3}
ADC1 = {32, 33, 34, 35, 36, 39}
HEADER_PINS = {15, 2, 4, 16, 17, 5, 18, 19, 21, 3, 1, 22, 23,          # 베이스보드 왼쪽 줄
               13, 12, 14, 27, 26, 25, 33, 32, 35, 34, 39, 36}         # 오른쪽 줄
# Wemos D1 R32 + 센서쉴드 V5: 실드에 인쇄된 우노 라벨 → GPIO
WEMOS_LABEL = {"0": 3, "1": 1, "2": 26, "3": 25, "4": 17, "5": 16, "6": 27, "7": 14, "8": 12, "9": 13,
               "10": 5, "11": 23, "12": 19, "13": 18, "A0": 2, "A1": 4, "A2": 35, "A3": 34, "A4": 36, "A5": 39}
WEMOS_PINS = set(WEMOS_LABEL.values())
ESP32_MA = 240   # 와이파이 송신 순간 최대 (ESP32 데이터시트 802.11b TX)

# (부품, GPIO, 방향, 전원 위치, 전원 전압, 부품→ESP32 신호 최대 전압, 5V 전류 mA, 3.3V 전류 mA, 비고)
#   방향: out = ESP32가 내보냄, in = ESP32가 읽음, adc = 아날로그 읽기
V1 = [
    ("릴레이 모듈",   26, "out", "D26 줄 V", JUMPER_V, None, 90, 0, "코일 5V — 약 70~90mA"),
    ("PIR HC-SR501", 25, "in",  "D25 줄 V", JUMPER_V, 3.3, 1, 0, "전원 4.5V 이상, 출력 3.3V"),
    ("조도센서",      34, "adc", "3.3V 헤더", 3.3, 3.3, 0, 5, "출력 ≤ 전원(3.3V)"),
]
NEO_MA = 16 * 4 * 20 * 128 / 255   # 16구 RGBW · 채널당 20mA · 밝기 상한 128/255
V2 = [
    ("네오픽셀 링 16구", 27, "out", "D27 줄 V", JUMPER_V, None, round(NEO_MA), 0, "밝기 상한 128/255 기준 최악값"),
    ("PIR HC-SR501",    25, "in",  "D25 줄 V", JUMPER_V, 3.3, 1, 0, ""),
    ("조도센서",         34, "adc", "3.3V 헤더", 3.3, 3.3, 0, 5, ""),
]


# Wemos 판 — 실드 V줄은 5V(V1: SEL 꽂음 = 보드 5V, V2: SEL 뺌 = EXT PWR 5V), 조도 VCC는 Bluetooth 헤더 3V3
WEMOS_V1 = [
    ("릴레이 모듈",   WEMOS_LABEL["2"],  "out", "실드 2번 V", 5.0, None, 90, 0, "코일 5V"),
    ("PIR HC-SR501", WEMOS_LABEL["3"],  "in",  "실드 3번 V", 5.0, 3.3, 1, 0, ""),
    ("조도센서",      WEMOS_LABEL["A3"], "adc", "Bluetooth 3V3", 3.3, 3.3, 0, 5, ""),
]
WEMOS_V2 = [
    ("네오픽셀 링 16구", WEMOS_LABEL["6"],  "out", "실드 6번 V (EXT)", 5.0, None, round(NEO_MA), 0, "밝기 상한 128/255 기준"),
    ("PIR HC-SR501",    WEMOS_LABEL["3"],  "in",  "실드 3번 V (EXT)", 5.0, 3.3, 1, 0, ""),
    ("조도센서",         WEMOS_LABEL["A3"], "adc", "Bluetooth 3V3", 3.3, 3.3, 0, 5, ""),
]


def check(name, parts, adapter_ma, header_pins=HEADER_PINS, power_note=f"전압 점퍼 {JUMPER_V:g}V"):
    print(f"\n════ {name} 배선 점검 ({power_note}) ════")
    ok = True
    used = {}
    for part, pin, kind, where, vpow, vsig, ma5, ma3, note in parts:
        msgs = []
        if pin in used:
            msgs.append(f"❌ GPIO{pin}을 {used[pin]}와 같이 씀")
        used[pin] = part
        if pin not in header_pins:
            msgs.append("❌ 보드/실드에 해당 헤더 없음")
        if kind == "out" and pin in INPUT_ONLY:
            msgs.append("❌ 입력 전용 핀에 출력")
        if pin in STRAPPING | UART0:
            msgs.append("❌ 부팅/업로드에 관여하는 핀")
        if kind == "adc" and pin not in ADC1:
            msgs.append("❌ ADC2 — 와이파이를 켜면 아날로그 읽기 불가")
        if vsig is not None and vsig > 3.3:
            msgs.append(f"❌ ESP32로 {vsig}V 신호가 들어옴 (3.3V 초과)")
        if part.startswith("네오픽셀"):
            msgs.append("⚠ 데이터 신호 3.3V < 규격 기준 3.5V(5V×0.7) — 대부분 동작, 깜빡이면 레벨시프터")
        if part.startswith("PIR") and vpow < 4.5:
            msgs.append("❌ HC-SR501은 4.5V 이상 필요")
        if part.startswith("릴레이") and vpow < 4.5:
            msgs.append("❌ 5V 코일 릴레이가 붙지 않음")
        bad = any(m.startswith("❌") for m in msgs)
        ok &= not bad
        status = "❌" if bad else ("⚠" if msgs else "✅")
        print(f"  {status} {part:<14} GPIO{pin:<3} 전원: {where} ({vpow:g}V)  {note}")
        for m in msgs:
            print(f"       {m}")

    total5 = ESP32_MA + sum(p[6] for p in parts)
    total3 = sum(p[7] for p in parts)
    print(f"\n  전류 예산: ESP32 {ESP32_MA} + 부품 {total5 - ESP32_MA} = 최대 약 {total5} mA "
          f"(어댑터 {adapter_ma} mA의 {total5 * 100 // adapter_ma}%)")
    print(f"  3.3V 헤더 부하: {total3} mA")
    if total5 > adapter_ma * 0.8:
        print("  ❌ 어댑터 용량의 80% 초과")
        ok = False
    else:
        print("  ✅ 어댑터 용량의 80% 이내")
    print(f"  결과: {'통과' if ok else '수정 필요'}")
    return ok


def firmware_pins(path):
    import re
    src = open(path, encoding="utf-8").read()
    return {m[0]: int(m[1]) for m in re.findall(r"const int (\w+_PIN)\s*=\s*(\d+);", src)}


def check_firmware():
    """배선표의 GPIO와 펌웨어 상수가 같은지 — 같으면 보드를 바꿔도 코드 수정 없음"""
    import os
    here = os.path.dirname(os.path.abspath(__file__))
    v1 = firmware_pins(os.path.join(here, "../v1_relay_bulb/firmware/step3_auto_lamp/step3_auto_lamp.ino"))
    v2 = firmware_pins(os.path.join(here, "../v2_neopixel_color/firmware/step2_auto_mood_lamp/step2_auto_mood_lamp.ino"))
    pairs = [("V1 RELAY_PIN", v1.get("RELAY_PIN"), [V1[0][1], WEMOS_V1[0][1]]),
             ("V1 PIR_PIN", v1.get("PIR_PIN"), [V1[1][1], WEMOS_V1[1][1]]),
             ("V1 LIGHT_PIN", v1.get("LIGHT_PIN"), [V1[2][1], WEMOS_V1[2][1]]),
             ("V2 LED_PIN", v2.get("LED_PIN"), [V2[0][1], WEMOS_V2[0][1]]),
             ("V2 PIR_PIN", v2.get("PIR_PIN"), [V2[1][1], WEMOS_V2[1][1]]),
             ("V2 LIGHT_PIN", v2.get("LIGHT_PIN"), [V2[2][1], WEMOS_V2[2][1]])]
    print("\n════ 펌웨어 ↔ 배선표 핀 일치 (DevKit · Wemos) ════")
    ok = True
    for name, fw, tables in pairs:
        same = all(fw == x for x in tables)
        ok &= same
        print(f"  {'✅' if same else '❌'} {name} = {fw}  (배선표: DevKit {tables[0]} · Wemos {tables[1]})")
    print(f"  결과: {'통과 — 두 보드 모두 같은 코드' if ok else '수정 필요'}")
    return ok


if __name__ == "__main__":
    results = [
        check("DevKit V1 릴레이 전구형", V1, 1000),
        check("DevKit V2 네오픽셀 컬러형", V2, 2000),
        check("Wemos V1 릴레이 전구형", WEMOS_V1, 1000, WEMOS_PINS, "센서쉴드 SEL 꽂음 · 보드 5V"),
        check("Wemos V2 네오픽셀 컬러형", WEMOS_V2, 2400, WEMOS_PINS, "센서쉴드 SEL 뺌 · EXT PWR 5V"),
    ]
    results.append(check_firmware())
    raise SystemExit(0 if all(results) else 1)
