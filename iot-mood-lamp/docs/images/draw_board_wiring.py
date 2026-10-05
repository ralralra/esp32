"""IoT 무드등 실물형 배선도 — ESP32 DevKit 30핀 + 확장 베이스보드.

    python3 draw_board_wiring.py      → wiring_v1_relay.svg / wiring_v2_neopixel.svg
PNG는 sim/README.md 의 렌더링 명령으로 만든다.
핀 배치는 sim/check_wiring.py 와 같아야 한다 (릴레이 26 · 네오픽셀 27 · PIR 25 · 조도 34).
"""
import math, os

HERE = os.path.dirname(os.path.abspath(__file__))
FONT = "Pretendard, 'Noto Sans CJK KR', 'WenQuanYi Zen Hei', sans-serif"
W, H = 1500, 1190
OX, OY, K = 50, 120, 1.5          # 보드 배치: 캔버스 = O + 보드좌표 × K

C_SIG, C_5V, C_3V, C_GND, C_AC = "#f2b705", "#d93025", "#ff7a00", "#222222", "#8d5524"

LEFT_ROWS  = ["D15", "D2", "D4", "D16", "D17", "D5", "D18", "D19", "D21", "RXD", "TXD", "D22", "D23"]
RIGHT_ROWS = ["D13", "D12", "D14", "D27", "D26", "D25", "D33", "D32", "D35", "D34", "VN", "VP", "EN"]
ROW_Y0, ROW_DY = 265, 17.67
COL = {"L": {"S": 40, "V": 60, "G": 80}, "R": {"G": 300, "V": 320, "S": 340}}
PWR_COL = {"5V": 292, "3.3V": 312, "GND": 332}
PWR_ROWS = [183, 201, 219, 237]


def P(xb, yb):
    return OX + xb * K, OY + yb * K


def row_y(name):
    return ROW_Y0 + RIGHT_ROWS.index(name) * ROW_DY


def pin_xy(row, kind):
    return P(COL["R"][kind], row_y(row))


def t(x, y, s, size=14, color="#1b2536", weight="normal", anchor="middle", rot=None, family=None):
    w = ' font-weight="bold"' if weight == "bold" else ""
    tr = f' transform="rotate({rot} {x} {y})"' if rot is not None else ""
    ff = f' font-family="{family}"' if family else ""
    return f'<text x="{x:.1f}" y="{y:.1f}" font-size="{size}"{w}{ff} fill="{color}" text-anchor="{anchor}"{tr}>{s}</text>'


# ── 보드 (보드 좌표, 380×520) ─────────────────────────────────────────
def board(jumper="5V"):
    o = [f'<g transform="translate({OX},{OY}) scale({K})">']
    o.append('<rect x="0" y="58" width="380" height="462" rx="14" fill="#1d1f22"/>')
    for cx, cy in [(22, 150), (360, 150), (22, 505), (360, 505)]:
        o.append(f'<circle cx="{cx}" cy="{cy}" r="9" fill="#ffffff"/>')
    # 위쪽 전원 입력부
    o.append('<rect x="35" y="12" width="62" height="118" rx="4" fill="#0b0b0b" stroke="#444" stroke-width="1.5"/>')
    o.append('<circle cx="66" cy="36" r="9" fill="#2a2a2a" stroke="#777"/>')
    o.append(t(106, 95, "DC 6.5-16V", 9, "#cfd6de", rot=90))
    o.append('<rect x="116" y="44" width="36" height="46" rx="3" fill="#c9ced3" stroke="#8a9096"/>')
    o.append('<rect x="124" y="54" width="20" height="10" rx="2" fill="#5a5f64"/>')
    o.append('<rect x="183" y="74" width="30" height="34" rx="2" fill="#0d0d0d"/>')
    o.append('<rect x="234" y="42" width="58" height="46" rx="6" fill="#c9ced3" stroke="#8a9096"/>')
    o.append('<rect x="244" y="58" width="38" height="12" rx="6" fill="#5a5f64"/>')
    o.append(t(302, 66, "USB5V", 9, "#cfd6de", rot=90))
    o.append('<rect x="333" y="78" width="10" height="6" fill="#ff4d4d"/>')
    o.append(t(352, 100, "PWR-LED", 8, "#cfd6de", rot=90))
    # 전압 점퍼 3핀 (3V · 공통 · 5V)
    for x, c in [(262, "#c0392b"), (280, "#2e9d3e"), (298, "#c0392b")]:
        o.append(f'<rect x="{x-5}" y="124" width="10" height="10" fill="{c}"/>')
    cap_x = 271 if jumper == "3V" else 289
    o.append(f'<rect x="{cap_x-14}" y="119" width="28" height="20" rx="2" fill="#111" stroke="#f5d75d" stroke-width="2"/>')
    o.append(t(262, 150, "3V", 8, "#cfd6de"))
    o.append(t(298, 150, "5V", 8, "#cfd6de"))
    o.append(t(318, 132, "JUMP", 8, "#cfd6de", rot=90))
    # 왼쪽 위 I2C 헤더
    for i, lab in enumerate(["GND", "VCC", "D21", "D22"]):
        y = PWR_ROWS[i]
        o.append(t(22, y + 3, lab, 8, "#cfd6de"))
        for x, c in [(45, "#2e9d3e"), (65, "#c0392b" if lab == "VCC" else "#2e9d3e")]:
            o.append(f'<rect x="{x-5}" y="{y-5}" width="10" height="10" fill="{c}"/>')
    # 오른쪽 위 전원 헤더 5V · 3.3V · GND
    o.append(t(292, 170, "5V", 8, "#cfd6de"))
    o.append(t(312, 170, "3.3V", 7, "#cfd6de"))
    o.append(t(352, 214, "GND", 8, "#cfd6de", rot=90))
    for y in PWR_ROWS:
        for lab, x in PWR_COL.items():
            c = "#2e9d3e" if lab == "GND" else "#c0392b"
            o.append(f'<rect x="{x-5}" y="{y-5}" width="10" height="10" fill="{c}"/>')
    # ESP32 DevKit
    o.append('<rect x="100" y="150" width="182" height="350" rx="6" fill="#43474b"/>')
    o.append('<rect x="172" y="146" width="38" height="34" rx="3" fill="#c9ced3" stroke="#8a9096"/>')
    for cx, lab in [(132, "IO0"), (250, "EN")]:
        o.append(f'<rect x="{cx-11}" y="168" width="22" height="18" rx="2" fill="#d8dcdf"/>')
        o.append(f'<circle cx="{cx}" cy="177" r="6" fill="#2b2b2b"/>')
        o.append(t(cx, 198, lab, 8, "#e8ecef"))
    o.append('<rect x="168" y="205" width="46" height="46" fill="#151515"/>')
    o.append('<rect x="150" y="262" width="22" height="34" fill="#151515"/>')
    o.append('<rect x="198" y="270" width="20" height="16" fill="#f0a020"/>')
    o.append('<rect x="124" y="312" width="134" height="132" rx="4" fill="#b8bdc2" stroke="#8a9096"/>')
    o.append(t(191, 372, "ESP-WROOM-32", 11, "#2b2f33", "bold"))
    o.append(t(191, 390, "ESP32 DevKit 30핀", 9, "#3d434a"))
    o.append('<path d="M138 452 h22 v26 h12 v-26 h12 v26 h12 v-26 h12 v26 h12 v-26 h22" fill="none" stroke="#2a2d30" stroke-width="3"/>')
    for i in range(15):
        y = 228 + i * 17.7
        for x in (110, 272):
            o.append(f'<circle cx="{x}" cy="{y:.1f}" r="5.5" fill="#1aa3a3" stroke="#0d6e6e"/>')
    # G·V·S 헤더
    for side, rows in (("L", LEFT_ROWS), ("R", RIGHT_ROWS)):
        xs = COL[side]
        x0 = min(xs.values()) - 8
        o.append(f'<rect x="{x0}" y="{ROW_Y0-9}" width="56" height="{ROW_DY*12+18:.0f}" fill="#0c0c0c"/>')
        for i, lab in enumerate(rows):
            y = ROW_Y0 + i * ROW_DY
            for kind, x in xs.items():
                c = {"S": "#2e9d3e", "V": "#c0392b", "G": "#2e9d3e"}[kind]
                o.append(f'<rect x="{x-5}" y="{y-5:.1f}" width="10" height="10" fill="{c}"/>')
            lx = 18 if side == "L" else 362
            o.append(t(lx, y + 3, lab, 8, "#cfd6de"))
        labels = ["S", "V", "G"] if side == "L" else ["G", "V", "S"]
        for lab, x in zip(labels, sorted(xs.values())):
            o.append(t(x, 506, lab, 9, "#cfd6de", "bold"))
    o.append("</g>")
    return o


# ── 배선 도구 ───────────────────────────────────────────────────────
def wire(a, b, color, w=5, bend=0.45, via=None):
    (x1, y1), (x2, y2) = a, b
    if via:
        vx, vy = via
        d = f"M{x1:.1f} {y1:.1f} C {x1+40:.1f} {y1:.1f}, {vx:.1f} {vy+40:.1f}, {vx:.1f} {vy:.1f} S {x2-120:.1f} {y2:.1f}, {x2:.1f} {y2:.1f}"
    else:
        dx = (x2 - x1) * bend
        d = f"M{x1:.1f} {y1:.1f} C {x1+dx:.1f} {y1:.1f}, {x2-dx:.1f} {y2:.1f}, {x2:.1f} {y2:.1f}"
    return [f'<path d="{d}" fill="none" stroke="#000" stroke-opacity="0.25" stroke-width="{w+2}" stroke-linecap="round"/>',
            f'<path d="{d}" fill="none" stroke="{color}" stroke-width="{w}" stroke-linecap="round"/>']


def dupont(x, y, n=1, horizontal=False):
    """헤더 위에 꽂힌 암 커넥터 하우징"""
    if horizontal:
        return [f'<rect x="{x-9}" y="{y-8}" width="{n*30+3}" height="16" rx="2" fill="#141414" stroke="#555" stroke-width="1"/>']
    return [f'<rect x="{x-8}" y="{y-8}" width="16" height="16" rx="2" fill="#141414" stroke="#555" stroke-width="1"/>']


def module_pin(x, y, label, side="left"):
    o = [f'<rect x="{x-7}" y="{y-4}" width="14" height="8" fill="#d4af37"/>']
    if side == "left":
        o.append(t(x + 14, y + 4, label, 12, "#ffffff", "bold", "start"))
    return o


def badge(x, y, n, text, color="#1758a8", width=None):
    w = width or (len(text) * 13 + 46)
    return [f'<rect x="{x}" y="{y-17}" width="{w}" height="30" rx="15" fill="{color}"/>',
            f'<circle cx="{x+15}" cy="{y-2}" r="11" fill="#ffffff"/>',
            t(x + 15, y + 3, n, 13, color, "bold"),
            t(x + 32, y + 3, text, 14, "#ffffff", "bold", "start")]


# ── 부품 ────────────────────────────────────────────────────────────
def relay_module(x, y):
    """KY-019형 1채널 릴레이 — 검은 기판, 왼쪽 핀 S·+·−, 오른쪽 터미널 NC·COM·NO"""
    o = [f'<rect x="{x}" y="{y}" width="300" height="150" rx="8" fill="#1a1a1a" stroke="#000" stroke-width="2"/>']
    for cx, cy in [(x + 14, y + 14), (x + 286, y + 14), (x + 14, y + 136), (x + 286, y + 136)]:
        o.append(f'<circle cx="{cx}" cy="{cy}" r="7" fill="#ffffff"/>')
    o += [f'<rect x="{x+78}" y="{y+30}" width="134" height="96" rx="4" fill="#2f7fd8" stroke="#1a4f8f"/>',
          t(x + 145, y + 56, "TONGLING", 12, "#eaf2ff", "bold"),
          t(x + 145, y + 76, "5VDC", 13, "#ffffff", "bold"),
          t(x + 145, y + 94, "10A 250VAC", 10, "#cfe1ff"),
          t(x + 145, y + 112, "JQC-3FF-S-Z", 10, "#cfe1ff"),
          f'<rect x="{x+228}" y="{y+22}" width="62" height="112" rx="3" fill="#2a5fd0" stroke="#1a3f90"/>']
    for i, lab in enumerate(["NC", "COM", "NO"]):
        cy = y + 42 + i * 36
        o += [f'<circle cx="{x+259}" cy="{cy}" r="11" fill="#c8ccd0" stroke="#666"/>',
              f'<path d="M{x+252} {cy} h14" stroke="#555" stroke-width="3"/>',
              t(x + 222, cy + 4, lab, 10, "#ffffff", "bold", "end")]
    o += [f'<rect x="{x+96}" y="{y+8}" width="40" height="8" rx="3" fill="#444"/>',          # 다이오드
          f'<rect x="{x+150}" y="{y+6}" width="14" height="12" rx="2" fill="#333"/>',         # 트랜지스터
          f'<circle cx="{x+60}" cy="{y+28}" r="5" fill="#ff5252"/>', t(x + 60, y + 18, "LED", 8, "#aaa"),
          t(x + 150, y - 10, "릴레이 모듈 KY-019형 (5V · HIGH 트리거)", 14, "#1b2536", "bold")]
    pins = {"S": (x - 2, y + 48), "+": (x - 2, y + 75), "−": (x - 2, y + 102)}
    for lab, (px, py) in pins.items():
        o += module_pin(px, py, lab)
    terms = {"NO": (x + 259, y + 114), "COM": (x + 259, y + 78)}
    return o, pins, terms


def pir_module(x, y):
    o = [f'<rect x="{x}" y="{y}" width="230" height="160" rx="8" fill="#1e7a3c" stroke="#0f4d24" stroke-width="2"/>',
         f'<circle cx="{x+130}" cy="{y+80}" r="62" fill="#f4f4f1" stroke="#c9c9c2" stroke-width="2"/>']
    for r in (48, 34, 20):
        o.append(f'<circle cx="{x+130}" cy="{y+80}" r="{r}" fill="none" stroke="#dcdcd4" stroke-width="2"/>')
    o += [f'<circle cx="{x+210}" cy="{y+40}" r="9" fill="#f08c00" stroke="#a35f00"/>',
          f'<circle cx="{x+210}" cy="{y+120}" r="9" fill="#f08c00" stroke="#a35f00"/>',
          t(x + 115, y - 10, "인체감지 PIR (HC-SR501)", 14, "#1b2536", "bold")]
    pins = {"VCC": (x - 2, y + 45), "OUT": (x - 2, y + 80), "GND": (x - 2, y + 115)}
    for lab, (px, py) in pins.items():
        o += module_pin(px, py, lab)
    return o, pins


def ldr_module(x, y):
    o = [f'<rect x="{x}" y="{y}" width="190" height="120" rx="8" fill="#1f5fae" stroke="#0e3a70" stroke-width="2"/>',
         f'<circle cx="{x+130}" cy="{y+60}" r="24" fill="#f3e3c3" stroke="#b08a4a" stroke-width="2"/>',
         f'<path d="M{x+114} {y+50} q4 -8 8 0 t8 0 t8 0 t8 0 M{x+114} {y+62} q4 -8 8 0 t8 0 t8 0 t8 0 M{x+114} {y+74} q4 -8 8 0 t8 0 t8 0 t8 0" fill="none" stroke="#b5452b" stroke-width="2"/>',
         t(x + 95, y - 10, "조도센서 모듈 (LDR, 아날로그 AO)", 14, "#1b2536", "bold")]
    pins = {"AO": (x - 2, y + 28), "GND": (x - 2, y + 60), "VCC": (x - 2, y + 92)}
    for lab, (px, py) in pins.items():
        o += module_pin(px, py, lab)
    return o, pins


def neopixel_ring(cx, cy, r=105):
    o = [f'<circle cx="{cx}" cy="{cy}" r="{r}" fill="#1a1a1a" stroke="#000" stroke-width="2"/>',
         f'<circle cx="{cx}" cy="{cy}" r="{r-38}" fill="#ffffff"/>']
    for i in range(24):
        a = 2 * math.pi * i / 24 - math.pi / 2
        lx, ly = cx + (r - 19) * math.cos(a), cy + (r - 19) * math.sin(a)
        o.append(f'<rect x="{lx-8:.1f}" y="{ly-8:.1f}" width="16" height="16" rx="2" fill="#fff6dc" stroke="#d8c48c" transform="rotate({math.degrees(a):.1f} {lx:.1f} {ly:.1f})"/>')
        o.append(f'<circle cx="{lx:.1f}" cy="{ly:.1f}" r="4" fill="#ffd36b"/>')
    o.append(t(cx, cy - 6, "네오픽셀 링", 14, "#1b2536", "bold"))
    o.append(t(cx, cy + 14, "24구 RGBW", 12, "#56677e"))
    pads = {"DIN": (cx - 70, cy + r + 18), "5V": (cx - 20, cy + r + 18), "GND": (cx + 30, cy + r + 18)}
    for lab, (px, py) in pads.items():
        o += [f'<rect x="{px-9}" y="{py-24}" width="18" height="18" rx="3" fill="#d4af37"/>',
              t(px, py + 12, lab, 12, "#1b2536", "bold")]
    return o, {k: (x, y - 15) for k, (x, y) in pads.items()}


def resistor(x, y):
    o = [f'<rect x="{x-24}" y="{y-9}" width="48" height="18" rx="8" fill="#e8d3a6" stroke="#9a7f4a"/>']
    for dx, c in [(-12, "#ff7a00"), (-4, "#ff7a00"), (4, "#8b4513"), (14, "#d4af37")]:
        o.append(f'<rect x="{x+dx}" y="{y-9}" width="4" height="18" fill="{c}"/>')
    o.append(t(x, y - 16, "330Ω", 12, "#8a4b00", "bold"))
    return o


def capacitor(x, y):
    return [f'<rect x="{x-16}" y="{y-32}" width="32" height="44" rx="5" fill="#1f3f8f" stroke="#10245a"/>',
            f'<rect x="{x+6}" y="{y-32}" width="7" height="44" fill="#9fb3e6"/>',
            t(x, y + 30, "1000µF", 12, "#1b2536", "bold"),
            t(x, y + 46, "+극 → 5V", 11, "#56677e")]


def adapter(x, y, label, sub="USB-C 케이블"):
    return [f'<rect x="{x}" y="{y}" width="150" height="74" rx="12" fill="#f4f6f8" stroke="#9aa5b1" stroke-width="2"/>',
            f'<rect x="{x+150}" y="{y+18}" width="26" height="12" fill="#b0b6bc"/>',
            f'<rect x="{x+150}" y="{y+42}" width="26" height="12" fill="#b0b6bc"/>',
            t(x + 75, y + 32, label, 15, "#1b2536", "bold"),
            t(x + 75, y + 54, sub, 12, "#56677e")]


def notes(y0, items, warn):
    o = []
    for i, n in enumerate(items):
        y = y0 + i * 40
        o += [f'<rect x="50" y="{y-25}" width="{W-100}" height="34" rx="8" fill="#f2f5f9"/>', t(70, y - 2, n, 15, "#1b2536", anchor="start")]
    y = y0 + len(items) * 40
    o += [f'<rect x="50" y="{y-25}" width="{W-100}" height="34" rx="8" fill="#fdecea"/>', t(70, y - 2, warn, 15, "#9a1c12", "bold", "start")]
    return o


def legend(y, ac=True, wires="모든 연결은 암-암 점퍼선 · 브레드보드 없음"):
    o = [t(50, y, "선 색", 13, "#56677e", "bold", "start")]
    x = 100
    items = [(C_SIG, "신호(S)"), (C_5V, "5V (V)"), (C_3V, "3.3V"), (C_GND, "GND (G)")] + ([(C_AC, "220V (교사 작업)")] if ac else [])
    for c, lab in items:
        o += [f'<path d="M{x} {y-5} h34" stroke="{c}" stroke-width="6" stroke-linecap="round"/>', t(x + 42, y, lab, 13, "#1b2536", anchor="start")]
        x += 150
    o.append(t(W - 50, y, wires, 13, "#56677e", anchor="end"))
    return o


def svg_doc(body, title, subtitle):
    return "\n".join([f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" viewBox="0 0 {W} {H}" font-family="{FONT}">',
                      f'<rect width="{W}" height="{H}" fill="#ffffff"/>',
                      t(W / 2, 40, title, 28, "#1b2536", "bold"), t(W / 2, 68, subtitle, 15, "#56677e")] + body + ["</svg>"])


def ldr_and_pir(o, pir_xy, ldr_xy):
    po, pp = pir_module(*pir_xy)
    lo, lp = ldr_module(*ldr_xy)
    # PIR — D25 줄 (모듈 핀 순서가 헤더와 다름 → 이름 보고 한 가닥씩)
    o += wire(pin_xy("D25", "V"), pp["VCC"], C_5V)
    o += wire(pin_xy("D25", "S"), pp["OUT"], C_SIG)
    o += wire(pin_xy("D25", "G"), pp["GND"], C_GND)
    # 조도 — D34 줄의 S·G + 3.3V 헤더
    o += wire(pin_xy("D34", "S"), lp["AO"], C_SIG)
    o += wire(pin_xy("D34", "G"), lp["GND"], C_GND)
    v33 = P(PWR_COL["3.3V"], PWR_ROWS[3])
    o += wire(v33, lp["VCC"], C_3V, via=(OX + 380 * K + 50, OY + 330 * K))
    o += po + lo
    for kind in ("G", "V", "S"):
        o += dupont(*pin_xy("D25", kind))
    for kind in ("G", "S"):
        o += dupont(*pin_xy("D34", kind))
    o += dupont(*v33)
    return v33


def power_in(o, label):
    ux, uy = P(263, 52)
    o += adapter(700, 118, label)
    o += [f'<path d="M700 155 C 600 155, {ux:.0f} {uy-90:.0f}, {ux:.0f} {uy:.0f}" fill="none" stroke="#333" stroke-width="7" stroke-linecap="round"/>']


def row_tags(o, rows):
    """배선 위에 쓰는 줄 이름표 — 전선에 가려지지 않게 맨 위에"""
    for r in rows:
        x, y = P(363, row_y(r))
        o += [f'<rect x="{x-20:.0f}" y="{y-11:.0f}" width="42" height="21" rx="4" fill="#f5d75d" stroke="#3a3000"/>',
              t(x + 1, y + 4, r, 12, "#3a3000", "bold")]


def jumper_callout(o):
    jx, jy = P(289, 129)
    o += [f'<circle cx="{jx:.0f}" cy="{jy:.0f}" r="30" fill="none" stroke="#f5d75d" stroke-width="4"/>',
          f'<path d="M{jx+28:.0f} {jy-10:.0f} L 650 {jy-60:.0f}" stroke="#b8860b" stroke-width="3"/>']
    o += badge(650, jy - 60, "1", "전압 점퍼 → 5V 쪽", "#b8860b")


def v1():
    o = board("5V")
    ro, rp, rt = relay_module(830, 360)
    # 릴레이 — D26 줄
    o += wire(pin_xy("D26", "S"), rp["S"], C_SIG)
    o += wire(pin_xy("D26", "V"), rp["+"], C_5V)
    o += wire(pin_xy("D26", "G"), rp["−"], C_GND)
    o += ro
    for kind in ("G", "V", "S"):
        o += dupont(*pin_xy("D26", kind))
    ldr_and_pir(o, (860, 630), (880, 870))
    # 220V 램프 (교사 작업)
    bx, by = 1360, 200            # 전구 중심
    px, py = 1360, 540            # 콘센트 플러그
    com, no = rt["COM"], rt["NO"]
    o += [f'<path d="M{com[0]} {com[1]} C 1230 {com[1]}, 1240 {py}, {px-40} {py}" fill="none" stroke="{C_AC}" stroke-width="6"/>',
          f'<path d="M{no[0]} {no[1]} C 1230 {no[1]}, 1250 {by+60}, {bx-12} {by+62}" fill="none" stroke="{C_AC}" stroke-width="6"/>',
          f'<path d="M{px+20} {py-22} C {px+70} {py-120}, {bx+70} {by+120}, {bx+12} {by+62}" fill="none" stroke="#3b6ea5" stroke-width="6" stroke-dasharray="14 8"/>',
          f'<circle cx="{bx}" cy="{by}" r="44" fill="#fff6c8" stroke="#d4b100" stroke-width="3"/>',
          f'<rect x="{bx-16}" y="{by+40}" width="32" height="30" rx="4" fill="#9aa1a8"/>',
          t(bx, by + 6, "전구", 15, "#6b5a00", "bold"),
          f'<rect x="{px-40}" y="{py-22}" width="80" height="44" rx="8" fill="#e8e8e8" stroke="#777"/>',
          t(px, py + 6, "220V", 15, "#333", "bold"),
          t(px, py + 42, "콘센트", 13, "#333"),
          t(1150, py + 36, "L(활선) → COM", 13, C_AC, "bold", "start"),
          t(1215, by + 52, "NO → 소켓", 13, C_AC, "bold", "start"),
          t(bx + 70, by + 200, "N(중성선)", 13, "#3b6ea5", "bold", "start")]
    o += badge(1210, 610, "!", "220V는 교사 작업", "#c62828", 210)
    # 전원
    power_in(o, "5V 1A 충전기")
    jumper_callout(o)
    o += badge(830, 312, "2", "릴레이 → D26 줄 (S·+·−)")
    o += badge(1130, 700, "3", "PIR → D25 줄")
    o += badge(1100, 930, "4", "조도 → D34 줄 + VCC는 3.3V 헤더")
    row_tags(o, ["D26", "D25", "D34"])
    o += legend(1040)
    o += notes(1085, ["① 모듈 핀 이름을 보고 S·V·G에 한 가닥씩 — 릴레이(S·+·−)는 순서가 같지만 PIR은 다르니 확인",
                      "② 업로드는 DevKit USB(PC)만, 완성 후에는 베이스보드 USB-C(충전기)만 — 두 곳 동시 연결 금지"],
                 "⚠ 릴레이 단자(COM·NO)와 220V 램프 코드는 교사가 연결하고 절연 — 학생은 보드 쪽 저전압 배선만")
    return svg_doc(o, "IoT 무드등 V1 — 릴레이 전구형 실물 배선도",
                   "ESP32 DevKit 30핀 + 확장 베이스보드 · 릴레이 GPIO26 · PIR GPIO25 · 조도 GPIO34 · 동작 검증 완료")


def v2():
    o = board("5V")
    no, npads = neopixel_ring(1080, 250)
    # 네오픽셀 — D27 줄 (DIN에 330Ω, 링에 1000µF)
    rx, ry = 860, 470
    o += wire(pin_xy("D27", "S"), (rx - 26, ry), C_SIG)
    o += wire((rx + 26, ry), npads["DIN"], C_SIG, bend=0.3)
    o += wire(pin_xy("D27", "V"), npads["5V"], C_5V)
    o += wire(pin_xy("D27", "G"), npads["GND"], C_GND)
    o += no + resistor(rx, ry)
    cx, cy = 1290, 420
    o += [f'<path d="M{npads["5V"][0]} {npads["5V"][1]-8} C {npads["5V"][0]} 330, {cx-40} 300, {cx-8} {cy-34}" fill="none" stroke="{C_5V}" stroke-width="3"/>',
          f'<path d="M{npads["GND"][0]} {npads["GND"][1]-8} C {npads["GND"][0]} 320, {cx} 290, {cx+8} {cy-34}" fill="none" stroke="{C_GND}" stroke-width="3"/>']
    o += capacitor(cx, cy)
    for kind in ("G", "V", "S"):
        o += dupont(*pin_xy("D27", kind))
    ldr_and_pir(o, (860, 630), (880, 870))
    power_in(o, "5V 2A 어댑터")
    jumper_callout(o)
    o += badge(1200, 130, "2", "네오픽셀 → D27 줄")
    o += badge(1130, 700, "3", "PIR → D25 줄")
    o += badge(1100, 930, "4", "조도 → D34 줄 + VCC는 3.3V 헤더")
    row_tags(o, ["D27", "D25", "D34"])
    o += legend(1040, ac=False)
    o += notes(1085, ["① 링에는 핀이 없어요 → DIN·5V·GND 패드에 암 커넥터 선을 납땜 · DIN 선 중간에 330Ω, 링 5V–GND에 1000µF",
                      "② 업로드는 DevKit USB(PC)만 (밝기 낮게), 완성 후에는 베이스보드 USB-C(5V 2A)만 — 두 곳 동시 연결 금지"],
                 "⚠ 전구는 빼고 램프의 220V 코드는 분리 — V2는 220V를 쓰지 않아요")
    return svg_doc(o, "IoT 무드등 V2 — 네오픽셀 컬러형 실물 배선도",
                   "ESP32 DevKit 30핀 + 확장 베이스보드 · 네오픽셀 GPIO27 · PIR GPIO25 · 조도 GPIO34 · 동작 검증 완료")


if __name__ == "__main__":
    for name, fn in (("wiring_v1_relay.svg", v1), ("wiring_v2_neopixel.svg", v2)):
        with open(os.path.join(HERE, name), "w", encoding="utf-8") as f:
            f.write(fn())
        print(name)
