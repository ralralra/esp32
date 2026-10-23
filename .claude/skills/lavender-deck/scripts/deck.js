// lavender-deck — 연보라 배경 · 진남색 제목 · 보라 포인트 디자인의 pptx를 만드는 공용 모듈
//
// 사용:
//   const { createDeck } = require("<skill>/scripts/deck.js");
//   const d = await createDeck({ title: "덱 제목", footer: "바닥글" });
//   const s = d.slide({ section: "소개", label: "시작 · 오늘의 지도", tag: "따라 하기", title: "제목", sub: "부제" });
//   d.step(s, 1, "첫 번째 할 일", 0.6, 2.3, 6);
//   await d.finish("out.pptx");
//
// 의존: pptxgenjs · react-icons · react · react-dom · sharp (· jszip, 테마 적용용)
// require 실패 시 빌드 폴더에서: npm install pptxgenjs react-icons react react-dom sharp jszip

const path = require("path");
const fs = require("fs");

function req(name) {
  // 빌드 스크립트 폴더 · 현재 폴더 · 이 파일 폴더 순으로 찾는다
  const paths = [process.cwd(), path.dirname(require.main ? require.main.filename : process.cwd()), __dirname];
  try { return require(require.resolve(name, { paths })); }
  catch (e) { throw new Error(`'${name}' 모듈을 찾지 못했습니다. 빌드 폴더에서 실행: npm install pptxgenjs react-icons react react-dom sharp jszip\n(${e.message})`); }
}

// ── 디자인 토큰 (references/design.md 와 같은 값) ──
const THEME = {
  name: "Lavender Deck",
  headFontFace: "Noto Sans KR",
  bodyFontFace: "Noto Sans KR",
  colors: {
    dk1: "1F1442", lt1: "FFFFFF", dk2: "6E6591", lt2: "EFEAFB",
    accent1: "5038A6", accent2: "F2A03D", accent3: "2A9D8F", accent4: "E0567A", accent5: "3D7FD9", accent6: "DDD8F7",
    hlink: "5038A6", folHlink: "7A52C7",
  },
};
const H = THEME.colors;
const BG = "F7F4FB";          // 슬라이드 배경
const LINE = "DDD8F7";        // 카드 테두리 · 콜아웃 채움
const MONO = "JetBrains Mono";
const DANGER_BG = "FDECEA", DANGER_LINE = "F5C6C0", DANGER_TX = "9A1C12";
const W = 13.33, HGT = 7.5;   // LAYOUT_WIDE

async function createDeck(opts = {}) {
  const pptxgen = req("pptxgenjs");
  const React = req("react");
  const ReactDOMServer = req("react-dom/server");
  const sharp = req("sharp");
  const fa = req("react-icons/fa");

  const pres = new pptxgen();
  pres.layout = "LAYOUT_WIDE";
  pres.theme = { headFontFace: THEME.headFontFace, bodyFontFace: THEME.bodyFontFace };
  pres.title = opts.title || "Presentation";
  pres.author = opts.author || "";
  const footer = opts.footer || opts.title || "";
  const C = pres.SchemeColor;

  // ── 레이아웃 두 개: CONTENT(라벨·제목·부제·바닥글·쪽번호) · TITLE(표지) ──
  pres.defineSlideMaster({
    title: "CONTENT",
    background: { color: BG },
    objects: [
      { placeholder: { options: { name: "label", type: "body", x: 0.6, y: 0.32, w: 8, h: 0.3, fontSize: 11, bold: true, color: C.accent1, valign: "middle", margin: 0 }, text: "" } },
      { placeholder: { options: { name: "title", type: "title", x: 0.6, y: 0.95, w: 9.5, h: 0.75, fontSize: 30, bold: true, color: C.text1, valign: "middle", align: "left", margin: 0 }, text: "" } },
      { placeholder: { options: { name: "sub", type: "body", x: 0.6, y: 1.68, w: 9.5, h: 0.35, fontSize: 13, color: C.text2, valign: "middle", margin: 0 }, text: "" } },
      { text: { text: footer, options: { x: 0.6, y: 7.0, w: 6, h: 0.3, fontSize: 9, color: C.text2, margin: 0 } } },
    ],
    slideNumber: { x: 12.3, y: 7.0, w: 0.5, h: 0.3, fontSize: 9, color: H.dk2, align: "right" },
  });
  pres.defineSlideMaster({
    title: "TITLE",
    background: { color: BG },
    objects: [
      { placeholder: { options: { name: "title", type: "title", x: 0.8, y: 1.6, w: 7.2, h: 2.2, fontSize: 48, bold: true, color: C.text1, valign: "middle", align: "left", margin: 0, lineSpacingMultiple: 1.05 }, text: "" } },
      { placeholder: { options: { name: "sub", type: "body", x: 0.8, y: 4.3, w: 7.2, h: 0.9, fontSize: 15, color: C.text2, valign: "top", margin: 0 }, text: "" } },
    ],
  });

  // ── 아이콘: react-icons/fa 이름으로 PNG data URI 생성 (캐시) ──
  const iconCache = new Map();
  async function icon(name, color = H.accent1, size = 256) {
    const key = name + color;
    if (iconCache.has(key)) return iconCache.get(key);
    const Comp = fa[name];
    if (!Comp) throw new Error(`react-icons/fa 에 '${name}' 아이콘이 없습니다`);
    const svg = ReactDOMServer.renderToStaticMarkup(React.createElement(Comp, { color: "#" + color, size }));
    const buf = await sharp(Buffer.from(svg)).resize(size, size).png().toBuffer();
    const uri = "image/png;base64," + buf.toString("base64");
    iconCache.set(key, uri);
    return uri;
  }
  const checkW = await icon("FaCheck", H.lt1);

  const sections = new Set();
  // ── 슬라이드 만들기: 섹션·라벨·태그·제목·부제를 한 번에 ──
  function slide({ section = "본문", label, tag: tagText, tagColor, title, sub, cover = false, notes } = {}) {
    if (!sections.has(section)) { pres.addSection({ title: section }); sections.add(section); }
    const s = pres.addSlide({ masterName: cover ? "TITLE" : "CONTENT", sectionTitle: section });
    if (!cover && label) s.addText(label, { placeholder: "label" });
    if (!cover && tagText) tag(s, tagText, 0.6, 0.62, tagColor || H.dk1);
    if (title) s.addText(title, { placeholder: "title" });
    if (sub) s.addText(sub, { placeholder: "sub" });
    if (notes) s.addNotes(notes);
    return s;
  }

  // ── 모티프 부품 ──
  function tag(s, text, x, y, fill = H.dk1, w = 1.15, textColor) {          // 알약 태그
    s.addShape(pres.ShapeType.roundRect, { x, y, w, h: 0.3, rectRadius: 0.06, fill: { color: fill }, line: { color: fill, width: 0 } });
    s.addText(text, { x, y, w, h: 0.3, fontSize: 10.5, bold: true, color: textColor || (fill === LINE || fill === H.lt2 ? H.accent1 : H.lt1), align: "center", valign: "middle", margin: 0, isTextBox: true });
  }
  function num(s, n, x, y, d = 0.36, fill = H.accent1) {                    // 번호 원
    s.addShape(pres.ShapeType.ellipse, { x, y, w: d, h: d, fill: { color: fill }, line: { color: fill, width: 0 } });
    s.addText(String(n), { x, y, w: d, h: d, fontSize: d * 30, bold: true, color: H.lt1, align: "center", valign: "middle", margin: 0, isTextBox: true });
  }
  function step(s, n, runs, x, y, w, fs = 13, fill) {                        // 번호 원 + 한 줄 (runs: 문자열 또는 [{text, options}])
    num(s, n, x, y + 0.06, 0.36, fill);
    s.addText(runs, { x: x + 0.5, y, w: w - 0.5, h: 0.48, fontSize: fs, color: H.dk1, margin: 0, valign: "middle", isTextBox: true });
  }
  function card(s, x, y, w, h, fill = H.lt1, line = LINE) {                  // 흰 카드 + 연보라 테두리 (fill 바꾸면 채움 카드)
    s.addShape(pres.ShapeType.roundRect, { x, y, w, h, rectRadius: 0.1, fill: { color: fill }, line: { color: line, width: fill === line ? 0 : 1 } });
  }
  function callout(s, runs, x, y, w, h, fs = 12) {                           // 연보라 콜아웃 · 보라 글씨
    s.addShape(pres.ShapeType.roundRect, { x, y, w, h, rectRadius: 0.08, fill: { color: LINE }, line: { color: LINE, width: 0 } });
    s.addText(runs, { x: x + 0.2, y, w: w - 0.4, h, fontSize: fs, color: H.accent1, margin: 0, valign: "middle", isTextBox: true });
  }
  function banner(s, runs, x, y, w, h, fill = H.accent2, fs = 13) {          // 채움 배너 (주황 강조 · 보라 결론)
    s.addShape(pres.ShapeType.roundRect, { x, y, w, h, rectRadius: 0.1, fill: { color: fill }, line: { color: fill, width: 0 } });
    s.addText(runs, { x: x + 0.3, y, w: w - 0.6, h, fontSize: fs, color: H.lt1, margin: 0, valign: "middle", isTextBox: true });
  }
  function codeBar(s, lines, x, y, w, h, fs = 11) {                          // 진남색 코드 바 (모노스페이스)
    const arr = Array.isArray(lines) ? lines : [lines];
    s.addShape(pres.ShapeType.roundRect, { x, y, w, h, rectRadius: 0.08, fill: { color: H.dk1 }, line: { color: H.dk1, width: 0 } });
    s.addText(arr.map((t, i) => ({ text: t, options: { breakLine: i < arr.length - 1 } })),
      { x: x + 0.25, y: y + 0.12, w: w - 0.5, h: h - 0.24, fontSize: fs, fontFace: MONO, color: H.lt1, margin: 0, valign: arr.length > 1 ? "top" : "middle", isTextBox: true });
  }
  function chip(s, text, x, y, w = 1.7) {                                    // 명령어·키워드 칩 (연보라 바탕 · 모노)
    s.addShape(pres.ShapeType.roundRect, { x, y, w, h: 0.36, rectRadius: 0.06, fill: { color: H.lt2 }, line: { color: LINE, width: 1 } });
    s.addText(text, { x, y, w, h: 0.36, fontSize: 11, fontFace: MONO, bold: true, color: H.accent1, align: "center", valign: "middle", margin: 0, isTextBox: true });
  }
  function iconTile(s, img, x, y, d = 0.62) {                                // 흰 타일 안 아이콘
    card(s, x, y, d, d);
    s.addImage({ data: img, x: x + d * 0.22, y: y + d * 0.22, w: d * 0.56, h: d * 0.56 });
  }
  function checkRow(s, text, x, y, w, fs = 12) {                             // 진남색 체크 원 + 글
    s.addShape(pres.ShapeType.ellipse, { x, y: y + 0.07, w: 0.3, h: 0.3, fill: { color: H.dk1 }, line: { color: H.dk1, width: 0 } });
    s.addImage({ data: checkW, x: x + 0.07, y: y + 0.14, w: 0.16, h: 0.16 });
    s.addText(text, { x: x + 0.42, y, w: w - 0.42, h: 0.44, fontSize: fs, color: H.dk1, margin: 0, valign: "middle", isTextBox: true });
  }
  function head(s, text, x, y, w, fs = 15) {                                 // 소제목
    s.addText(text, { x, y, w, h: 0.4, fontSize: fs, bold: true, color: H.dk1, margin: 0, valign: "middle", isTextBox: true });
  }
  function para(s, text, x, y, w, h, fs = 12, color = H.dk2) {               // 본문 단락
    s.addText(text, { x, y, w, h, fontSize: fs, color, margin: 0, valign: "top", isTextBox: true });
  }
  const bullets = (items, opts = {}) => items.map((t, i) => ({ text: t, options: { bullet: true, breakLine: i < items.length - 1, paraSpaceAfter: 5, ...opts } }));
  function bulletBox(s, items, x, y, w, h, fs = 12) {
    s.addText(bullets(items, { fontSize: fs, color: H.dk1 }), { x, y, w, h, margin: 0, valign: "top", isTextBox: true });
  }
  function numberCard(s, n, title, body, x, y, w, h, filled = false) {      // "01 제목 본문" 큰 숫자 카드 (filled: 보라 채움 = 강조)
    card(s, x, y, w, h, filled ? H.accent1 : H.lt1, filled ? H.accent1 : LINE);
    const tc = filled ? H.lt1 : H.dk1, nc = filled ? H.lt1 : H.accent1, bc = filled ? "C9BFEF" : H.dk2;
    s.addText(String(n).padStart(2, "0"), { x: x + 0.3, y: y + 0.2, w: w - 0.6, h: 0.6, fontSize: 30, bold: true, color: nc, margin: 0, isTextBox: true });
    s.addText(title, { x: x + 0.3, y: y + 0.9, w: w - 0.6, h: 0.5, fontSize: 18, bold: true, color: tc, margin: 0, valign: "middle", isTextBox: true });
    if (body) s.addText(body, { x: x + 0.3, y: y + 1.5, w: w - 0.6, h: h - 1.7, fontSize: 12, color: filled ? H.lt1 : bc, margin: 0, valign: "top", isTextBox: true });
  }
  function warnBox(s, title, body, x, y, w, h, img) {                        // 빨간 경고 상자
    s.addShape(pres.ShapeType.roundRect, { x, y, w, h, rectRadius: 0.1, fill: { color: DANGER_BG }, line: { color: DANGER_LINE, width: 1 } });
    if (img) s.addImage({ data: img, x: x + 0.2, y: y + 0.2, w: 0.34, h: 0.34 });
    s.addText([{ text: title, options: { bold: true, breakLine: true } }, { text: body, options: {} }],
      { x: x + (img ? 0.7 : 0.25), y: y + 0.12, w: w - (img ? 0.9 : 0.5), h: h - 0.24, fontSize: 11, color: DANGER_TX, margin: 0, valign: "top", isTextBox: true });
  }
  function imageCard(s, imgPath, x, y, w, h, pad = 0.15) {                   // 흰 카드 안 이미지
    card(s, x, y, w, h);
    s.addImage({ path: imgPath, x: x + pad, y: y + pad, w: w - pad * 2, h: h - pad * 2 });
  }
  function phoneMock(s, x, y, w = 3.0, h = 5.6) {                            // 폰 목업 틀 (안은 호출자가 채움)
    s.addShape(pres.ShapeType.roundRect, { x, y, w, h, rectRadius: 0.45, fill: { color: H.lt1 }, line: { color: H.dk1, width: 5 } });
    s.addShape(pres.ShapeType.roundRect, { x: x + w / 2 - 0.5, y: y + 0.18, w: 1.0, h: 0.16, rectRadius: 0.08, fill: { color: H.dk1 }, line: { color: H.dk1, width: 0 } });
    return { x: x + 0.2, y: y + 0.6, w: w - 0.4, h: h - 0.9 };               // 안쪽 영역
  }
  // ── 표: 헤더 셀 · 본문 셀 · 테두리 ──
  const th = (t, fill = H.dk1) => ({ text: t, options: { bold: true, color: H.lt1, fill: { color: fill }, fontSize: 11.5, valign: "middle", align: "center" } });
  const td = (t, i, extra = {}) => ({ text: t, options: { fontSize: 11.5, color: H.dk1, fill: { color: i % 2 ? H.lt2 : H.lt1 }, valign: "middle", align: "center", ...extra } });
  const border = { type: "solid", color: BG, pt: 1.5 };
  function table(s, header, rows, x, y, w, colW, rowH = 0.48, opts = {}) {
    const data = [header.map((h) => th(h))].concat(rows.map((r, i) => r.map((c, j) => td(c, i, opts.cell ? opts.cell(c, i, j) : {}))));
    s.addTable(data, { x, y, w, colW, rowH, border, margin: 0.08 });
  }

  async function finish(outFile) {
    await pres.writeFile({ fileName: outFile });
    const { applyTheme } = require(path.join(__dirname, "apply_theme.js"));
    await applyTheme(outFile, THEME);
    return outFile;
  }

  return {
    pres, C, H, THEME, BG, LINE, MONO, W, HGT, ShapeType: pres.ShapeType,
    icon, slide, tag, num, step, card, callout, banner, codeBar, chip, iconTile, checkRow, head, para, bullets, bulletBox,
    numberCard, warnBox, imageCard, phoneMock, th, td, border, table, finish,
  };
}

module.exports = { createDeck, THEME, BG, LINE, MONO };
