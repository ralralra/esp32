const pptxgen = require("pptxgenjs");
const pres = new pptxgen();
pres.layout = "LAYOUT_WIDE"; // 13.33 x 7.5

const C = {
  text: "1F2937", sub: "6B7280", green: "059669", greenDark: "047857",
  greenLight: "ECFDF5", border: "E5E7EB", cardBg: "F9FAFB",
  codeBg: "0F172A", code: "E2E8F0", codeCmt: "94A3B8", codeHl: "6EE7B7",
  amber: "B45309", amberLight: "FEF3C7", red: "DC2626",
  blue: "1D4ED8", blueLight: "EFF6FF", white: "FFFFFF"
};
const KO = "Malgun Gothic";
const MONO = "Courier New";
const REPO = "github.com/ralralra/esp32";
const BASE = REPO + "/tree/main/rfid-attendance-system";
const GAS_URL = REPO + "/blob/main/wemos-d1-r32-projects/rfid-attendance-system/apps_script/relay.gs";
const INO_URL = REPO + "/blob/main/wemos-d1-r32-projects/rfid-attendance-system/firmware/rfid_attendance/rfid_attendance.ino";
const UID_URL = REPO + "/blob/main/wemos-d1-r32-projects/rfid-attendance-system/firmware/uid_test/uid_test.ino";

function newSlide() {
  const s = pres.addSlide();
  s.background = { color: C.white };
  return s;
}
function header(s, kicker, title) {
  s.addText(kicker, { x: 0.55, y: 0.28, w: 12.2, h: 0.32, fontFace: KO,
    fontSize: 11, bold: true, color: C.green, margin: 0 });
  s.addText(title, { x: 0.55, y: 0.58, w: 12.2, h: 0.62, fontFace: KO,
    fontSize: 25, bold: true, color: C.text, margin: 0 });
}
function footerUrl(s, label, url) {
  s.addText([
    { text: label + "  ", options: { color: C.sub, bold: true } },
    { text: url, options: { color: C.green } }
  ], { x: 0.55, y: 7.02, w: 12.2, h: 0.3, fontFace: KO, fontSize: 10, margin: 0 });
}
function card(s, x, y, w, h, fill, border) {
  s.addShape(pres.ShapeType.roundRect, { x, y, w, h, rectRadius: 0.06,
    fill: { color: fill || C.white }, line: { color: border || C.border, width: 1 } });
}
function numCircle(s, n, x, y, color) {
  s.addText(String(n), { shape: pres.ShapeType.ellipse, x, y, w: 0.4, h: 0.4,
    fill: { color: color || C.green }, align: "center", valign: "middle",
    fontFace: KO, fontSize: 13, bold: true, color: C.white, margin: 0, line: { type: "none" } });
}
function chip(s, label, x, y, w, bg, fg) {
  s.addText(label, { shape: pres.ShapeType.roundRect, rectRadius: 0.05,
    x, y, w, h: 0.32, fill: { color: bg }, color: fg, fontFace: KO,
    fontSize: 10, bold: true, align: "center", valign: "middle", margin: 0, line: { type: "none" } });
}
function codeBox(s, x, y, w, h, lines, fontSize) {
  const runs = lines.map((ln, i) => {
    let color = C.code;
    if (ln.trim().startsWith("//")) color = C.codeCmt;
    else if (i === 0) color = C.codeHl;
    return { text: ln, options: { color, breakLine: true } };
  });
  s.addText(runs, { shape: pres.ShapeType.roundRect, rectRadius: 0.06,
    x, y, w, h, fill: { color: C.codeBg }, fontFace: MONO,
    fontSize: fontSize || 10.5, align: "left", valign: "top",
    margin: 10, lineSpacingMultiple: 1.14, line: { type: "none" } });
}
function bullets(s, items, x, y, w, h, fontSize) {
  const runs = [];
  items.forEach((it, i) => {
    runs.push({ text: it.t, options: { bullet: { code: "2022", indent: 12 },
      bold: !!it.b, color: it.c || C.text, breakLine: i < items.length - 1,
      paraSpaceAfter: 8 } });
  });
  s.addText(runs, { x, y, w, h, fontFace: KO, fontSize: fontSize || 13,
    align: "left", valign: "top", margin: 0, lineSpacingMultiple: 1.1 });
}

// ═════ 1. 표지 ═════
{
  const s = newSlide();
  s.addText("ESP32 × RFID × 웹앱 연동 교육지도서", { x: 0.9, y: 1.4, w: 11.5, h: 0.5,
    fontFace: KO, fontSize: 16, bold: true, color: C.green, margin: 0 });
  s.addText("스마트 출석체크 시스템 만들기", { x: 0.9, y: 1.9, w: 11.5, h: 1.0,
    fontFace: KO, fontSize: 40, bold: true, color: C.text, margin: 0 });
  s.addText("학생증(NFC) 태그 한 번으로 실시간 출석 확인까지 — 하드웨어·서버·앱 전 과정", {
    x: 0.9, y: 2.95, w: 11.5, h: 0.5, fontFace: KO, fontSize: 16, color: C.sub, margin: 0 });
  const parts = [
    ["하드웨어", "Wemos D1 R32 (ESP32)\n+ RC522 RFID 리더"],
    ["중계 서버", "Google Apps Script\n+ 구글 시트 DB"],
    ["앱", "스마트 출석체크 웹앱\n(AI Studio · Cloud Run)"]
  ];
  parts.forEach((p, i) => {
    const x = 0.9 + i * 4.0;
    card(s, x, 4.0, 3.7, 1.55, C.greenLight, "D1FAE5");
    s.addText(p[0], { x: x + 0.25, y: 4.18, w: 3.2, h: 0.35, fontFace: KO,
      fontSize: 13, bold: true, color: C.greenDark, margin: 0 });
    s.addText(p[1], { x: x + 0.25, y: 4.58, w: 3.2, h: 0.8, fontFace: KO,
      fontSize: 12.5, color: C.text, margin: 0, lineSpacingMultiple: 1.15 });
  });
  s.addText([
    { text: "전체 코드 · 자료  ", options: { color: C.sub, bold: true } },
    { text: BASE, options: { color: C.green, bold: true } }
  ], { x: 0.9, y: 6.2, w: 11.5, h: 0.4, fontFace: KO, fontSize: 13, margin: 0 });
  s.addNotes("이 자료는 실제로 제작·검증한 시스템을 바탕으로 한 교육지도서입니다. 하드웨어(ESP32+RC522), 중계 서버(Apps Script), 웹앱 세 부분이 어떻게 연결되는지가 전체 주제입니다. 학생들에게 '학교에서 매일 하는 출석'을 직접 자동화한다는 점을 강조하면 동기 부여에 좋습니다.");
}

// ═════ 2. 학습 목표 & 차시 구성 ═════
{
  const s = newSlide();
  header(s, "교육 안내", "학습 목표와 차시 구성");
  bullets(s, [
    { t: "IoT 시스템의 3층 구조(장치–서버–앱)를 직접 만들며 이해한다", b: true },
    { t: "RFID/NFC의 원리와 카드 고유번호(UID)의 개념을 설명할 수 있다" },
    { t: "클라우드 중계 서버(Apps Script)로 서로 다른 네트워크의 장치를 연결한다" },
    { t: "센서 입력 → 서버 처리 → 데이터 저장 → 화면 표시의 전체 데이터 흐름을 추적한다" }
  ], 0.6, 1.7, 5.6, 3.2, 13);
  const lessons = [
    ["1차시", "개념 + 배선", "RFID 원리, 시스템 구조 이해, RC522 배선"],
    ["2차시", "카드 읽기", "uid_test 업로드, 학생증 UID 확인 실습"],
    ["3차시", "중계 서버", "구글 시트 + relay.gs 배포, 브라우저 테스트"],
    ["4차시", "앱 연동", "앱 생성·완성 펌웨어 업로드, 등록·출석 실습"]
  ];
  lessons.forEach((l, i) => {
    const y = 1.7 + i * 1.28;
    card(s, 6.6, y, 6.1, 1.12, C.white, C.border);
    chip(s, l[0], 6.85, y + 0.4, 0.85, C.greenLight, C.greenDark);
    s.addText(l[1], { x: 7.9, y: y + 0.14, w: 4.6, h: 0.4, fontFace: KO,
      fontSize: 13, bold: true, color: C.text, margin: 0 });
    s.addText(l[2], { x: 7.9, y: y + 0.52, w: 4.6, h: 0.5, fontFace: KO,
      fontSize: 11, color: C.sub, margin: 0 });
  });
  card(s, 0.6, 5.3, 5.6, 1.5, C.cardBg, C.border);
  s.addText("권장 대상 · 시간", { x: 0.85, y: 5.45, w: 5.1, h: 0.35, fontFace: KO,
    fontSize: 12.5, bold: true, color: C.text, margin: 0 });
  s.addText("중·고등 정보/동아리, 메이커 교육 (아두이노 기초 경험 권장)\n차시당 45~50분 기준 4차시. 3~4명 팀 활동에 적합",
    { x: 0.85, y: 5.82, w: 5.1, h: 0.9, fontFace: KO, fontSize: 11, color: C.sub,
      margin: 0, lineSpacingMultiple: 1.2 });
  s.addNotes("차시 구성은 예시입니다. 하드웨어 경험이 없는 학급이라면 1~2차시를 늘려 배선과 시리얼 모니터 사용법에 시간을 더 쓰세요. 3차시(서버)는 눈에 보이는 결과가 적어 지루할 수 있으니, 브라우저 주소창 테스트로 '내가 서버에 말을 걸고 있다'는 경험을 만들어 주는 것이 핵심입니다.");
}

// ═════ 3. 완성 모습 ═════
{
  const s = newSlide();
  header(s, "무엇을 만드는가", "완성된 시스템의 세 장면");
  const scenes = [
    ["① 출석체크 시작", "선생님이 앱에서 교시와 대상 학생을 고르고 [출석체크 시작]을 누릅니다. 앱에 30초 카운트다운이 뜨고, 교실의 ESP32 리더기 LED가 깜빡이기 시작합니다."],
    ["② 학생증 태그", "학생이 자기 학생증을 리더기에 1~2초 붙입니다. 부저가 '삑삑' 울리면 성공. 미등록 카드나 다른 학생 카드는 '삐—' 소리로 거절됩니다."],
    ["③ 실시간 확인", "앱 화면이 곧바로 \"OO 학생의 1교시 출석이 확인되었습니다\"로 바뀌고, 인증 시각과 출석/지각 상태가 표시됩니다. 구글 시트에도 자동으로 기록됩니다."]
  ];
  scenes.forEach((sc, i) => {
    const x = 0.6 + i * 4.17;
    card(s, x, 1.7, 3.92, 3.3, C.white, C.border);
    s.addText(sc[0], { x: x + 0.28, y: 1.95, w: 3.4, h: 0.4, fontFace: KO,
      fontSize: 14.5, bold: true, color: C.greenDark, margin: 0 });
    s.addText(sc[1], { x: x + 0.28, y: 2.45, w: 3.4, h: 2.4, fontFace: KO,
      fontSize: 11.5, color: C.text, margin: 0, lineSpacingMultiple: 1.25 });
  });
  card(s, 0.6, 5.35, 12.15, 1.3, C.greenLight, "D1FAE5");
  s.addText("등록도 같은 방식", { x: 0.88, y: 5.5, w: 11.5, h: 0.35, fontFace: KO,
    fontSize: 13, bold: true, color: C.greenDark, margin: 0 });
  s.addText("앱에서 학생 정보(이름·학년·반·번호)를 입력하고 [등록 시작] → 새 학생증을 태그하면 그 카드가 그 학생의 카드로 저장됩니다. 이후 출석 때는 카드만 대면 누구인지 자동으로 알아봅니다.",
    { x: 0.88, y: 5.87, w: 11.5, h: 0.7, fontFace: KO, fontSize: 11.5, color: C.text,
      margin: 0, lineSpacingMultiple: 1.2 });
  s.addNotes("수업 도입에서 이 세 장면을 시연(또는 영상)으로 먼저 보여주세요. '무엇이 만들어지는지'를 먼저 보면 이후의 배선·코드 학습에 목적이 생깁니다. 시뮬레이션 버튼이 있어 하드웨어 없이도 앱 흐름 시연이 가능합니다.");
}

// ═════ 4. 개념: RFID/NFC ═════
{
  const s = newSlide();
  header(s, "개념 이해 ①", "RFID/NFC — 배터리 없는 카드가 대답하는 원리");
  bullets(s, [
    { t: "리더기가 13.56MHz 전파(자기장)를 계속 내보냄 → 카드 속 코일에 전기가 유도됨 → 카드 칩이 그 전기로 깨어나 자기 번호를 답함", b: true },
    { t: "그래서 카드에는 배터리가 없고, 리더기에 가까이(수 cm) 대야만 동작" },
    { t: "UID = 카드마다 새겨진 고유번호 (예: 6F3178D9). 이 시스템은 UID만 읽음 — 교통카드 잔액 등 내부 데이터는 건드리지 않음" },
    { t: "RC522는 ISO 14443 Type A 방식만 읽음 — 학생증이 Type A인지 폰 NFC Tools 앱으로 미리 확인 (\"NfcA\" 표시)" },
    { t: "스마트폰의 NFC는 보안상 매번 UID가 바뀌므로 카드 대신 쓸 수 없음", c: C.red }
  ], 0.6, 1.7, 6.6, 4.8, 12.5);
  // 오른쪽 미니 다이어그램
  card(s, 7.5, 1.9, 5.2, 4.3, C.cardBg, C.border);
  card(s, 7.9, 2.4, 1.9, 1.4, C.greenLight, "A7F3D0");
  s.addText("RC522\n리더기", { x: 7.9, y: 2.55, w: 1.9, h: 1.1, fontFace: KO, fontSize: 12,
    bold: true, color: C.greenDark, align: "center", margin: 0, lineSpacingMultiple: 1.1 });
  card(s, 10.4, 2.4, 1.9, 1.4, C.blueLight, "BFDBFE");
  s.addText("학생증\n(코일+칩)", { x: 10.4, y: 2.55, w: 1.9, h: 1.1, fontFace: KO, fontSize: 12,
    bold: true, color: C.blue, align: "center", margin: 0, lineSpacingMultiple: 1.1 });
  s.addShape(pres.ShapeType.line, { x: 9.85, y: 2.8, w: 0.5, h: 0,
    line: { color: C.green, width: 2, endArrowType: "arrow" } });
  s.addShape(pres.ShapeType.line, { x: 10.35, y: 3.35, w: -0.5, h: 0,
    line: { color: C.blue, width: 2, endArrowType: "arrow" } });
  s.addText("전파(전력+질문)", { x: 9.0, y: 2.42, w: 2.2, h: 0.3, fontFace: KO,
    fontSize: 9, color: C.green, align: "center", margin: 0 });
  s.addText("UID 응답", { x: 9.0, y: 3.42, w: 2.2, h: 0.3, fontFace: KO,
    fontSize: 9, color: C.blue, align: "center", margin: 0 });
  s.addText("우리가 쓰는 것 = UID 하나\n\n\"이 UID가 태그되었다\"는 사실만 서버로 보내고, '누구의 카드인지'는 서버(학생명단 시트)가 판단합니다. 장치는 단순하게, 판단은 서버가 — IoT 설계의 기본 원칙입니다.",
    { x: 7.85, y: 4.1, w: 4.5, h: 2.0, fontFace: KO, fontSize: 11, color: C.sub,
      margin: 0, lineSpacingMultiple: 1.25 });
  s.addNotes("실습 전 개념 수업용 슬라이드입니다. '배터리 없는 카드가 어떻게 대답할까?'를 발문으로 시작하세요. 전자기 유도는 무선충전과 같은 원리라고 연결해주면 이해가 빠릅니다. Type A/B 구분은 '우리 리더기가 읽을 수 있는 카드인지 폰으로 미리 검사한다'는 실용적 포인트로 정리하면 충분합니다.");
}

// ═════ 5. 시스템 구조 ═════
{
  const s = newSlide();
  header(s, "개념 이해 ②", "왜 중계 서버가 필요한가 — 시스템 전체 구조");
  card(s, 0.7, 1.75, 3.3, 1.5, C.blueLight, "BFDBFE");
  s.addText("스마트 출석체크 앱", { x: 0.7, y: 1.95, w: 3.3, h: 0.4, fontFace: KO,
    fontSize: 14, bold: true, color: C.blue, align: "center", margin: 0 });
  s.addText("AI Studio · Cloud Run\n(인터넷)", { x: 0.7, y: 2.38, w: 3.3, h: 0.7,
    fontFace: KO, fontSize: 11.5, color: C.sub, align: "center", margin: 0 });
  card(s, 5.0, 1.65, 3.35, 1.7, C.greenLight, "A7F3D0");
  s.addText("Apps Script 중계 서버", { x: 5.0, y: 1.88, w: 3.35, h: 0.4, fontFace: KO,
    fontSize: 14, bold: true, color: C.greenDark, align: "center", margin: 0 });
  s.addText("웹 앱 URL 하나로\n양쪽 요청을 모두 처리", { x: 5.0, y: 2.3, w: 3.35, h: 0.8,
    fontFace: KO, fontSize: 11.5, color: C.sub, align: "center", margin: 0 });
  card(s, 9.35, 1.75, 3.3, 1.5, C.amberLight, "FDE68A");
  s.addText("ESP32 + RC522", { x: 9.35, y: 1.95, w: 3.3, h: 0.4, fontFace: KO,
    fontSize: 14, bold: true, color: C.amber, align: "center", margin: 0 });
  s.addText("교실 와이파이 안\n(외부에서 접속 불가)", { x: 9.35, y: 2.38, w: 3.3, h: 0.7,
    fontFace: KO, fontSize: 11.5, color: C.sub, align: "center", margin: 0 });
  s.addShape(pres.ShapeType.line, { x: 4.05, y: 2.5, w: 0.9, h: 0,
    line: { color: C.blue, width: 2.2, beginArrowType: "arrow", endArrowType: "arrow" } });
  s.addText("세션 시작 · 결과 폴링(1초)", { x: 3.35, y: 2.02, w: 2.3, h: 0.3,
    fontFace: KO, fontSize: 9.5, color: C.blue, align: "center", margin: 0 });
  s.addShape(pres.ShapeType.line, { x: 8.4, y: 2.5, w: 0.9, h: 0,
    line: { color: C.amber, width: 2.2, beginArrowType: "arrow", endArrowType: "arrow" } });
  s.addText("명령 폴링(2초) · UID 전송", { x: 7.7, y: 2.02, w: 2.3, h: 0.3,
    fontFace: KO, fontSize: 9.5, color: C.amber, align: "center", margin: 0 });
  s.addShape(pres.ShapeType.line, { x: 6.67, y: 3.36, w: 0, h: 0.6,
    line: { color: C.green, width: 2.2, beginArrowType: "arrow", endArrowType: "arrow" } });
  card(s, 5.0, 3.98, 3.35, 1.25, C.white, C.border);
  s.addText("구글 시트 (데이터베이스)", { x: 5.0, y: 4.14, w: 3.35, h: 0.35,
    fontFace: KO, fontSize: 12.5, bold: true, color: C.text, align: "center", margin: 0 });
  s.addText("학생명단 · 출석기록 · 교시설정", { x: 5.0, y: 4.52, w: 3.35, h: 0.35,
    fontFace: KO, fontSize: 11, color: C.sub, align: "center", margin: 0 });
  card(s, 0.7, 5.45, 6.0, 1.45, C.cardBg, C.border);
  s.addText("직접 통신이 불가능한 이유", { x: 0.95, y: 5.6, w: 5.6, h: 0.35,
    fontFace: KO, fontSize: 13, bold: true, color: C.text, margin: 0 });
  s.addText("앱은 인터넷(Cloud Run)에, ESP32는 교실 와이파이(사설망) 안에 있어 서로의 주소를 모릅니다. 그래서 둘 다 접속할 수 있는 Apps Script 웹앱을 가운데에 둡니다.",
    { x: 0.95, y: 5.97, w: 5.6, h: 0.85, fontFace: KO, fontSize: 11.5, color: C.sub,
      margin: 0, lineSpacingMultiple: 1.15 });
  card(s, 6.95, 5.45, 5.7, 1.45, C.cardBg, C.border);
  s.addText("왜 폴링(주기적으로 묻기) 방식인가", { x: 7.2, y: 5.6, w: 5.3, h: 0.35,
    fontFace: KO, fontSize: 13, bold: true, color: C.text, margin: 0 });
  s.addText("Apps Script는 서버가 먼저 연락(푸시)할 수 없습니다. 대신 앱은 1초, ESP32는 2초마다 \"할 일 있나요?\"를 물어보는, 단순하지만 매우 안정적인 구조를 씁니다.",
    { x: 7.2, y: 5.97, w: 5.3, h: 0.85, fontFace: KO, fontSize: 11.5, color: C.sub,
      margin: 0, lineSpacingMultiple: 1.15 });
  footerUrl(s, "전체 자료", BASE);
  s.addNotes("이 구조도가 전체 수업의 지도(地圖)입니다. 이후 모든 코드는 이 그림의 화살표 중 하나를 구현하는 것임을 반복해서 짚어주세요. '집 와이파이의 공유기 안 기기에 밖에서 접속 못 하는 것과 같다'는 비유가 효과적입니다. 우체국(중계 서버)에 편지를 맡기고 서로 찾아가는 구조라고 설명해도 좋습니다.");
}

// ═════ 6. 동작 흐름 ═════
{
  const s = newSlide();
  header(s, "개념 이해 ③", "출석체크 한 번의 전체 흐름 (30초 세션)");
  const steps = [
    ["앱", C.blue, C.blueLight, "출석체크 시작", "선생님이 교시·대상 학생을 고르고 버튼을 누르면 앱이 중계서버에 세션 생성을 요청 (action=start)"],
    ["중계서버", C.green, C.greenLight, "세션 생성", "\"DEV001 리더기, 30초 대기\" 세션을 캐시에 저장. 교시설정 시트에서 지각 기준 시각도 함께 조회"],
    ["ESP32", C.amber, C.amberLight, "폴링 → LED 깜빡", "2초마다 묻던 ESP32가 ATTEND 응답을 받으면 LED를 깜빡이며 카드 스캔 시작"],
    ["학생", C.sub, C.cardBg, "학생증 태그", "학생이 카드를 리더기에 1~2초 밀착. ESP32가 UID를 읽어 서버로 전송 (action=tag)"],
    ["중계서버", C.green, C.greenLight, "기록 + 지각 판정", "UID로 학생을 찾아 출석기록 시트에 저장. 지각 기준을 넘었으면 상태를 '지각'으로"],
    ["앱", C.blue, C.blueLight, "결과 표시", "1초마다 status를 묻던 앱이 done을 받으면 \"OO 학생 출석 확인\" + 시각·상태를 표시"]
  ];
  steps.forEach((st, i) => {
    const col = i % 3, row = Math.floor(i / 3);
    const x = 0.62 + col * 4.17, y = 1.6 + row * 2.62;
    card(s, x, y, 3.92, 2.4, C.white, C.border);
    numCircle(s, i + 1, x + 0.25, y + 0.25, st[1]);
    chip(s, st[0], x + 2.75, y + 0.28, 0.95, st[2], st[1]);
    s.addText(st[3], { x: x + 0.8, y: y + 0.26, w: 1.95, h: 0.4, fontFace: KO,
      fontSize: 13.5, bold: true, color: C.text, valign: "middle", margin: 0 });
    s.addText(st[4], { x: x + 0.28, y: y + 0.82, w: 3.4, h: 1.45, fontFace: KO,
      fontSize: 11, color: C.sub, margin: 0, lineSpacingMultiple: 1.15 });
  });
  s.addNotes("6단계를 학생들이 역할극으로 재연해 보게 하면 확실히 각인됩니다(앱 역할, 서버 역할, ESP32 역할, 학생 역할). 각 단계의 색 칩(앱=파랑, 서버=초록, ESP32=주황)은 이후 코드 슬라이드에서도 동일하게 쓰입니다.");
}

// ═════ 7. 준비물 ═════
{
  const s = newSlide();
  header(s, "1차시 · 준비", "준비물 체크리스트");
  const hw = [
    ["Wemos D1 R32", "ESP32 보드 (우노 모양). WROOM-32 DevKit도 동일 코드로 동작"],
    ["RC522 리더 모듈", "13.56MHz RFID/NFC. 핀헤더 납땜이 되어 있는지 확인"],
    ["점퍼선 (암-암) 7개", "배선용. 색을 다르게 준비하면 실수가 줄어듦"],
    ["수동부저 (선택)", "성공/실패 소리 피드백. 없어도 시스템은 동작"],
    ["USB 케이블", "데이터 전송 가능한 케이블 (충전 전용 불가)"]
  ];
  const sw = [
    ["Arduino IDE + esp32 보드 패키지", "보드: \"ESP32 Dev Module\" 선택"],
    ["MFRC522 라이브러리", "라이브러리 매니저에서 설치 (by GithubCommunity)"],
    ["2.4GHz 와이파이", "ESP32는 5GHz 접속 불가 — 반드시 2.4GHz SSID"],
    ["구글 계정", "시트 + Apps Script 배포용"],
    ["테스트 카드 + 학생증", "폰 NFC Tools 앱으로 NfcA 여부 사전 확인"]
  ];
  s.addText("하드웨어", { x: 0.6, y: 1.55, w: 6.0, h: 0.4, fontFace: KO, fontSize: 15,
    bold: true, color: C.greenDark, margin: 0 });
  hw.forEach((h, i) => {
    const y = 2.05 + i * 0.94;
    card(s, 0.6, y, 6.0, 0.82, C.white, C.border);
    s.addText(h[0], { x: 0.85, y: y + 0.08, w: 5.5, h: 0.32, fontFace: KO,
      fontSize: 12, bold: true, color: C.text, margin: 0 });
    s.addText(h[1], { x: 0.85, y: y + 0.4, w: 5.5, h: 0.35, fontFace: KO,
      fontSize: 10, color: C.sub, margin: 0 });
  });
  s.addText("소프트웨어 · 환경", { x: 6.95, y: 1.55, w: 6.0, h: 0.4, fontFace: KO,
    fontSize: 15, bold: true, color: C.greenDark, margin: 0 });
  sw.forEach((h, i) => {
    const y = 2.05 + i * 0.94;
    card(s, 6.95, y, 5.75, 0.82, C.white, C.border);
    s.addText(h[0], { x: 7.2, y: y + 0.08, w: 5.25, h: 0.32, fontFace: KO,
      fontSize: 12, bold: true, color: C.text, margin: 0 });
    s.addText(h[1], { x: 7.2, y: y + 0.4, w: 5.25, h: 0.35, fontFace: KO,
      fontSize: 10, color: C.sub, margin: 0 });
  });
  footerUrl(s, "세팅 가이드", REPO + "/blob/main/wemos-d1-r32-projects/esp32-class-projects/01_docs/setup_wemos_d1_r32.md");
  s.addNotes("수업 전 교사가 미리 확인할 것: ① 학교 와이파이가 2.4GHz인지, 로그인 페이지(캡티브 포털)형이면 ESP32 접속이 안 되므로 폰 핫스팟을 대안으로 준비 ② 학생증이 NfcA인지 폰으로 샘플 확인 ③ USB 케이블 중 충전 전용이 섞여 있지 않은지. 이 세 가지가 현장에서 가장 자주 발목을 잡습니다.");
}

// ═════ 8. 배선 ═════
{
  const s = newSlide();
  header(s, "1차시 · 실습", "배선 — RC522 ↔ Wemos D1 R32");
  bullets(s, [
    { t: "전원은 반드시 3V3 — 5V에 꽂으면 모듈 고장", b: true, c: C.red },
    { t: "RC522의 SDA(SS)는 보드의 SDA 핀이 아니라 IO05에 연결" },
    { t: "RC522의 RST는 보드의 RST 핀이 아니라 IO27에 연결" },
    { t: "IRQ는 연결하지 않음 · 부저(선택)는 IO04" },
    { t: "상태 LED는 보드 내장 파란 LED(IO02) 사용 — 따로 달 필요 없음" }
  ], 0.6, 1.65, 5.3, 2.8, 12);
  card(s, 0.6, 4.6, 5.3, 2.15, C.cardBg, C.border);
  s.addText("배선 검사 방법", { x: 0.85, y: 4.76, w: 4.8, h: 0.35, fontFace: KO,
    fontSize: 13, bold: true, color: C.text, margin: 0 });
  s.addText("uid_test 펌웨어 업로드 → 시리얼 모니터(115200)에서\n· 버전 레지스터 0x92 등 → 배선 정상\n· 0x00 / 0xFF → 배선 오류 (3V3·IO05·IO27 재확인)",
    { x: 0.85, y: 5.14, w: 4.85, h: 1.5, fontFace: KO, fontSize: 11.5, color: C.sub,
      margin: 0, lineSpacingMultiple: 1.2 });
  s.addImage({ path: "../images/rc522_wemos_d1_r32_wiring.png",
    x: 6.15, y: 1.6, w: 6.6, h: 4.01 });
  card(s, 6.15, 5.85, 6.6, 0.9, C.greenLight, "D1FAE5");
  s.addText("색깔별 연결: IO05→SDA · IO18→SCK · IO23→MOSI · IO19→MISO · IO27→RST · GND→GND · 3V3→3.3V",
    { x: 6.4, y: 5.98, w: 6.1, h: 0.65, fontFace: KO, fontSize: 11, color: C.greenDark,
      margin: 0, valign: "middle", lineSpacingMultiple: 1.15 });
  footerUrl(s, "배선 문서", REPO + "/blob/main/wemos-d1-r32-projects/rfid-attendance-system/docs/wiring.md");
  s.addNotes("가장 흔한 배선 실수 2가지를 칠판에 크게 적어두세요: ① RC522의 SDA를 보드의 SDA 핀에 꽂는 실수(이름이 같아서 매우 흔함 — 정답은 IO05) ② 5V 연결(모듈이 고장 남). 배선 후 전원을 켜기 전에 팀별로 교차 검사를 시키면 사고를 줄일 수 있습니다.");
}

// ═════ 9. uid_test setup ═════
{
  const s = newSlide();
  header(s, "2차시 · uid_test 함수 ①", "setup() — 리더기 자가진단");
  bullets(s, [
    { t: "업로드 후 시리얼 모니터만 보면 배선이 맞는지 즉시 판별 — 첫 실습에 딱 좋은 구조", b: true },
    { t: "VersionReg(버전 레지스터)를 읽어봄: 리더기가 SPI로 응답하면 0x92 같은 값, 배선이 틀리면 0x00/0xFF" },
    { t: "PCD_SetAntennaGain(RxGain_max): 수신 감도 최대 — 기본값(중간)으로는 인식 거리가 매우 짧음" },
    { t: "GsNReg/CWGsPReg 부스트: 송신 출력 최대 — 안테나가 작은 학생증·유스카드 인식의 핵심 설정" }
  ], 0.6, 1.75, 5.5, 4.5, 12);
  codeBox(s, 6.35, 1.6, 6.4, 5.2, [
    "void setup() {",
    "  Serial.begin(115200);",
    "  SPI.begin();      // 18/19/23번 핀 자동",
    "  rfid.PCD_Init();",
    "  // 수신 감도 최대 + 송신 출력 부스트",
    "  rfid.PCD_SetAntennaGain(MFRC522::RxGain_max);",
    "  rfid.PCD_WriteRegister(MFRC522::GsNReg, 0xF8);",
    "  rfid.PCD_WriteRegister(MFRC522::CWGsPReg,0x3F);",
    "",
    "  // 자가진단: 버전 레지스터 읽기",
    "  byte v = rfid.PCD_ReadRegister(",
    "    MFRC522::VersionReg);",
    "  Serial.print(\"RC522 버전 레지스터: 0x\");",
    "  Serial.println(v, HEX);",
    "",
    "  if (v == 0x00 || v == 0xFF)",
    "    Serial.println(\"배선을 확인하세요!\");",
    "  else",
    "    Serial.println(\"연결 OK. 카드를 대보세요\");",
    "}"
  ], 10.5);
  footerUrl(s, "코드", UID_URL);
  s.addNotes("'컴퓨터가 부품에게 이름을 물어보는 것'이 자가진단이라고 설명하세요. 0x92는 리더기 칩의 버전 번호일 뿐, 값 자체보다 '0x00/0xFF가 아니면 성공'이 포인트입니다. 여기서 실패한 팀은 배선 슬라이드로 돌아가 3V3·IO05·IO27 세 곳을 다시 확인하게 합니다.");
}

// ═════ 10. uid_test loop ═════
{
  const s = newSlide();
  header(s, "2차시 · uid_test 함수 ②", "loop() — 학생증 UID 읽기 실습");
  bullets(s, [
    { t: "카드가 감지되면 UID를 16진수 문자열로 출력 — \"카드 인식! UID = 6F3178D9\"", b: true },
    { t: "PICC_HaltA(): 읽은 카드와의 통신을 종료 — 다음 카드를 받을 준비" },
    { t: "1초 delay: 같은 카드가 연속으로 찍히는 것 방지" },
    { t: "실습 과제: 자기 학생증 UID를 읽어 기록해 두기 (4차시 등록 실습에서 맞는지 확인)", b: true },
    { t: "관찰 포인트: 인식 거리(몇 cm?), 카드 위치별 차이(중앙 vs 가장자리)" }
  ], 0.6, 1.75, 5.5, 4.5, 12);
  codeBox(s, 6.35, 1.6, 6.4, 5.2, [
    "void loop() {",
    "  // 카드가 있고 UID를 읽을 수 있으면",
    "  if (!rfid.PICC_IsNewCardPresent() ||",
    "      !rfid.PICC_ReadCardSerial()) {",
    "    delay(50);",
    "    return;",
    "  }",
    "",
    "  // UID → \"6F3178D9\" 형태 문자열",
    "  String uid = \"\";",
    "  for (byte i = 0; i < rfid.uid.size; i++) {",
    "    if (rfid.uid.uidByte[i] < 0x10)",
    "      uid += \"0\";",
    "    uid += String(rfid.uid.uidByte[i], HEX);",
    "  }",
    "  uid.toUpperCase();",
    "",
    "  Serial.println(\"카드 인식! UID = \" + uid);",
    "",
    "  rfid.PICC_HaltA();     // 카드 통신 종료",
    "  rfid.PCD_StopCrypto1();",
    "  delay(1000);           // 연속 인식 방지",
    "}"
  ], 10);
  footerUrl(s, "코드", UID_URL);
  s.addNotes("이 차시의 목표는 '내 학생증의 고유번호를 눈으로 본다'입니다. 16진수가 처음인 학생에게는 '0~9 다음에 A~F까지 쓰는 숫자 표기법' 정도로만 짚고 넘어가세요. 학생증이 안 읽히는 경우 카드 가장자리를 안테나 중앙에 밀착시키게 하고, 그래도 안 되면 폰 NFC Tools로 NfcA 여부를 확인시킵니다.");
}

// ═════ 11. 개념: 세션과 폴링 ═════
{
  const s = newSlide();
  header(s, "개념 이해 ④", "세션과 폴링 — 서버가 먼저 말을 걸 수 없을 때");
  card(s, 0.6, 1.65, 6.0, 2.4, C.white, C.border);
  s.addText("세션(Session) = \"지금 카드를 기다리는 중\"이라는 메모", { x: 0.88, y: 1.85, w: 5.5, h: 0.6,
    fontFace: KO, fontSize: 13.5, bold: true, color: C.text, margin: 0, lineSpacingMultiple: 1.1 });
  s.addText("앱이 [시작]을 누르면 서버가 \"DEV001 리더기, 출석 모드, 30초\"라는 메모를 캐시에 적어 둡니다. 캐시는 30초 뒤 자동으로 지워지므로, 별도의 타이머 코드 없이도 세션이 만료됩니다.",
    { x: 0.88, y: 2.5, w: 5.5, h: 1.4, fontFace: KO, fontSize: 11.5, color: C.sub,
      margin: 0, lineSpacingMultiple: 1.25 });
  card(s, 0.6, 4.3, 6.0, 2.4, C.white, C.border);
  s.addText("폴링(Polling) = 주기적으로 \"할 일 있나요?\" 묻기", { x: 0.88, y: 4.5, w: 5.5, h: 0.4,
    fontFace: KO, fontSize: 13.5, bold: true, color: C.text, margin: 0 });
  s.addText("ESP32는 2초마다 메모가 있는지 묻고, 있으면 카드 스캔을 시작합니다. 앱은 1초마다 결과가 나왔는지 묻습니다. 서버가 먼저 연락할 수 없는 환경에서 쓰는 가장 단순하고 확실한 방법입니다.",
    { x: 0.88, y: 4.92, w: 5.5, h: 1.6, fontFace: KO, fontSize: 11.5, color: C.sub,
      margin: 0, lineSpacingMultiple: 1.25 });
  // 오른쪽 타임라인
  card(s, 7.0, 1.65, 5.7, 5.05, C.cardBg, C.border);
  s.addText("시간 순서 예시", { x: 7.28, y: 1.82, w: 5.2, h: 0.35, fontFace: KO,
    fontSize: 13, bold: true, color: C.text, margin: 0 });
  const tl = [
    ["0초", "앱: start → 서버가 세션 메모 저장", C.blue],
    ["~2초", "ESP32: poll → \"ATTEND\" 받음 → LED 깜빡", C.amber],
    ["7초", "학생 태그 → ESP32: tag(UID) 전송", C.amber],
    ["7.5초", "서버: 학생 찾기·기록·지각 판정 → 세션 done", C.green],
    ["8초", "앱: status → done + 결과 받음 → 화면 표시", C.blue],
    ["~10초", "ESP32: 다음 poll → \"IDLE\" → 대기 복귀", C.amber]
  ];
  tl.forEach((t, i) => {
    const y = 2.3 + i * 0.72;
    chip(s, t[0], 7.28, y + 0.04, 0.8, C.white, t[2]);
    s.addText(t[1], { x: 8.25, y: y, w: 4.3, h: 0.6, fontFace: KO, fontSize: 11,
      color: C.text, valign: "middle", margin: 0, lineSpacingMultiple: 1.05 });
  });
  s.addNotes("추상적인 두 개념을 이 수업의 구체물로 연결하는 슬라이드입니다. 비유: 세션 = 급식실 배식대에 붙인 '지금 배식 중' 팻말(시간이 지나면 떼어짐), 폴링 = 아이들이 '지금 배식해요?'라고 계속 물어보러 오는 것. 오른쪽 타임라인을 손가락으로 짚으며 앞 슬라이드의 6단계와 대응시키세요.");
}

// ═════ 12. 시트 DB ═════
{
  const s = newSlide();
  header(s, "3차시 · 데이터베이스", "구글 시트 구성 — 3개 시트가 DB 역할");
  const sheets = [
    ["학생명단", "학생증 등록 DB", ["UID (카드 고유번호)", "이름", "학년 / 반 / 번호", "등록일시"],
      "등록 모드에서 태그하면 자동으로 한 줄씩 추가됩니다. UID가 곧 \"이 카드 = 이 학생\" 연결고리."],
    ["출석기록", "출석/지각 로그", ["날짜 / 시각", "교시", "UID / 이름", "상태 (출석·지각)", "리더기"],
      "출석 세션에서 태그할 때마다 자동 기록. 같은 날 같은 교시 중복 태그는 기록하지 않습니다."],
    ["교시설정", "지각 판정 기준", ["교시 (1~7)", "시작시각", "지각기준 (HH:MM)"],
      "기본 시간표가 자동으로 채워지며, 학교 시간표에 맞게 시트에서 직접 수정하면 바로 반영됩니다."]
  ];
  sheets.forEach((sh, i) => {
    const x = 0.6 + i * 4.17;
    card(s, x, 1.6, 3.92, 4.6, C.white, C.border);
    s.addText(sh[0], { x: x + 0.28, y: 1.85, w: 3.3, h: 0.4, fontFace: KO,
      fontSize: 16, bold: true, color: C.greenDark, margin: 0 });
    s.addText(sh[1], { x: x + 0.28, y: 2.28, w: 3.3, h: 0.3, fontFace: KO,
      fontSize: 11, color: C.sub, margin: 0 });
    bullets(s, sh[2].map(t => ({ t })), x + 0.3, 2.75, 3.4, 1.8, 11.5);
    s.addText(sh[3], { x: x + 0.28, y: 4.75, w: 3.4, h: 1.3, fontFace: KO,
      fontSize: 10.5, color: C.sub, margin: 0, lineSpacingMultiple: 1.15, italic: true });
  });
  card(s, 0.6, 6.35, 12.1, 0.55, C.greenLight, "D1FAE5");
  s.addText("빈 시트에서 시작해도 OK — 첫 요청이 들어오는 순간 세 시트를 한 번에 머리글과 함께 자동 생성합니다.",
    { x: 0.85, y: 6.35, w: 11.6, h: 0.55, fontFace: KO, fontSize: 11.5, bold: true,
      color: C.greenDark, margin: 0, valign: "middle" });
  footerUrl(s, "시트 구성 문서", REPO + "/blob/main/wemos-d1-r32-projects/rfid-attendance-system/apps_script/README.md");
  s.addNotes("'데이터베이스'라는 말이 어렵다면 '자동으로 채워지는 출석부 엑셀'이라고 소개하세요. 핵심 개념은 관계: 출석기록의 UID로 학생명단에서 이름을 찾는 구조(간단한 테이블 조인)를 화살표로 그려 보여주면, 이후 findStudent 함수를 이해하는 밑돌이 됩니다.");
}

// ═════ 13. GAS 배포 ═════
{
  const s = newSlide();
  header(s, "3차시 · 중계 서버", "Apps Script 배포 — 10분 완성");
  const steps = [
    ["새 시트 만들기", "sheets.new 로 빈 구글 시트 생성. 이름은 자유 (예: 스마트출석체크)"],
    ["Apps Script 열기", "시트 메뉴에서 확장 프로그램 → Apps Script. 기본 Code.gs 내용은 전부 삭제"],
    ["relay.gs 붙여넣기", "저장소의 relay.gs 전체를 붙여넣고 저장 (Ctrl+S)"],
    ["웹 앱으로 배포", "배포 → 새 배포 → 웹 앱. 실행: 나 / 액세스: 모든 사용자 → 배포 후 URL 복사"]
  ];
  steps.forEach((st, i) => {
    const x = 0.6 + i * 3.13;
    card(s, x, 1.65, 2.88, 2.5, C.white, C.border);
    numCircle(s, i + 1, x + 0.25, 1.9);
    s.addText(st[0], { x: x + 0.75, y: 1.9, w: 2.0, h: 0.65, fontFace: KO,
      fontSize: 13, bold: true, color: C.text, valign: "middle", margin: 0, lineSpacingMultiple: 1.05 });
    s.addText(st[1], { x: x + 0.26, y: 2.65, w: 2.38, h: 1.35, fontFace: KO,
      fontSize: 10.5, color: C.sub, margin: 0, lineSpacingMultiple: 1.15 });
  });
  card(s, 0.6, 4.5, 6.0, 2.2, C.amberLight, "FDE68A");
  s.addText("⚠ 꼭 기억하세요", { x: 0.85, y: 4.66, w: 5.5, h: 0.35, fontFace: KO,
    fontSize: 13, bold: true, color: C.amber, margin: 0 });
  s.addText("· 액세스 권한은 반드시 \"모든 사용자\" — ESP32는 구글 로그인을 못 합니다\n· 코드 수정 후에는 배포 관리 → 새 버전으로 재배포해야 반영\n· \"새 배포\"를 또 만들면 URL이 바뀌니 주의",
    { x: 0.85, y: 5.05, w: 5.5, h: 1.5, fontFace: KO, fontSize: 11.5, color: C.text,
      margin: 0, lineSpacingMultiple: 1.25 });
  card(s, 6.85, 4.5, 5.85, 2.2, C.cardBg, C.border);
  s.addText("배포 확인 (브라우저에서)", { x: 7.1, y: 4.66, w: 5.4, h: 0.35, fontFace: KO,
    fontSize: 13, bold: true, color: C.text, margin: 0 });
  s.addText([
    { text: "배포URL?action=periods\n", options: { fontFace: MONO, fontSize: 11.5, color: C.greenDark, bold: true } },
    { text: "→ 교시 목록 JSON이 나오면 성공. 이 순간 시트 3개가 자동 생성됩니다. 이 URL을 앱과 ESP32 펌웨어 양쪽에 넣습니다.",
      options: { fontFace: KO, fontSize: 11.5, color: C.sub } }
  ], { x: 7.1, y: 5.05, w: 5.4, h: 1.5, margin: 0, lineSpacingMultiple: 1.25, valign: "top" });
  footerUrl(s, "코드", GAS_URL);
  s.addNotes("권한 승인 팝업에서 '확인되지 않은 앱' 경고가 떠서 학생들이 당황합니다 — '고급 → 프로젝트로 이동'을 미리 안내하세요. 배포URL?action=periods를 처음 열었을 때 시트 탭 3개가 스르륵 생기는 순간이 이 차시의 하이라이트입니다. 화면을 미러링해 다같이 보세요.");
}

// ═════ 14. GAS 구조 ═════
{
  const s = newSlide();
  header(s, "3차시 · relay.gs 코드 ①", "코드 구조 — action 하나로 모든 요청을 나눈다");
  bullets(s, [
    { t: "웹 앱 URL은 하나뿐 — 요청의 action 파라미터로 기능을 구분합니다", b: true },
    { t: "앱용 6개: start(세션 시작) · status(결과) · cancel · students · records · periods" },
    { t: "ESP32용 2개: poll(할 일 조회) · tag(태그 보고)" },
    { t: "앱에는 JSON, ESP32에는 \"한 줄 텍스트\"로 응답 — ESP32에서 JSON 해석 라이브러리가 필요 없어짐" },
    { t: "doGet/doPost 모두 같은 route()로 — GET만 써도 전부 동작" }
  ], 0.6, 1.75, 5.5, 4.6, 12.5);
  codeBox(s, 6.35, 1.6, 6.4, 5.2, [
    "function route(e) {",
    "  var p = (e && e.parameter) || {};",
    "  switch (p.action) {",
    "    // ── 앱이 부르는 것 ──",
    "    case 'start':    return startSession(p);",
    "    case 'status':   return sessionStatus(p);",
    "    case 'cancel':   return cancelSession(p);",
    "    case 'students': return listStudents(p);",
    "    case 'records':  return listRecords(p);",
    "    case 'periods':  return listPeriods(p);",
    "    // ── ESP32가 부르는 것 ──",
    "    case 'poll':     return devicePoll(p);",
    "    case 'tag':      return deviceTag(p);",
    "    default: return text('ERR,unknown_action');",
    "  }",
    "}",
    "",
    "function doGet(e)  { return route(e); }",
    "function doPost(e) { return route(e); }"
  ], 11);
  footerUrl(s, "코드", GAS_URL);
  s.addNotes("switch문을 '민원 창구'에 비유하세요: 하나의 접수창구(URL)에 온 민원을 용건(action)에 따라 담당자(함수)에게 넘기는 구조. 학생 활동: 브라우저 주소창에서 action 값을 바꿔가며 호출해 보고, 없는 action을 넣으면 ERR가 오는 것까지 확인하게 하세요.");
}

// ═════ 15. startSession ═════
{
  const s = newSlide();
  header(s, "3차시 · relay.gs 코드 ②", "startSession() — 앱이 세션을 만든다");
  bullets(s, [
    { t: "앱의 [출석체크 시작]/[등록 시작] 버튼이 호출하는 함수", b: true },
    { t: "세션 정보를 CacheService(임시 저장소)에 저장 — timeout(기본 30초) 뒤 자동 삭제되어 앱의 30초 카운트다운과 정확히 일치" },
    { t: "출석 모드에서 지각 기준(lateAfter)을 안 넘기면 교시설정 시트에서 자동 조회" },
    { t: "리더기(device) 1대당 세션 1개 — 같은 키에 다시 쓰면 덮어씀" }
  ], 0.6, 1.75, 5.5, 4.6, 12.5);
  codeBox(s, 6.35, 1.6, 6.4, 5.2, [
    "function startSession(p) {",
    "  var timeout = Math.min(Math.max(",
    "    parseInt(p.timeout || '30', 10), 10), 300);",
    "",
    "  // 지각 기준: 없으면 교시설정 시트에서",
    "  var lateAfter = p.lateAfter || '';",
    "  if (p.mode==='attend' && !lateAfter && p.period)",
    "    lateAfter = findLateAfter(p.period);",
    "",
    "  var sess = { mode: p.mode, state: 'waiting',",
    "    device: p.device, name: p.name || '',",
    "    period: p.period || '',",
    "    lateAfter: lateAfter, result: null };",
    "",
    "  // timeout초 뒤 자동 만료되는 캐시에 저장",
    "  CacheService.getScriptCache().put(",
    "    cacheKey(p.device),",
    "    JSON.stringify(sess), timeout);",
    "  return json({ ok:true, timeout:timeout });",
    "}"
  ], 10.5);
  footerUrl(s, "코드", GAS_URL);
  s.addNotes("Math.min(Math.max(...))는 '10~300초 사이로 강제'하는 안전장치입니다 — 이상한 값이 들어와도 서버가 망가지지 않게 하는 입력 검증의 예로 소개하세요. state가 waiting으로 시작해 done으로 바뀌는 '상태 변화'가 이후 함수들을 관통하는 열쇠입니다.");
}

// ═════ 16. devicePoll / sessionStatus ═════
{
  const s = newSlide();
  header(s, "3차시 · relay.gs 코드 ③", "devicePoll() · sessionStatus() — 양쪽의 폴링 창구");
  bullets(s, [
    { t: "devicePoll: ESP32가 2초마다 호출. 응답은 IDLE / ATTEND / REGISTER 딱 한 단어", b: true },
    { t: "sessionStatus: 앱이 1초마다 호출. state가 done이 되면 result(이름·상태·시각)를 화면에 표시" },
    { t: "세션이 만료되면 캐시에서 사라져 자동으로 none/IDLE — 타이머 코드가 필요 없음" },
    { t: "같은 세션을 두 창구가 다른 형식(텍스트/JSON)으로 보여줄 뿐 — 데이터는 하나" }
  ], 0.6, 1.75, 5.5, 4.6, 12.5);
  codeBox(s, 6.35, 1.6, 6.4, 5.2, [
    "// ESP32용: 한 단어로만 답한다",
    "function devicePoll(p) {",
    "  var sess = loadSession(p.device);",
    "  if (!sess || sess.state !== 'waiting')",
    "    return text('IDLE');",
    "  return text(sess.mode === 'register'",
    "    ? 'REGISTER' : 'ATTEND');",
    "}",
    "",
    "// 앱용: 세션 상태를 JSON으로",
    "function sessionStatus(p) {",
    "  var sess = loadSession(p.device);",
    "  if (!sess) return json({ state: 'none' });",
    "  return json({ state: sess.state,",
    "    mode: sess.mode, result: sess.result });",
    "}",
    "",
    "// 캐시에서 세션 읽기 (만료되면 null)",
    "function loadSession(device) {",
    "  var raw = CacheService.getScriptCache()",
    "    .get(cacheKey(device));",
    "  return raw ? JSON.parse(raw) : null;",
    "}"
  ], 10);
  footerUrl(s, "코드", GAS_URL);
  s.addNotes("실습: 브라우저에서 start를 호출한 뒤 30초 안에 poll을 부르면 ATTEND, 30초 뒤에 부르면 IDLE이 나오는 것을 직접 확인시키세요. '캐시가 지워졌기 때문'이라는 인과가 눈으로 보입니다. 같은 데이터를 받는 쪽에 맞는 형식으로 다르게 포장한다는 점(텍스트 vs JSON)도 좋은 설계 토론거리입니다.");
}

// ═════ 17. deviceTag ═════
{
  const s = newSlide();
  header(s, "3차시 · relay.gs 코드 ④", "deviceTag() — 태그가 도착하면 벌어지는 일");
  bullets(s, [
    { t: "ESP32가 카드를 읽으면 ?action=tag&device=DEV001&uid=6F3178D9 형태로 호출", b: true },
    { t: "LockService: 두 태그가 동시에 도착해도 시트가 꼬이지 않게 잠금" },
    { t: "세션 모드에 따라 doRegister(등록) 또는 doAttend(출석)로 분기" },
    { t: "결과를 세션에 담고 state를 done으로 → 앱이 status 폴링으로 가져감 (60초 유지)" },
    { t: "반환값은 ESP32의 부저를 결정: OK(삑삑) / UNKNOWN·DUP·MISMATCH·ALREADY(삐—)" }
  ], 0.6, 1.75, 5.5, 4.7, 12);
  codeBox(s, 6.35, 1.6, 6.4, 5.2, [
    "function deviceTag(p) {",
    "  var lock = LockService.getScriptLock();",
    "  lock.waitLock(5000);  // 동시 태그 보호",
    "  try {",
    "    var sess = loadSession(p.device);",
    "    if (!sess || sess.state !== 'waiting')",
    "      return text('IDLE');",
    "",
    "    var uid = String(p.uid).toUpperCase();",
    "    var out = (sess.mode === 'register')",
    "      ? doRegister(sess, uid)",
    "      : doAttend(sess, uid);",
    "",
    "    // 결과를 남기고 세션 완료 처리",
    "    sess.state = 'done';",
    "    CacheService.getScriptCache().put(",
    "      cacheKey(p.device),",
    "      JSON.stringify(sess), 60);",
    "    return text(out);",
    "  } finally { lock.releaseLock(); }",
    "}"
  ], 10.5);
  footerUrl(s, "코드", GAS_URL);
  s.addNotes("Lock은 '화장실 문 잠금'에 비유하세요 — 두 명이 동시에 들어오면 안 되니 한 명씩. try/finally는 '무슨 일이 있어도 나올 때 문을 연다'입니다. ESP32 없이도 브라우저에서 tag를 직접 호출해 이 함수를 시험할 수 있다는 점이 수업 운영에 매우 유용합니다.");
}

// ═════ 18. doRegister ═════
{
  const s = newSlide();
  header(s, "3차시 · relay.gs 코드 ⑤", "doRegister() — 학생증 등록");
  bullets(s, [
    { t: "앱에서 입력한 학생 정보(세션에 저장됨) + 태그된 UID를 학생명단 시트에 한 줄로 저장", b: true },
    { t: "이미 등록된 카드면 거절(DUP) — 한 카드가 두 학생이 될 수 없음" },
    { t: "이 한 줄이 등록의 전부 — 이후 출석 때 findStudent(uid)로 이 줄을 찾아 \"누구인지\" 알아냄" },
    { t: "폰 NFC는 태그할 때마다 UID가 바뀌므로 등록 불가 — 실물 카드만" }
  ], 0.6, 1.75, 5.5, 4.6, 12.5);
  codeBox(s, 6.35, 1.6, 6.4, 5.2, [
    "function doRegister(sess, uid) {",
    "  var sheet = getSheet('학생명단',",
    "    ['UID','이름','학년','반','번호','등록일시']);",
    "",
    "  // 이미 등록된 카드인지 확인",
    "  var found = findStudent(uid);",
    "  if (found) {",
    "    sess.result = { ok: false,",
    "      reason: 'dup', name: found.name };",
    "    return 'DUP,' + found.name;",
    "  }",
    "",
    "  // 학생명단에 한 줄 추가 = 등록 완료",
    "  sheet.appendRow([uid, sess.name,",
    "    sess.grade, sess.klass, sess.number,",
    "    now('yyyy-MM-dd HH:mm:ss')]);",
    "",
    "  sess.result = { ok: true,",
    "    name: sess.name, uid: uid };",
    "  return 'OK_REG,' + sess.name;",
    "}"
  ], 10.5);
  footerUrl(s, "코드", GAS_URL);
  s.addNotes("'등록'이 거창한 게 아니라 시트에 한 줄 쓰는 것임을 보여주는 슬라이드입니다. 등록 후 시트를 열어 방금 생긴 줄을 직접 확인시키세요. 개인정보 교육 포인트: UID는 카드 번호일 뿐이지만 이름과 묶이는 순간 개인정보가 되므로 시트 공유 설정에 주의해야 한다는 점을 언급하면 좋습니다.");
}

// ═════ 19. doAttend ═════
{
  const s = newSlide();
  header(s, "3차시 · relay.gs 코드 ⑥", "doAttend() — 출석 기록과 지각 판정");
  bullets(s, [
    { t: "검사 3단계: ① 미등록 카드(UNKNOWN) → ② 대상과 다른 학생(MISMATCH) → ③ 같은 교시 중복(ALREADY)", b: true },
    { t: "지각 판정: 태그 시각이 lateAfter(교시설정 시트 기준)를 넘으면 상태 = 지각" },
    { t: "검사를 모두 통과하면 출석기록 시트에 한 줄 추가 — 이게 곧 출석부" },
    { t: "결과는 앱(이름·상태·시각)과 ESP32(부저) 양쪽에 전달" }
  ], 0.6, 1.75, 5.5, 4.2, 12);
  codeBox(s, 6.35, 1.6, 6.4, 5.2, [
    "function doAttend(sess, uid) {",
    "  var student = findStudent(uid);",
    "  if (!student) return 'UNKNOWN';  // ①미등록",
    "",
    "  if (sess.name && sess.name !== student.name)",
    "    return 'MISMATCH,' + student.name; // ②",
    "",
    "  // ③ 같은 날 같은 교시 중복 확인",
    "  // (출석기록에서 날짜·교시·UID가 같은 줄)",
    "  if (이미기록있음)",
    "    return 'ALREADY,' + student.name;",
    "",
    "  var time = now('HH:mm:ss');",
    "  var status = '출석';",
    "  if (sess.lateAfter &&",
    "      time > sess.lateAfter + ':00')",
    "    status = '지각';  // 지각 판정",
    "",
    "  sheet.appendRow([today, time, sess.period,",
    "    uid, student.name, status, sess.device]);",
    "  return 'OK,'+student.name+','+status+','+time;",
    "}"
  ], 10);
  s.addText("※ 슬라이드 코드는 이해를 위한 요약본입니다. 전체 코드는 아래 주소에서 확인하세요.",
    { x: 0.6, y: 6.35, w: 5.6, h: 0.5, fontFace: KO, fontSize: 10, italic: true,
      color: C.sub, margin: 0 });
  footerUrl(s, "코드", GAS_URL);
  s.addNotes("검사 순서(미등록→불일치→중복)를 먼저, 기록은 마지막에 — '문지기를 다 통과해야 도장을 찍는다'는 구조입니다. 시각 비교가 문자열 비교('09:12:33' > '09:10:00')로 동작하는 이유(자릿수가 같은 시간 문자열은 사전순=시간순)는 상급 학생에게 좋은 심화 질문입니다.");
}

// ═════ 20. GAS 도우미 함수 ═════
{
  const s = newSlide();
  header(s, "3차시 · relay.gs 코드 ⑦", "도우미 함수들 — 반복 작업을 맡는 조연");
  bullets(s, [
    { t: "findStudent(uid): 학생명단을 위에서부터 훑어 UID가 같은 줄을 찾음 — 없으면 null", b: true },
    { t: "getSheet(이름, 머리글): 시트가 없으면 머리글과 함께 만들어서 돌려줌 — \"자동 생성\"의 정체" },
    { t: "now(형식): 한국 시간대(Asia/Seoul) 기준 현재 시각 문자열" },
    { t: "text()/json(): 응답 형식 포장 — ESP32용 텍스트와 앱용 JSON" }
  ], 0.6, 1.75, 5.5, 4.6, 12.5);
  codeBox(s, 6.35, 1.6, 6.4, 5.2, [
    "function findStudent(uid) {",
    "  var rows = getSheet('학생명단', 머리글)",
    "    .getDataRange().getValues();",
    "  // 1행(머리글) 건너뛰고 한 줄씩 비교",
    "  for (var i = 1; i < rows.length; i++) {",
    "    if (String(rows[i][0]).toUpperCase()",
    "        === uid) {",
    "      return { name: String(rows[i][1]),",
    "        grade: rows[i][2], klass: rows[i][3] };",
    "    }",
    "  }",
    "  return null;  // 못 찾음 = 미등록",
    "}",
    "",
    "function getSheet(name, headers) {",
    "  var ss = SpreadsheetApp",
    "    .getActiveSpreadsheet();",
    "  var sheet = ss.getSheetByName(name);",
    "  if (!sheet) {          // 없으면 생성",
    "    sheet = ss.insertSheet(name);",
    "    sheet.appendRow(headers);",
    "  }",
    "  return sheet;",
    "}"
  ], 10);
  footerUrl(s, "코드", GAS_URL);
  s.addNotes("findStudent는 반복문으로 표를 검색하는 가장 기본적인 패턴이라 코딩 초심자 지도에 좋은 예제입니다. '학생이 500명이면 느리지 않을까?'라는 질문이 나오면 훌륭한 심화 토론(검색 효율, 인덱스 개념)으로 이어갈 수 있습니다.");
}

// ═════ 21. ESP32 설정+setup ═════
{
  const s = newSlide();
  header(s, "4차시 · 완성 펌웨어 ①", "설정 4줄과 setup() — 시작 준비");
  bullets(s, [
    { t: "바꿀 곳은 상단 4줄뿐: 와이파이 이름/비번(2.4GHz만!), 중계서버 URL, 리더기 이름", b: true },
    { t: "RC522 강화 설정 2가지 — 수신 감도 최대(RxGain_max) + 송신 출력 부스트(GsN/CWGsP). 실제 테스트에서 학생증 인식을 가능하게 한 핵심" },
    { t: "와이파이 접속이 끝나면 부저 삑삑 — 준비 완료 신호" },
    { t: "uid_test와 같은 초기화 — 2차시에 검증한 설정을 그대로 가져옴" }
  ], 0.6, 1.75, 5.5, 4.2, 12);
  codeBox(s, 6.35, 1.6, 6.4, 5.2, [
    "// ── [설정] 여기 4줄만 바꾸면 됩니다 ──",
    "const char* WIFI_SSID = \"와이파이이름\";",
    "const char* WIFI_PASS = \"비밀번호\";",
    "const char* RELAY_URL = \"https://script.",
    "  google.com/macros/s/배포ID/exec\";",
    "const char* DEVICE_ID = \"DEV001\";",
    "",
    "void setup() {",
    "  Serial.begin(115200);",
    "  SPI.begin();",
    "  rfid.PCD_Init();",
    "  rfid.PCD_SetAntennaGain(MFRC522::RxGain_max);",
    "  rfid.PCD_WriteRegister(MFRC522::GsNReg, 0xF8);",
    "  rfid.PCD_WriteRegister(MFRC522::CWGsPReg,0x3F);",
    "",
    "  WiFi.begin(WIFI_SSID, WIFI_PASS);",
    "  while (WiFi.status() != WL_CONNECTED)",
    "    delay(500);",
    "  Serial.println(WiFi.localIP());",
    "  beep(80); beep(80);  // 준비 완료!",
    "}"
  ], 10);
  footerUrl(s, "코드", INO_URL);
  s.addNotes("업로드 전 반드시 '전체 선택 → 삭제 → 붙여넣기'를 시키세요. 새 스케치의 빈 setup/loop 위에 덧붙이면 'redefinition of void setup()' 컴파일 에러가 납니다(실제로 매우 흔한 실수). RELAY_URL은 /exec으로 끝나야 한다는 점도 체크리스트에 넣으세요.");
}

// ═════ 22. loop ═════
{
  const s = newSlide();
  header(s, "4차시 · 완성 펌웨어 ②", "loop() — 카드 스캔이 최우선인 이유");
  bullets(s, [
    { t: "우선순위: ① 카드 확인 → ② LED 깜빡 → ③ 서버 폴링", b: true },
    { t: "서버 요청(HTTPS)은 한 번에 2~3초 걸리고 그동안 모든 게 멈춤 — 그래서 폴링을 마지막에, 세션 중에는 5초 간격으로 뜸하게" },
    { t: "폴링 직전에도 카드를 한 번 더 확인 — 카드가 올라온 채로 멈추는 일 방지" },
    { t: "LED: 출석 0.5초 / 등록 0.15초 간격 — \"지금 태그하면 된다\"는 신호" },
    { t: "delay 대신 millis() 비교 — 기다리는 동안에도 다른 일을 할 수 있는 구조" }
  ], 0.6, 1.75, 5.5, 4.7, 11.5);
  codeBox(s, 6.35, 1.6, 6.4, 5.2, [
    "void loop() {",
    "  // ① 카드 확인이 최우선",
    "  if (handleCard()) return;",
    "",
    "  // ② 세션 중 LED 깜빡 (등록은 더 빠르게)",
    "  if (mode != \"IDLE\") {",
    "    unsigned long iv =",
    "      (mode == \"REGISTER\") ? 150 : 500;",
    "    if (millis() - lastBlink >= iv) {",
    "      lastBlink = millis();",
    "      ledOn = !ledOn;",
    "      digitalWrite(PIN_LED, ledOn);",
    "    }",
    "  }",
    "",
    "  // ③ 서버 폴링 (대기 2초 / 세션 중 5초)",
    "  unsigned long pi =",
    "    (mode == \"IDLE\") ? 2000 : 5000;",
    "  if (millis() - lastPoll >= pi) {",
    "    if (handleCard()) return; // 한 번 더!",
    "    lastPoll = millis();",
    "    String res = httpGet(폴링주소);",
    "    if (res==\"ATTEND\" || res==\"REGISTER\"",
    "        || res==\"IDLE\") mode = res;",
    "  }",
    "}"
  ], 9.5);
  footerUrl(s, "코드", INO_URL);
  s.addNotes("이 구조는 실제 디버깅 경험에서 나왔습니다: 처음엔 세션 중에도 2초마다 폴링했더니 HTTPS 요청(2~3초)에 loop가 묶여 LED가 멈추고 태그를 놓쳤습니다. '느린 작업이 빠른 작업을 막으면 안 된다'는 임베디드의 핵심 교훈을 실화로 들려주세요. millis() 패턴 vs delay()의 차이도 이 슬라이드에서 짚기 좋습니다.");
}

// ═════ 23. cardTagged/readUid ═════
{
  const s = newSlide();
  header(s, "4차시 · 완성 펌웨어 ③", "cardTagged() · readUid() — 학생증을 읽는 법");
  bullets(s, [
    { t: "일반 감지(REQA) 대신 WUPA(강제 깨우기)를 사용", b: true },
    { t: "학생증 같은 보안 스마트카드(ISO 14443-4)는 REQA에 응답하지 않거나 HALT 상태에 머무는 경우가 있음 — WUPA가 훨씬 안정적" },
    { t: "readUid()는 UID를 \"6F3178D9\" 같은 대문자 16진수 문자열로 — 시트에 저장되는 값과 동일한 형식" },
    { t: "UID만 읽으므로 카드 내부 데이터(잔액 등)는 건드리지 않음" }
  ], 0.6, 1.75, 5.5, 4.5, 12);
  codeBox(s, 6.35, 1.6, 6.4, 5.2, [
    "bool cardTagged() {",
    "  // WUPA: HALT 상태 카드까지 깨워서 감지",
    "  byte atqa[2];",
    "  byte size = sizeof(atqa);",
    "  MFRC522::StatusCode st =",
    "    rfid.PICC_WakeupA(atqa, &size);",
    "  if (st != MFRC522::STATUS_OK &&",
    "      st != MFRC522::STATUS_COLLISION)",
    "    return false;",
    "  return rfid.PICC_ReadCardSerial();",
    "}",
    "",
    "String readUid() {",
    "  // 예: 6F3178D9 (대문자 16진수)",
    "  String uid = \"\";",
    "  for (byte i = 0; i < rfid.uid.size; i++) {",
    "    if (rfid.uid.uidByte[i] < 0x10)",
    "      uid += \"0\";",
    "    uid += String(rfid.uid.uidByte[i], HEX);",
    "  }",
    "  uid.toUpperCase();",
    "  return uid;",
    "}"
  ], 10);
  footerUrl(s, "코드", INO_URL);
  s.addNotes("교통카드·은행카드는 읽히는데 학생증만 안 읽히던 실제 문제를 WUPA + 송신 부스트로 해결했습니다. '같은 규격이라도 카드마다 특성이 다르고, 데이터시트와 실험으로 해결한다'는 엔지니어링 과정 자체가 교육 소재입니다. 0x10 미만일 때 '0'을 붙이는 이유(자릿수 맞춤: F → 0F)는 짧은 퀴즈로 좋습니다.");
}

// ═════ 24. handleCard ═════
{
  const s = newSlide();
  header(s, "4차시 · 완성 펌웨어 ④", "handleCard() — 태그 처리의 중심");
  bullets(s, [
    { t: "카드가 없으면 false — loop는 다음 일(LED·폴링)로 넘어감", b: true },
    { t: "세션이 없을 때(IDLE): UID를 시리얼에만 출력 — 서버 전송 없이 현장 인식 테스트 가능" },
    { t: "세션 중: UID를 서버로 전송(action=tag)하고 응답에 따라 부저 — 성공 삑삑 / 실패 삐—" },
    { t: "처리 후 1초 대기 — 같은 카드 연속 인식 방지" }
  ], 0.6, 1.75, 5.5, 4.5, 12);
  codeBox(s, 6.35, 1.6, 6.4, 5.2, [
    "bool handleCard() {",
    "  if (!cardTagged()) return false;",
    "  String uid = readUid();",
    "  rfid.PICC_HaltA();       // 카드 통신 종료",
    "  rfid.PCD_StopCrypto1();",
    "",
    "  if (mode == \"IDLE\") {",
    "    // 세션 없음 → 확인용 출력만",
    "    Serial.println(\"카드 인식: \" + uid);",
    "    delay(1000);",
    "    return true;",
    "  }",
    "",
    "  // 세션 중 → 서버로 전송",
    "  digitalWrite(PIN_LED, HIGH);",
    "  String res = httpGet(RELAY_URL +",
    "    \"?action=tag&device=\" + DEVICE_ID +",
    "    \"&uid=\" + uid);",
    "  handleTagResult(res);  // 부저·메시지",
    "",
    "  mode = \"IDLE\";",
    "  digitalWrite(PIN_LED, LOW);",
    "  delay(1000);  // 연속 인식 방지",
    "  return true;",
    "}"
  ], 9.5);
  footerUrl(s, "코드", INO_URL);
  s.addNotes("'세션이 없을 때는 출력만, 있을 때는 전송'이라는 분기가 핵심입니다. 시리얼 모니터를 열어두고 세션 없이 태그(출력만) → 앱에서 시작 후 태그(전송)를 연달아 보여주면 mode 변수의 역할이 명확해집니다.");
}

// ═════ 25. httpGet ═════
{
  const s = newSlide();
  header(s, "4차시 · 완성 펌웨어 ⑤", "httpGet() · handleTagResult() — 서버와 대화하기");
  bullets(s, [
    { t: "핵심 1 — 리다이렉트 따라가기: Apps Script는 응답 시 주소를 한 번 이동시킴. setFollowRedirects가 없으면 항상 빈 응답 (연동 실패 1순위 원인)", b: true },
    { t: "핵심 2 — setInsecure(): 인증서 검증 생략(수업용 간소화). 통신 자체는 HTTPS 암호화" },
    { t: "10초 타임아웃 — 서버가 늦어도 ESP32가 영원히 멈추지 않게" },
    { t: "handleTagResult(): 응답 첫 단어(OK/OK_REG/UNKNOWN/DUP/MISMATCH/ALREADY)로 부저와 시리얼 메시지 결정" }
  ], 0.6, 1.75, 5.5, 4.7, 11.5);
  codeBox(s, 6.35, 1.6, 6.4, 5.2, [
    "String httpGet(String url) {",
    "  WiFiClientSecure client;",
    "  client.setInsecure(); // 인증서 검증 생략",
    "",
    "  HTTPClient https;",
    "  // Apps Script 리다이렉트 따라가기 (필수!)",
    "  https.setFollowRedirects(",
    "    HTTPC_STRICT_FOLLOW_REDIRECTS);",
    "  https.setTimeout(10000);",
    "",
    "  String body = \"\";",
    "  if (https.begin(client, url)) {",
    "    if (https.GET() == HTTP_CODE_OK) {",
    "      body = https.getString();",
    "      body.trim();",
    "    }",
    "    https.end();",
    "  }",
    "  return body;",
    "  // 예: \"OK,김세이프,지각,21:22:58\"",
    "}"
  ], 10);
  footerUrl(s, "코드", INO_URL);
  s.addNotes("'ESP32가 크롬 브라우저처럼 주소를 열고 응답 글자를 받아온다'고 설명하면 됩니다. 브라우저에서 같은 주소를 열어 같은 글자가 보이는 것을 나란히 시연하면 (브라우저=ESP32) 등식이 만들어져 디버깅 능력이 생깁니다.");
}

// ═════ 26. 앱 제작 ═════
{
  const s = newSlide();
  header(s, "4차시 · 앱", "앱 만들기 — 프롬프트 한 번으로");
  const steps = [
    ["프롬프트 열기", "저장소의 docs/app_build_prompt.md 파일을 엽니다"],
    ["URL 넣기", "{{중계서버URL}} 자리를 3차시에 배포한 실제 URL로 바꿉니다"],
    ["AI Studio에 붙여넣기", "프롬프트 전체를 붙여넣으면 앱이 생성됩니다"],
    ["배포", "AI Studio에서 앱을 배포(Cloud Run)하고 주소를 받습니다"]
  ];
  steps.forEach((st, i) => {
    const x = 0.6 + i * 3.13;
    card(s, x, 1.65, 2.88, 2.2, C.white, C.border);
    numCircle(s, i + 1, x + 0.25, 1.9);
    s.addText(st[0], { x: x + 0.75, y: 1.9, w: 2.0, h: 0.65, fontFace: KO,
      fontSize: 12.5, bold: true, color: C.text, valign: "middle", margin: 0, lineSpacingMultiple: 1.05 });
    s.addText(st[1], { x: x + 0.26, y: 2.62, w: 2.38, h: 1.1, fontFace: KO,
      fontSize: 10.5, color: C.sub, margin: 0, lineSpacingMultiple: 1.15 });
  });
  card(s, 0.6, 4.2, 12.1, 2.5, C.cardBg, C.border);
  s.addText("프롬프트가 만들어 주는 화면 (3개 탭)", { x: 0.88, y: 4.38, w: 11.5, h: 0.4,
    fontFace: KO, fontSize: 13.5, bold: true, color: C.text, margin: 0 });
  const tabs = [
    ["출석체크", "교시·대상 학생 선택 → 시작 → 30초 대기(카운트다운) → 결과 표시(출석/지각 배지). 시뮬레이션 버튼 포함"],
    ["학생 관리", "학생 정보 입력 → [등록 시작] → 태그 → 학생증 등록. 등록된 학생 목록 조회"],
    ["출결 기록", "날짜·교시 필터로 출석기록 조회 + 출석/지각 요약 통계"]
  ];
  tabs.forEach((t, i) => {
    const x = 0.88 + i * 3.98;
    s.addText(t[0], { x, y: 4.9, w: 3.7, h: 0.35, fontFace: KO, fontSize: 12.5,
      bold: true, color: C.greenDark, margin: 0 });
    s.addText(t[1], { x, y: 5.28, w: 3.7, h: 1.3, fontFace: KO, fontSize: 10.5,
      color: C.sub, margin: 0, lineSpacingMultiple: 1.2 });
  });
  footerUrl(s, "앱 프롬프트 전문", REPO + "/blob/main/wemos-d1-r32-projects/rfid-attendance-system/docs/app_build_prompt.md");
  s.addNotes("프롬프트에는 화면 구성뿐 아니라 API 명세와 CORS 주의사항까지 들어 있어, AI가 통신 코드를 정확히 생성합니다. 수업에서는 'AI에게 정확한 명세를 주면 정확한 결과가 나온다'는 프롬프트 엔지니어링 교육 소재로도 활용할 수 있습니다. 이미 앱이 있는 경우엔 docs/app_integration.md의 수정 프롬프트를 쓰면 됩니다.");
}

// ═════ 27. 앱 연동 코드 ═════
{
  const s = newSlide();
  header(s, "4차시 · 앱", "앱은 이 두 가지만 한다 — 시작하고, 묻는다");
  bullets(s, [
    { t: "요청은 반드시 GET + URL 파라미터 — 커스텀 헤더(Content-Type 등)를 붙이면 CORS 오류로 실패", b: true },
    { t: "① 시작: start 호출로 세션 생성 → ESP32가 폴링으로 알아챔" },
    { t: "② 확인: 1초마다 status 호출 → done이면 결과 표시, none이면 시간 초과" },
    { t: "시뮬레이션 버튼도 같은 tag 주소를 호출 — ESP32 없이 전체 흐름 테스트 가능" },
    { t: "결과 화면은 result의 ok / name / status / time / reason만 표시하면 끝" }
  ], 0.6, 1.75, 5.5, 4.6, 12);
  codeBox(s, 6.35, 1.6, 6.4, 5.2, [
    "// 요청 도우미: GET + URL 파라미터만",
    "async function relay(params) {",
    "  const qs = new URLSearchParams(params);",
    "  const res = await fetch(`${RELAY_URL}?${qs}`);",
    "  return JSON.parse(await res.text());",
    "}",
    "",
    "// ① [출석체크 시작] 버튼",
    "relay({ action: 'start', mode: 'attend',",
    "  device: 'DEV001', period: '1',",
    "  name: '김세이프', timeout: '30' });",
    "",
    "// ② 대기 화면에서 1초마다 결과 확인",
    "const s = await relay({ action: 'status',",
    "  device: 'DEV001' });",
    "if (s.state === 'done')",
    "  showResult(s.result);",
    "  // { ok: true, name: '김세이프',",
    "  //   status: '지각', time: '21:22:58' }",
    "else if (s.state === 'none')",
    "  showTimeout();  // 30초 초과"
  ], 10);
  footerUrl(s, "연동 가이드", REPO + "/blob/main/wemos-d1-r32-projects/rfid-attendance-system/docs/app_integration.md");
  s.addNotes("ESP32의 폴링 코드(2초마다 poll)와 이 코드(1초마다 status)를 나란히 놓고 '똑같은 패턴'임을 보여주세요 — 언어(C++/JS)가 달라도 구조는 같다는 것이 이 수업의 종합 정리입니다. CORS는 '브라우저의 보안 검문'이며, 단순한 GET 요청은 검문 없이 통과한다는 정도로 설명하면 충분합니다.");
}

// ═════ 28. 통신 규칙 ═════
{
  const s = newSlide();
  header(s, "정리", "통신 규칙 한눈에 보기");
  const th = { fontFace: KO, fontSize: 11, bold: true, color: C.greenDark,
    fill: { color: C.greenLight }, valign: "middle" };
  const td = { fontFace: KO, fontSize: 10.5, color: C.text, valign: "middle" };
  const tdm = { fontFace: MONO, fontSize: 10, color: C.text, valign: "middle" };
  s.addText("ESP32 ↔ 서버 (한 줄 텍스트)", { x: 0.6, y: 1.55, w: 6.0, h: 0.35,
    fontFace: KO, fontSize: 14, bold: true, color: C.text, margin: 0 });
  s.addTable([
    [{ text: "응답", options: th }, { text: "뜻 / ESP32의 반응", options: th }],
    [{ text: "IDLE / ATTEND / REGISTER", options: tdm }, { text: "poll 응답 — 대기 / 출석 세션 / 등록 세션 (LED 깜빡)", options: td }],
    [{ text: "OK,이름,상태,시각", options: tdm }, { text: "출석 기록 완료 → 삑삑", options: td }],
    [{ text: "OK_REG,이름", options: tdm }, { text: "학생증 등록 완료 → 삑삑", options: td }],
    [{ text: "UNKNOWN / DUP / MISMATCH / ALREADY", options: tdm }, { text: "미등록·중복 카드·다른 학생·중복 출석 → 삐—", options: td }]
  ], { x: 0.6, y: 1.95, w: 6.05, colW: [2.45, 3.6], border: { color: C.border, pt: 0.75 },
    rowH: 0.42, margin: 0.06 });
  s.addText("앱 ↔ 서버 (JSON)", { x: 7.05, y: 1.55, w: 5.7, h: 0.35,
    fontFace: KO, fontSize: 14, bold: true, color: C.text, margin: 0 });
  s.addTable([
    [{ text: "action", options: th }, { text: "용도", options: th }],
    [{ text: "start", options: tdm }, { text: "세션 시작 (mode=attend|register)", options: td }],
    [{ text: "status", options: tdm }, { text: "결과 폴링 → state: none|waiting|done", options: td }],
    [{ text: "cancel", options: tdm }, { text: "세션 취소 (모달 닫기)", options: td }],
    [{ text: "students / records / periods", options: tdm }, { text: "학생 목록 / 출석기록 / 교시설정 조회", options: td }]
  ], { x: 7.05, y: 1.95, w: 5.7, colW: [2.3, 3.4], border: { color: C.border, pt: 0.75 },
    rowH: 0.42, margin: 0.06 });
  card(s, 0.6, 4.75, 12.15, 1.9, C.cardBg, C.border);
  s.addText("설계 포인트 (수업 정리용)", { x: 0.85, y: 4.9, w: 11.6, h: 0.35, fontFace: KO,
    fontSize: 13, bold: true, color: C.text, margin: 0 });
  s.addText("· ESP32에는 텍스트, 앱에는 JSON — 기기 성능에 맞는 응답 형식 분리\n· 상태는 전부 서버(캐시+시트)에 — 앱과 ESP32는 언제 꺼져도 다시 물어보면 됨\n· 세션 자동 만료(30초) — 타이머 코드 없이 캐시 TTL로 해결",
    { x: 0.85, y: 5.28, w: 11.6, h: 1.25, fontFace: KO, fontSize: 11.5, color: C.sub,
      margin: 0, lineSpacingMultiple: 1.25 });
  footerUrl(s, "통신 규칙 문서", REPO + "/blob/main/wemos-d1-r32-projects/rfid-attendance-system/apps_script/README.md");
  s.addNotes("이 표를 A4로 출력해 실습실에 붙여두면 디버깅할 때 학생들이 스스로 찾아봅니다. '프로토콜(약속)을 먼저 정하면 앱 팀과 장치 팀이 따로 개발해도 나중에 맞물린다'는 협업 포인트도 정리 발문으로 좋습니다.");
}

// ═════ 29. 테스트 & 트러블슈팅 ═════
{
  const s = newSlide();
  header(s, "운영", "테스트 순서와 자주 겪는 문제");
  card(s, 0.6, 1.6, 5.9, 4.55, C.white, C.border);
  s.addText("연동 테스트 순서", { x: 0.88, y: 1.8, w: 5.3, h: 0.4, fontFace: KO,
    fontSize: 14, bold: true, color: C.greenDark, margin: 0 });
  const tests = [
    "브라우저에서 URL?action=periods → JSON 확인 (시트 자동 생성)",
    "uid_test 업로드 → 시리얼에서 학생증 UID 확인",
    "앱 [학생증 등록] → LED 빠른 깜빡 → 태그 → 학생명단 시트 확인",
    "앱 [출석체크 시작] → LED 느린 깜빡 → 태그 → 결과·출석기록 확인",
    "ESP32 없이도: 브라우저에서 ?action=tag&device=DEV001&uid=... 로 태그 흉내"
  ];
  tests.forEach((t, i) => {
    numCircle(s, i + 1, 0.9, 2.28 + i * 0.76);
    s.addText(t, { x: 1.45, y: 2.24 + i * 0.76, w: 4.85, h: 0.72, fontFace: KO,
      fontSize: 10.5, color: C.text, valign: "middle", margin: 0, lineSpacingMultiple: 1.1 });
  });
  card(s, 6.8, 1.6, 5.95, 4.55, C.white, C.border);
  s.addText("자주 겪는 문제", { x: 7.08, y: 1.8, w: 5.4, h: 0.4, fontFace: KO,
    fontSize: 14, bold: true, color: C.amber, margin: 0 });
  const trouble = [
    ["학생증만 인식이 안 돼요", "송신 부스트+WUPA가 적용된 최신 펌웨어인지 확인. 카드 가장자리를 안테나 중앙에 1~2초 밀착"],
    ["앱 결과가 안 와요", "GAS 배포 액세스가 \"모든 사용자\"인지, 코드 수정 후 새 버전으로 재배포했는지 확인"],
    ["와이파이가 안 붙어요", "ESP32는 2.4GHz만 지원 — 5GHz 전용 SSID는 접속 불가. 폰 핫스팟(2.4GHz)이 좋은 대안"],
    ["세션 중 인식이 둔해요", "정상 — 5초마다 서버 폴링 중엔 잠시 멈춤. 카드를 떼지 말고 붙이고 있으면 잡힘"]
  ];
  trouble.forEach((t, i) => {
    s.addText(t[0], { x: 7.08, y: 2.3 + i * 0.95, w: 5.4, h: 0.3, fontFace: KO,
      fontSize: 11.5, bold: true, color: C.text, margin: 0 });
    s.addText(t[1], { x: 7.08, y: 2.6 + i * 0.95, w: 5.4, h: 0.6, fontFace: KO,
      fontSize: 10, color: C.sub, margin: 0, lineSpacingMultiple: 1.1 });
  });
  card(s, 0.6, 6.35, 12.15, 0.6, C.greenLight, "D1FAE5");
  s.addText([
    { text: "전체 코드 · 문서 · 배선도  ", options: { bold: true, color: C.greenDark } },
    { text: BASE, options: { color: C.green, bold: true } }
  ], { x: 0.85, y: 6.35, w: 11.6, h: 0.6, fontFace: KO, fontSize: 13,
    margin: 0, valign: "middle" });
  s.addNotes("테스트 순서 ①~⑤는 '한 층씩 검증'하는 구조입니다(서버→리더기→등록→출석→시뮬레이션). 문제가 생기면 어느 층에서 막혔는지부터 찾게 하세요 — 이 분리 진단 습관이 이 수업이 기르는 가장 중요한 역량입니다.");
}

// ═════ 30. 수업 활용 & 확장 ═════
{
  const s = newSlide();
  header(s, "마무리", "수업 활용과 확장 아이디어");
  const ideas = [
    ["리더기 여러 대", "DEVICE_ID만 DEV002, DEV003…으로 바꾸면 교실마다 리더기 설치 가능 — 세션이 리더기별로 분리되어 있음"],
    ["하교 체크 추가", "세션 mode에 'leave'를 추가하고 출석기록에 구분 열을 넣으면 등·하교 시스템으로 확장"],
    ["통계 대시보드", "records API로 데이터를 받아 주간 출석률 그래프 화면을 앱에 추가 — 데이터 분석 수업과 연계"],
    ["알림 연동", "GAS에서 지각 기록 시 보호자에게 메일 발송(MailApp) — 서비스 기획 토론과 연계"]
  ];
  ideas.forEach((t, i) => {
    const col = i % 2, row = Math.floor(i / 2);
    const x = 0.6 + col * 6.2, y = 1.65 + row * 1.75;
    card(s, x, y, 5.95, 1.55, C.white, C.border);
    s.addText(t[0], { x: x + 0.28, y: y + 0.18, w: 5.4, h: 0.35, fontFace: KO,
      fontSize: 13, bold: true, color: C.greenDark, margin: 0 });
    s.addText(t[1], { x: x + 0.28, y: y + 0.58, w: 5.4, h: 0.85, fontFace: KO,
      fontSize: 11, color: C.sub, margin: 0, lineSpacingMultiple: 1.2 });
  });
  card(s, 0.6, 5.25, 12.15, 1.4, C.cardBg, C.border);
  s.addText("운영 시 유의사항", { x: 0.88, y: 5.4, w: 11.5, h: 0.35, fontFace: KO,
    fontSize: 13, bold: true, color: C.text, margin: 0 });
  s.addText("· 학생명단 시트에는 이름 등 개인정보가 있으므로 시트 공유 범위를 최소화하세요\n· 실제 출결의 법적 근거로 쓰기보다 교육·보조 도구로 활용하는 것을 권장합니다",
    { x: 0.88, y: 5.77, w: 11.5, h: 0.8, fontFace: KO, fontSize: 11.5, color: C.sub,
      margin: 0, lineSpacingMultiple: 1.25 });
  footerUrl(s, "전체 자료", BASE);
  s.addNotes("확장 4가지는 모두 지금 구조 위에 얹을 수 있는 것들로, 팀 프로젝트 주제로 나눠주기 좋습니다. 마지막으로 첫 구조도 슬라이드로 돌아가 '우리가 이 화살표를 전부 직접 만들었다'로 마무리하면 성취감이 큽니다.");
}

pres.writeFile({ fileName: "./smart_attendance_teaching_guide.pptx" })
  .then(() => console.log("done"));
