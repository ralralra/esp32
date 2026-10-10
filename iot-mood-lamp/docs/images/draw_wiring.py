"""IoT 무드등 실물형 배선도 — Wemos D1 R32 + 아두이노 센서쉴드 V5.

    python3 draw_wiring.py      → wiring_v1_neopixel.svg / wiring_v2_relay.svg
PNG는 headless 크로미움으로 SVG를 1500×1190 으로 찍는다 (README 참고).
핀: 네오픽셀 27(실드 6) · 릴레이 26(실드 2) · PIR 25(실드 3) · 조도 34(A3)
"""
import math, os

HERE = os.path.dirname(os.path.abspath(__file__))
FONT = "Pretendard, 'Noto Sans CJK KR', 'WenQuanYi Zen Hei', sans-serif"
W, H = 1500, 1190
C_SIG, C_5V, C_3V, C_GND, C_AC = "#f2b705", "#d93025", "#ff7a00", "#222222", "#8d5524"


def t(x, y, s, size=14, color="#1b2536", weight="normal", anchor="middle", rot=None):
    w = ' font-weight="bold"' if weight == "bold" else ""
    tr = f' transform="rotate({rot} {x} {y})"' if rot is not None else ""
    return f'<text x="{x:.1f}" y="{y:.1f}" font-size="{size}"{w} fill="{color}" text-anchor="{anchor}"{tr}>{s}</text>'


# ── 배선 도구 ───────────────────────────────────────────────────────
def wire(a, b, color, w=5, bend=0.45):
    (x1, y1), (x2, y2) = a, b
    dx = (x2 - x1) * bend
    d = f"M{x1:.1f} {y1:.1f} C {x1+dx:.1f} {y1:.1f}, {x2-dx:.1f} {y2:.1f}, {x2:.1f} {y2:.1f}"
    return [f'<path d="{d}" fill="none" stroke="#000" stroke-opacity="0.25" stroke-width="{w+2}" stroke-linecap="round"/>',
            f'<path d="{d}" fill="none" stroke="{color}" stroke-width="{w}" stroke-linecap="round"/>']


def dupont(x, y):
    """헤더 위에 꽂힌 암 커넥터 하우징"""
    return [f'<rect x="{x-8}" y="{y-8}" width="16" height="16" rx="2" fill="#141414" stroke="#555" stroke-width="1"/>']


def module_pin(x, y, label):
    return [f'<rect x="{x-7}" y="{y-4}" width="14" height="8" fill="#d4af37"/>',
            t(x + 14, y + 4, label, 12, "#ffffff", "bold", "start")]


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
    o += [f'<rect x="{x+96}" y="{y+8}" width="40" height="8" rx="3" fill="#444"/>',
          f'<rect x="{x+150}" y="{y+6}" width="14" height="12" rx="2" fill="#333"/>',
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


def adapter(x, y, label, sub):
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


# ── 실드 위 핀 좌표 ─────────────────────────────────────────────────
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
    """V1 네오픽셀 — SEL 점퍼 뺌, EXT PWR로 5V (2포트 충전기)"""
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
    ex, ey = EXT_XY
    o += [f'<rect x="60" y="120" width="190" height="100" rx="14" fill="#f4f6f8" stroke="#9aa5b1" stroke-width="2"/>',
          t(155, 152, "2포트 USB 충전기", 15, "#1b2536", "bold"), t(155, 174, "2.4A 포트 → EXT PWR · 1A 포트 → 보드", 11, "#56677e"),
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
                      "② 링 패드에 암 커넥터 선 납땜 · DIN 선 중간 330Ω · 링 5V–GND에 1000µF · 조도센서 VCC만 Bluetooth 헤더 3V3"],
                 "⚠ SEL 점퍼를 꽂은 채로 EXT PWR에 전원을 넣지 마세요 — 보드 전원과 합선됩니다 · 전구는 빼고 램프 220V 코드는 분리")
    return svg_doc(o, "IoT 무드등 V1 — 네오픽셀 배선도 (Wemos D1 R32 + 센서쉴드 V5)",
                   "네오픽셀 GPIO27(실드 6) · PIR GPIO25(실드 3) · 조도 GPIO34(A3) · 220V 없음 — 학생이 전부 배선")


def v2():
    """V2 릴레이 전구 — SEL 점퍼 꽂음, 보드 micro USB 5V 1A"""
    o = shield(sel_on=True)
    ro, rp, rt = relay_module(830, 170)
    o += wire(dig(2, "S"), rp["S"], C_SIG)
    o += wire(dig(2, "V"), rp["+"], C_5V)
    o += wire(dig(2, "G"), rp["−"], C_GND)
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
    o += adapter(60, 150, "5V 1A 충전기", "micro USB 케이블")
    o += [f'<path d="M140 224 C 140 300, 20 330, 40 380" fill="none" stroke="#333" stroke-width="7" stroke-linecap="round"/>']
    o += [f'<circle cx="{SEL_XY[0]+12}" cy="{SEL_XY[1]}" r="34" fill="none" stroke="#f5d75d" stroke-width="4"/>']
    o += badge(250, 690, "1", "SEL 점퍼 꽂기 (보드 5V 사용)", "#b8860b")
    o += badge(830, 130, "2", "릴레이 S·+·− → 실드 2번 S·V·G")
    tags(o, [((dig(2, "G")[0], DIG_Y["G"] + 30), "2번"), ((dig(3, "G")[0] - 26, DIG_Y["G"] + 30), "3번"),
             ((ana(3, "G")[0], ANA_Y["G"] - 30), "A3")])
    o += legend(1040)
    o += notes(1085, ["① 실드 숫자는 우노 라벨 — 2번=GPIO26, 3번=GPIO25, A3=GPIO34 · 모듈 핀 이름을 보고 S·V·G에 한 가닥씩",
                      "② 실드의 V는 5V예요 → 조도센서 VCC만 Bluetooth 헤더의 3V3 핀으로 따로 연결"],
                 "⚠ 릴레이 단자(COM·NO)와 220V 램프 코드는 교사가 연결하고 절연 — 학생은 실드 쪽 저전압 배선만")
    return svg_doc(o, "IoT 무드등 V2 — 릴레이 전구 배선도 (Wemos D1 R32 + 센서쉴드 V5)",
                   "릴레이 GPIO26(실드 2) · PIR GPIO25(실드 3) · 조도 GPIO34(A3) · 220V 쪽은 교사 작업")


if __name__ == "__main__":
    for name, fn in (("wiring_v1_neopixel.svg", v1), ("wiring_v2_relay.svg", v2)):
        with open(os.path.join(HERE, name), "w", encoding="utf-8") as f:
            f.write(fn())
        print(name)
