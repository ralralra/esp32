"""IoT 무드등 실물형 배선도 — Wemos D1 R32 직결 (쉴드 없이, 브레드보드 없이).

    python3 draw_wemos_direct.py      → wiring_wemos_direct_v1.svg / wiring_wemos_direct_v2.svg
보드 그림은 D1 R32 핀맵(전원 헤더에 5V 두 개)을 따랐다. 부품 그림은 draw_board_wiring.py 재사용.
GPIO는 DevKit·센서쉴드 판과 같다: 릴레이 26 · 네오픽셀 27 · PIR 25 · 조도 34.
"""
import os
from draw_board_wiring import (W, H, HERE, C_SIG, C_5V, C_3V, C_GND, C_AC, t, wire, dupont, badge,
                               module_pin, relay_module, pir_module, neopixel_ring, resistor,
                               capacitor, adapter, notes, legend, svg_doc)

OX, OY, K = 470, 140, 0.6        # 캔버스 = (OX + x·K, OY + (y − 100)·K) — 보드 그림 좌표계


def P(x, y):
    return OX + x * K, OY + (y - 100) * K


LEFT_PWR = [("OD", 505), ("5V", 540), ("RST", 574), ("3V3", 608), ("5V", 642), ("GND", 677), ("GND", 711), ("VIN", 746)]
LEFT_INNER = [("IO15", 608), ("IO33", 642), ("IO32", 677)]
ANALOG = [("IO02", 815), ("IO04", 850), ("IO36", 884), ("IO34", 919), ("IO38", 953), ("IO39", 988)]
RIGHT = [("SCL", 366), ("SDA", 400), ("RST", 435), ("GND", 469), ("IO18", 504), ("IO19", 539), ("IO23", 573),
         ("IO05", 608), ("IO13", 642), ("IO12", 677), ("IO14", 747), ("IO27", 781), ("IO16", 816), ("IO17", 850),
         ("IO25", 885), ("IO26", 919), ("TX0", 954), ("RX0", 989)]
MIDDLE = [("SD2", 642), ("SD3", 677), ("CMD", 711), ("CLK", 746), ("SD0", 781), ("SD1", 816)]

PIN = {  # 우리가 쓰는 핀의 캔버스 좌표
    "5V_1": P(40, 540), "5V_2": P(40, 642), "3V3": P(40, 608),
    "GND_L1": P(40, 677), "GND_L2": P(40, 711), "GND_R": P(697, 469),
    "IO34": P(40, 919), "IO25": P(697, 885), "IO26": P(697, 919), "IO27": P(697, 781),
}


def board():
    s = K
    o = [f'<g transform="translate({OX},{OY - 100 * K}) scale({K})">',
         '<path d="M0 115 H740 V990 L700 1050 H0 Z" fill="#0f0f10"/>',
         '<rect x="62" y="40" width="120" height="210" rx="6" fill="#111" stroke="#444" stroke-width="2"/>',
         '<circle cx="122" cy="90" r="22" fill="#2a2a2a" stroke="#777" stroke-width="2"/>',
         '<rect x="470" y="115" width="105" height="90" fill="#bfc4c8"/>',
         t(422, 180, "USB", 28, "#ffffff"),
         '<rect x="632" y="120" width="90" height="130" rx="4" fill="#d8dcdf"/>',
         '<circle cx="677" cy="185" r="26" fill="#1b1b1b"/>',
         '<rect x="275" y="255" width="70" height="70" fill="#1d1d1d" stroke="#555"/>',
         '<rect x="445" y="290" width="90" height="145" fill="#1d1d1d" stroke="#555"/>',
         '<rect x="258" y="375" width="55" height="70" fill="#d8d0c0" stroke="#888"/>',
         '<rect x="255" y="540" width="50" height="22" fill="#9aa1a8"/>',
         '<rect x="170" y="710" width="235" height="250" rx="6" fill="#c9ced3" stroke="#8a9096" stroke-width="2"/>',
         '<rect x="170" y="960" width="235" height="85" fill="#1a1a1a"/>',
         t(435, 740, "D1 R32", 30, "#ffffff", "bold", rot=90)]
    for cx, cy in [(37, 290), (697, 285), (105, 1000), (478, 1000)]:
        o.append(f'<circle cx="{cx}" cy="{cy}" r="24" fill="#ffffff"/>')

    def pin(x, y, lab, side):
        o.append(f'<circle cx="{x}" cy="{y}" r="9" fill="#ffffff"/>')
        if side == "L":
            o.append(t(x + 16, y + 8, lab, 22, "#ffffff", anchor="start"))
        else:
            o.append(t(x - 16, y + 8, lab, 22, "#ffffff", anchor="end"))

    for lab, y in LEFT_PWR + ANALOG:
        pin(40, y, lab, "L")
    for lab, y in LEFT_INNER:
        pin(107, y, lab, "L")
    for lab, y in RIGHT:
        pin(697, y, lab, "R")
    for lab, y in MIDDLE:
        pin(597, y, lab, "R")
    o += ['<path d="M95 800 V990" stroke="#ffffff" stroke-width="2"/>', t(130, 900, "ANALOG", 20, "#ffffff", rot=90),
          '<path d="M635 475 V990" stroke="#ffffff" stroke-width="2"/>', t(625, 560, "SPI", 20, "#ffffff", rot=90),
          t(625, 880, "DIGITAL", 20, "#ffffff", rot=90), "</g>"]
    return o


def route(a, b, color, lane_x, lane_y, w=5):
    """보드를 돌아가는 배선 — 왼쪽 핀에서 나가 위/아래 길로 오른쪽 부품까지"""
    (x1, y1), (x2, y2) = a, b
    d = f"M{x1:.0f} {y1:.0f} H{lane_x} V{lane_y} H{x2 - 60:.0f} V{y2:.0f} H{x2:.0f}"
    return [f'<path d="{d}" fill="none" stroke="#000" stroke-opacity="0.25" stroke-width="{w+2}" stroke-linejoin="round" stroke-linecap="round"/>',
            f'<path d="{d}" fill="none" stroke="{color}" stroke-width="{w}" stroke-linejoin="round" stroke-linecap="round"/>']


def ldr_left(x, y):
    """핀이 오른쪽에 있는 조도센서 모듈 (보드 왼쪽에 놓음)"""
    o = [f'<rect x="{x}" y="{y}" width="190" height="140" rx="8" fill="#1f5fae" stroke="#0e3a70" stroke-width="2"/>',
         f'<circle cx="{x+60}" cy="{y+70}" r="24" fill="#f3e3c3" stroke="#b08a4a" stroke-width="2"/>',
         f'<path d="M{x+44} {y+60} q4 -8 8 0 t8 0 t8 0 t8 0 M{x+44} {y+72} q4 -8 8 0 t8 0 t8 0 t8 0 M{x+44} {y+84} q4 -8 8 0 t8 0 t8 0 t8 0" fill="none" stroke="#b5452b" stroke-width="2"/>',
         t(x + 95, y - 10, "조도센서 (LDR, AO)", 14, "#1b2536", "bold")]
    pins = {"VCC": (x + 192, y + 30), "GND": (x + 192, y + 70), "AO": (x + 192, y + 110)}
    for lab, (px, py) in pins.items():
        o += [f'<rect x="{px-7}" y="{py-4}" width="14" height="8" fill="#d4af37"/>',
              t(px - 14, py + 4, lab, 12, "#ffffff", "bold", "end")]
    return o, pins


def tag(o, xy, lab, dx=0, dy=-22):
    x, y = xy[0] + dx, xy[1] + dy
    o += [f'<rect x="{x-26}" y="{y-11}" width="52" height="21" rx="4" fill="#f5d75d" stroke="#3a3000"/>',
          t(x, y + 4, lab, 12, "#3a3000", "bold")]


def power_table(o, rows, y0=845):
    o.append(t(60, y0, "전원 나눠 쓰기 — 부품마다 전원 핀 하나씩", 15, "#1b2536", "bold", "start"))
    for i, (pin_, part, col) in enumerate(rows):
        y = y0 + 32 + i * 30
        o += [f'<rect x="60" y="{y-18}" width="16" height="16" rx="3" fill="{col}"/>',
              t(86, y - 4, pin_, 14, "#1b2536", "bold", "start"), t(190, y - 4, f"→ {part}", 14, "#1b2536", anchor="start")]


def ldr_and_pir(o):
    lo, lp = ldr_left(90, 420)
    po, pp = pir_module(1060, 520)
    # 조도 — 왼쪽 헤더끼리
    o += wire(PIN["3V3"], lp["VCC"], C_3V, bend=0.4)
    o += wire(PIN["GND_L2"], lp["GND"], C_GND, bend=0.4)
    o += wire(PIN["IO34"], lp["AO"], C_SIG, bend=0.4)
    # PIR — 신호는 오른쪽 IO25, 전원·GND는 왼쪽에서 보드 아래로 돌아감
    o += wire(PIN["IO25"], pp["OUT"], C_SIG)
    o += route(PIN["5V_2"], pp["VCC"], C_5V, 452, 790)
    o += route(PIN["GND_L1"], pp["GND"], C_GND, 440, 805)
    o += lo + po
    for k in ("3V3", "GND_L2", "IO34", "IO25", "5V_2", "GND_L1"):
        o += dupont(*PIN[k])
    tag(o, PIN["IO34"], "IO34", dx=-40, dy=26)
    tag(o, PIN["IO25"], "IO25", dx=46, dy=0)
    o += badge(1060, 485, "3", "PIR → IO25 · 5V② · GND")
    o += badge(60, 385, "4", "조도 → IO34 · 3V3 · GND")


def v1():
    o = board()
    ro, rp, rt = relay_module(1040, 190)
    o += wire(PIN["IO26"], rp["S"], C_SIG)
    o += wire(PIN["GND_R"], rp["−"], C_GND)
    o += route(PIN["5V_1"], rp["+"], C_5V, 440, 112)
    o += ro
    for k in ("IO26", "GND_R", "5V_1"):
        o += dupont(*PIN[k])
    tag(o, PIN["IO26"], "IO26", dx=46, dy=0)
    ldr_and_pir(o)
    # 220V (교사 작업)
    com, no = rt["COM"], rt["NO"]
    bx, by, px, py = 1420, 300, 1420, 460
    o += [f'<path d="M{com[0]} {com[1]} C 1380 {com[1]}, 1360 {py}, {px-40} {py}" fill="none" stroke="{C_AC}" stroke-width="6"/>',
          f'<path d="M{no[0]} {no[1]} C 1370 {no[1]}, 1380 {by-80}, {bx} {by-44}" fill="none" stroke="{C_AC}" stroke-width="6"/>',
          f'<circle cx="{bx}" cy="{by-80}" r="36" fill="#fff6c8" stroke="#d4b100" stroke-width="3"/>',
          t(bx, by - 74, "전구", 14, "#6b5a00", "bold"),
          f'<rect x="{px-36}" y="{py-20}" width="72" height="40" rx="8" fill="#e8e8e8" stroke="#777"/>',
          t(px, py + 6, "220V", 14, "#333", "bold")]
    o += badge(1100, 395, "!", "220V는 교사 작업", "#c62828", 210)
    # 전원: 충전기 → micro USB
    ux, uy = P(522, 115)
    o += adapter(170, 150, "5V 1A 충전기", "micro USB 케이블")
    o += [f'<path d="M320 187 C {ux:.0f} 187, {ux:.0f} 150, {ux:.0f} {uy:.0f}" fill="none" stroke="#333" stroke-width="7" stroke-linecap="round"/>']
    o += badge(1040, 150, "2", "릴레이 S→IO26 · +→5V① · −→GND")
    power_table(o, [("5V ①", "릴레이 VCC", C_5V), ("5V ②", "PIR VCC", C_5V), ("3V3", "조도센서 VCC", C_3V),
                    ("GND ×3", "릴레이(오른쪽 GND) · PIR · 조도(왼쪽 GND 둘)", C_GND)])
    o += legend(1040, wires="모든 연결은 암-수 점퍼선 (보드 쪽 수) · 쉴드·브레드보드 없음")
    o += notes(1085, ["① 쉴드·브레드보드 없이 암-수 점퍼선으로 직결 — 보드에 IO 번호가 인쇄돼 있어 코드 번호 그대로 찾으면 돼요",
                      "② 5V 핀이 두 개라 릴레이·PIR이 하나씩, 조도센서는 3V3 — 코드는 DevKit·센서쉴드 판과 같아요"],
                 "⚠ 릴레이 단자(COM·NO)와 220V 램프 코드는 교사가 연결하고 절연 — 학생은 보드 쪽 저전압 배선만")
    return svg_doc(o, "IoT 무드등 V1 — Wemos D1 R32 직결 배선도 (쉴드 없이)",
                   "릴레이 IO26 · PIR IO25 · 조도 IO34 · 5V 두 핀을 하나씩 — 시뮬레이션 검증 완료")


def v2():
    o = board()
    no_, npads = neopixel_ring(1200, 205, r=95)
    rx, ry = 1040, 420
    o += wire(PIN["IO27"], (rx - 26, ry), C_SIG)
    o += wire((rx + 26, ry), npads["DIN"], C_SIG, bend=0.3)
    o += wire(PIN["GND_R"], npads["GND"], C_GND)
    d = f"M{PIN['5V_1'][0]:.0f} {PIN['5V_1'][1]:.0f} H440 V112 H1000 V350 H{npads['5V'][0]:.0f} V{npads['5V'][1]:.0f}"
    o += [f'<path d="{d}" fill="none" stroke="#000" stroke-opacity="0.25" stroke-width="7" stroke-linejoin="round"/>',
          f'<path d="{d}" fill="none" stroke="{C_5V}" stroke-width="5" stroke-linejoin="round" stroke-linecap="round"/>']
    o += no_ + resistor(rx, ry)
    cx, cy = 1380, 360
    o += [f'<path d="M{npads["5V"][0]} {npads["5V"][1]-8} C {npads["5V"][0]} 300, {cx-40} 280, {cx-8} {cy-34}" fill="none" stroke="{C_5V}" stroke-width="3"/>',
          f'<path d="M{npads["GND"][0]} {npads["GND"][1]-8} C {npads["GND"][0]} 290, {cx} 270, {cx+8} {cy-34}" fill="none" stroke="{C_GND}" stroke-width="3"/>']
    o += capacitor(cx, cy)
    for k in ("IO27", "GND_R", "5V_1"):
        o += dupont(*PIN[k])
    tag(o, PIN["IO27"], "IO27", dx=46, dy=0)
    ldr_and_pir(o)
    ux, uy = P(522, 115)
    o += adapter(170, 150, "5V 2A 어댑터", "micro USB 케이블")
    o += [f'<path d="M320 187 C {ux:.0f} 187, {ux:.0f} 150, {ux:.0f} {uy:.0f}" fill="none" stroke="#333" stroke-width="7" stroke-linecap="round"/>']
    o += badge(1000, 95, "2", "네오픽셀 → IO27 · 5V① · GND")
    power_table(o, [("5V ①", "네오픽셀 5V (밝기 상한 64)", C_5V), ("5V ②", "PIR VCC", C_5V), ("3V3", "조도센서 VCC", C_3V),
                    ("GND ×3", "네오픽셀(오른쪽 GND) · PIR · 조도(왼쪽 GND 둘)", C_GND)])
    o += legend(1040, ac=False, wires="모든 연결은 암-수 점퍼선 (보드 쪽 수) · 쉴드·브레드보드 없음")
    o += notes(1085, ["① 네오픽셀 전류가 보드를 거쳐 5V 핀으로 나가요 → 펌웨어 MAX_BRIGHTNESS를 128 → 64로 낮추기",
                      "② 링 패드에 암 커넥터 선 납땜 · DIN 선 중간 330Ω · 링 5V–GND에 1000µF · 조도센서 VCC만 3V3"],
                 "⚠ 더 밝게 쓰려면 센서쉴드(EXT PWR) 판이나 DevKit 베이스보드 판으로 — 전원을 보드 밖에서 넣어요")
    return svg_doc(o, "IoT 무드등 V2 — Wemos D1 R32 직결 배선도 (쉴드 없이)",
                   "네오픽셀 IO27 · PIR IO25 · 조도 IO34 · 5V 두 핀을 하나씩 — 시뮬레이션 검증 완료")


if __name__ == "__main__":
    for name, fn in (("wiring_wemos_direct_v1.svg", v1), ("wiring_wemos_direct_v2.svg", v2)):
        with open(os.path.join(HERE, name), "w", encoding="utf-8") as f:
            f.write(fn())
        print(name)
