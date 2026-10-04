"""IoT 무드등 실물형 배선도 — Wemos D1 R32 + 아두이노 센서쉴드 V5.

    python3 draw_wemos_wiring.py      → wiring_wemos_v1_relay.svg / wiring_wemos_v2_neopixel.svg
부품 그림(릴레이·PIR·조도·네오픽셀 등)은 draw_board_wiring.py 의 것을 그대로 쓴다.
GPIO 번호는 DevKit 판과 같다: 릴레이 26(D2) · 네오픽셀 27(D6) · PIR 25(D3) · 조도 34(A3).
"""
import os
from draw_board_wiring import (W, H, HERE, C_SIG, C_5V, C_3V, C_GND, C_AC, t, wire, dupont, badge,
                               relay_module, pir_module, ldr_module, neopixel_ring, resistor,
                               capacitor, adapter, notes, legend, svg_doc)

# ── 실드 위 핀 좌표 (캔버스 좌표) ─────────────────────────────────────
DIG_X0, DIG_DX = 238, 30          # 디지털 13 → 0 (왼쪽 → 오른쪽)
DIG_Y = {"S": 352, "V": 376, "G": 400}
ANA_X0, ANA_DX = 404, 34          # A0 → A5
ANA_Y = {"G": 700, "V": 724, "S": 748}
BT_X, BT_Y0, BT_DY = 676, 470, 24  # Bluetooth 헤더 (맨 아래가 3V3)
BT_PINS = ["", "", "", "3V3"]
SEL_XY = (190, 640)
EXT_XY = (92, 610)


def dig(n, kind):
    return DIG_X0 + (13 - n) * DIG_DX, DIG_Y[kind]


def ana(n, kind):
    return ANA_X0 + n * ANA_DX, ANA_Y[kind]


def bt_3v3():
    return BT_X, BT_Y0 + 3 * BT_DY


def shield(sel_on=True):
    o = []
    # 아래에 깔린 Wemos D1 R32 (가장자리·USB·DC 잭만 보임)
    o += ['<rect x="44" y="282" width="690" height="540" rx="14" fill="#14385f"/>',
          '<rect x="14" y="380" width="70" height="56" rx="4" fill="#c9ced3" stroke="#8a9096"/>',
          '<rect x="26" y="396" width="34" height="22" rx="3" fill="#5a5f64"/>',
          t(8, 372, "micro USB", 12, "#56677e", anchor="start"),
          '<rect x="6" y="690" width="92" height="74" rx="4" fill="#0b0b0b" stroke="#444"/>',
          '<circle cx="34" cy="727" r="12" fill="#2a2a2a" stroke="#777"/>',
          t(8, 784, "DC 잭", 12, "#56677e", anchor="start"),
          t(390, 840, "Wemos D1 R32 (아래) + 센서쉴드 V5 (위)", 14, "#56677e", "bold")]
    # 센서쉴드
    o += ['<rect x="70" y="300" width="640" height="500" rx="12" fill="#b3261e" stroke="#7a1712" stroke-width="2"/>',
          t(390, 560, "Sensor Shield V5.0", 22, "#ffd9d4", "bold"),
          t(390, 586, "핀마다 G·V·S 헤더", 14, "#ffd9d4")]
    # 디지털 헤더 0~13
    o.append(f'<rect x="{DIG_X0-14}" y="{DIG_Y["S"]-14}" width="{13*DIG_DX+28}" height="76" fill="#121212"/>')
    for n in range(14):
        x = DIG_X0 + (13 - n) * DIG_DX
        o.append(t(x, DIG_Y["S"] - 20, str(n), 12, "#ffffff", "bold"))
        for kind, y in DIG_Y.items():
            c = {"S": "#2e9d3e", "V": "#c0392b", "G": "#2e9d3e"}[kind]
            o.append(f'<rect x="{x-6}" y="{y-6}" width="12" height="12" fill="{c}"/>')
    for kind, y in DIG_Y.items():
        o.append(t(DIG_X0 - 30, y + 5, kind, 13, "#ffffff", "bold"))
    o.append(t(DIG_X0 + 6.5 * DIG_DX, 316, "디지털 0~13 (실드 숫자 = 우노 라벨)", 12, "#ffd9d4"))
    # 아날로그 헤더 A0~A5
    o.append(f'<rect x="{ANA_X0-14}" y="{ANA_Y["G"]-14}" width="{5*ANA_DX+28}" height="76" fill="#121212"/>')
    for n in range(6):
        x = ANA_X0 + n * ANA_DX
        o.append(t(x, ANA_Y["S"] + 26, f"A{n}", 12, "#ffffff", "bold"))
        for kind, y in ANA_Y.items():
            c = {"S": "#2e9d3e", "V": "#c0392b", "G": "#2e9d3e"}[kind]
            o.append(f'<rect x="{x-6}" y="{y-6}" width="12" height="12" fill="{c}"/>')
    for kind, y in ANA_Y.items():
        o.append(t(ANA_X0 - 30, y + 5, kind, 13, "#ffffff", "bold"))
    # Bluetooth 헤더 (3V3 핀)
    o.append(f'<rect x="{BT_X-12}" y="{BT_Y0-12}" width="24" height="{3*BT_DY+24}" fill="#121212"/>')
    for i, lab in enumerate(BT_PINS):
        y = BT_Y0 + i * BT_DY
        o.append(f'<rect x="{BT_X-6}" y="{y-6}" width="12" height="12" fill="{"#c0392b" if lab else "#9aa1a8"}"/>')
        if lab:
            o.append(t(BT_X - 18, y + 5, lab, 12, "#ffffff", "bold", "end"))
    o.append(t(BT_X, BT_Y0 - 20, "Bluetooth", 11, "#ffd9d4"))
    # 외부 전원 단자 + SEL 점퍼 + 리셋
    ex, ey = EXT_XY
    o += [f'<rect x="{ex-22}" y="{ey-30}" width="60" height="62" rx="4" fill="#2a9d4b" stroke="#1d6d34"/>',
          f'<circle cx="{ex-4}" cy="{ey}" r="10" fill="#c8ccd0" stroke="#666"/>',
          f'<circle cx="{ex+22}" cy="{ey}" r="10" fill="#c8ccd0" stroke="#666"/>',
          t(ex - 4, ey + 46, "GND", 11, "#ffffff", "bold"), t(ex + 22, ey + 46, "VCC", 11, "#ffffff", "bold"),
          t(ex + 8, ey - 40, "EXT PWR", 11, "#ffd9d4")]
    sx, sy = SEL_XY
    o += [f'<rect x="{sx-6}" y="{sy-6}" width="12" height="12" fill="#d4af37"/>',
          f'<rect x="{sx+18}" y="{sy-6}" width="12" height="12" fill="#d4af37"/>',
          t(sx + 12, sy + 30, "SEL", 12, "#ffffff", "bold")]
    if sel_on:
        o.append(f'<rect x="{sx-12}" y="{sy-14}" width="48" height="28" rx="3" fill="#111" stroke="#f5d75d" stroke-width="2"/>')
    o += ['<rect x="98" y="460" width="34" height="34" rx="4" fill="#d8dcdf"/>', '<circle cx="115" cy="477" r="10" fill="#2b2b2b"/>',
          t(115, 512, "RESET", 10, "#ffd9d4")]
    return o


def tags(o, items):
    """꽂는 자리 이름표 (전선 위에)"""
    for (x, y), lab in items:
        o += [f'<rect x="{x-24}" y="{y-11}" width="48" height="21" rx="4" fill="#f5d75d" stroke="#3a3000"/>',
              t(x, y + 4, lab, 12, "#3a3000", "bold")]


def pir_and_ldr(o):
    po, pp = pir_module(860, 470)
    lo, lp = ldr_module(880, 760)
    o += wire(dig(3, "V"), pp["VCC"], C_5V)
    o += wire(dig(3, "S"), pp["OUT"], C_SIG)
    o += wire(dig(3, "G"), pp["GND"], C_GND)
    o += wire(ana(3, "S"), lp["AO"], C_SIG)
    o += wire(ana(3, "G"), lp["GND"], C_GND)
    o += wire(bt_3v3(), lp["VCC"], C_3V)
    o += po + lo
    for k in ("S", "V", "G"):
        o += dupont(*dig(3, k))
    for k in ("S", "G"):
        o += dupont(*ana(3, k))
    o += dupont(*bt_3v3())
    o += badge(1130, 540, "3", "PIR → 실드 3번 (GPIO25)")
    o += badge(1090, 820, "4", "조도 → A3 (GPIO34) + VCC는 3V3")


def v1():
    o = shield(sel_on=True)
    ro, rp, rt = relay_module(830, 170)
    o += wire(dig(2, "S"), rp["IN"], C_SIG)
    o += wire(dig(2, "V"), rp["VCC"], C_5V)
    o += wire(dig(2, "G"), rp["GND"], C_GND)
    o += ro
    for k in ("S", "V", "G"):
        o += dupont(*dig(2, k))
    pir_and_ldr(o)
    # 220V (교사 작업)
    com, no = rt["COM"], rt["NO"]
    bx, by, px, py = 1370, 120, 1370, 400
    o += [f'<path d="M{com[0]} {com[1]} C 1230 {com[1]}, 1250 {py}, {px-40} {py}" fill="none" stroke="{C_AC}" stroke-width="6"/>',
          f'<path d="M{no[0]} {no[1]} C 1230 {no[1]}, 1260 {by+60}, {bx-12} {by+62}" fill="none" stroke="{C_AC}" stroke-width="6"/>',
          f'<path d="M{px+20} {py-22} C {px+70} {py-110}, {bx+70} {by+110}, {bx+12} {by+62}" fill="none" stroke="#3b6ea5" stroke-width="6" stroke-dasharray="14 8"/>',
          f'<circle cx="{bx}" cy="{by}" r="40" fill="#fff6c8" stroke="#d4b100" stroke-width="3"/>',
          f'<rect x="{bx-15}" y="{by+38}" width="30" height="26" rx="4" fill="#9aa1a8"/>',
          t(bx, by + 6, "전구", 15, "#6b5a00", "bold"),
          f'<rect x="{px-40}" y="{py-22}" width="80" height="44" rx="8" fill="#e8e8e8" stroke="#777"/>',
          t(px, py + 6, "220V", 15, "#333", "bold"), t(px, py + 40, "콘센트", 13, "#333"),
          t(bx + 64, by + 160, "N", 13, "#3b6ea5", "bold", "start")]
    o += badge(1215, 482, "!", "220V는 교사 작업", "#c62828", 210)
    # 전원: USB 충전기 → micro USB, SEL 점퍼 꽂음
    o += adapter(60, 150, "5V 1A 충전기", "micro USB 케이블")
    o += [f'<path d="M140 224 C 140 300, 20 330, 40 380" fill="none" stroke="#333" stroke-width="7" stroke-linecap="round"/>']
    o += [f'<circle cx="{SEL_XY[0]+12}" cy="{SEL_XY[1]}" r="34" fill="none" stroke="#f5d75d" stroke-width="4"/>']
    o += badge(250, 690, "1", "SEL 점퍼 꽂기 (보드 5V 사용)", "#b8860b")
    o += badge(830, 130, "2", "릴레이 → 실드 2번 (GPIO26)")
    tags(o, [((dig(2, "G")[0], DIG_Y["G"] + 30), "2번"), ((dig(3, "G")[0] - 26, DIG_Y["G"] + 30), "3번"),
             ((ana(3, "G")[0], ANA_Y["G"] - 30), "A3")])
    o += legend(1040)
    o += notes(1085, ["① 실드 숫자는 우노 라벨 — 2번=GPIO26, 3번=GPIO25, A3=GPIO34. 코드는 DevKit 판과 같아요 (수정 없음)",
                      "② 실드의 V는 5V예요 → 조도센서 VCC만 Bluetooth 헤더의 3V3 핀으로 따로 연결"],
                 "⚠ 릴레이 단자(COM·NO)와 220V 램프 코드는 교사가 연결하고 절연 — 학생은 실드 쪽 저전압 배선만")
    return svg_doc(o, "IoT 무드등 V1 — Wemos D1 R32 + 센서쉴드 배선도",
                   "릴레이 GPIO26(실드 2) · PIR GPIO25(실드 3) · 조도 GPIO34(A3) — DevKit 판과 같은 코드, 시뮬레이션 검증 완료")


def v2():
    o = shield(sel_on=False)
    no_, npads = neopixel_ring(1080, 200)
    rx, ry = 840, 300
    o += wire(dig(6, "S"), (rx - 26, ry), C_SIG)
    o += wire((rx + 26, ry), npads["DIN"], C_SIG, bend=0.3)
    o += wire(dig(6, "V"), npads["5V"], C_5V)
    o += wire(dig(6, "G"), npads["GND"], C_GND)
    o += no_ + resistor(rx, ry)
    cx, cy = 1290, 370
    o += [f'<path d="M{npads["5V"][0]} {npads["5V"][1]-8} C {npads["5V"][0]} 290, {cx-40} 260, {cx-8} {cy-34}" fill="none" stroke="{C_5V}" stroke-width="3"/>',
          f'<path d="M{npads["GND"][0]} {npads["GND"][1]-8} C {npads["GND"][0]} 280, {cx} 250, {cx+8} {cy-34}" fill="none" stroke="{C_GND}" stroke-width="3"/>']
    o += capacitor(cx, cy)
    for k in ("S", "V", "G"):
        o += dupont(*dig(6, k))
    pir_and_ldr(o)
    # 전원: 2포트 USB 충전기 1개 — ① 보드 micro USB ② USB 전원선(피복선) → EXT PWR, SEL 점퍼 뺌
    ex, ey = EXT_XY
    o += [f'<rect x="60" y="120" width="190" height="100" rx="14" fill="#f4f6f8" stroke="#9aa5b1" stroke-width="2"/>',
          t(155, 152, "2포트 USB 충전기", 15, "#1b2536", "bold"), t(155, 174, "합계 2.4A 이상", 12, "#56677e"),
          '<rect x="90" y="190" width="40" height="14" rx="3" fill="#5a5f64"/>', '<rect x="180" y="190" width="40" height="14" rx="3" fill="#5a5f64"/>',
          f'<path d="M110 204 C 110 300, 10 330, 40 380" fill="none" stroke="#333" stroke-width="7" stroke-linecap="round"/>',
          f'<path d="M200 204 C 200 250, 280 250, 290 262" fill="none" stroke="#333" stroke-width="7" stroke-linecap="round"/>',
          f'<path d="M292 264 C 330 300, 160 470, {ex+22} {ey}" fill="none" stroke="{C_5V}" stroke-width="5"/>',
          f'<path d="M288 266 C 310 310, 140 480, {ex-4} {ey}" fill="none" stroke="{C_GND}" stroke-width="5"/>',
          t(262, 246, "USB 전원선(끝이 빨강·검정 선) → EXT PWR", 12, "#56677e", anchor="start")]
    o += [f'<circle cx="{SEL_XY[0]+12}" cy="{SEL_XY[1]}" r="34" fill="none" stroke="#f5d75d" stroke-width="4"/>']
    o += badge(250, 690, "1", "SEL 점퍼 빼기 → 외부 5V 사용", "#b8860b")
    o += badge(1175, 95, "2", "네오픽셀 → 6번 (GPIO27)")
    tags(o, [((dig(6, "G")[0], DIG_Y["G"] + 30), "6번"), ((dig(3, "G")[0], DIG_Y["G"] + 30), "3번"),
             ((ana(3, "G")[0], ANA_Y["G"] - 30), "A3")])
    o += legend(1040, ac=False)
    o += notes(1085, ["① 네오픽셀 전류는 USB 전원선 → 실드 EXT PWR로 (SEL 점퍼를 빼야 디지털 줄 V가 외부 5V로 바뀜) · 보드는 micro USB로",
                      "② 링 패드에 암 커넥터 선 납땜 · DIN 선 중간 330Ω · 링 5V–GND에 1000µF · 조도센서 VCC만 3V3"],
                 "⚠ SEL 점퍼를 꽂은 채로 EXT PWR에 전원을 넣지 마세요 — 보드 전원과 합선됩니다")
    return svg_doc(o, "IoT 무드등 V2 — Wemos D1 R32 + 센서쉴드 배선도",
                   "네오픽셀 GPIO27(실드 6) · PIR GPIO25(실드 3) · 조도 GPIO34(A3) — DevKit 판과 같은 코드, 시뮬레이션 검증 완료")


if __name__ == "__main__":
    for name, fn in (("wiring_wemos_v1_relay.svg", v1), ("wiring_wemos_v2_neopixel.svg", v2)):
        with open(os.path.join(HERE, name), "w", encoding="utf-8") as f:
            f.write(fn())
        print(name)
