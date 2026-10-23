// 예시 빌드 — 표지 1장 + 본문 3장. 복사해서 내용만 바꾼다.
//   cd <빌드 폴더> && node example_build.js out.pptx
const path = require("path");
const { createDeck } = require(path.join(__dirname, "..", "scripts", "deck.js"));
const OUT = process.argv[2] || "example.pptx";

(async () => {
  const d = await createDeck({ title: "예시 덱", author: "", footer: "예시 덱 · 학생용" });
  const H = d.H;

  // 1. 표지 — 왼쪽 글 + 오른쪽 폰 목업
  let s = d.slide({ section: "시작", cover: true, notes: "[교사 노트] 완성본을 먼저 보여주고 시작한다." });
  d.tag(s, "IoT 실습 · 3시간", 0.8, 0.9, d.LINE, 1.6);
  s.addText([{ text: "3시간 뒤,", options: { breakLine: true } }, { text: "무드등 개발자 탄생", options: {} }], { placeholder: "title" });
  s.addText([{ text: "보드 · 센서 · 앱으로 만드는", options: { breakLine: true } }, { text: "\"어두워지면 저절로 켜지는 조명\"", options: { bold: true, color: H.dk1 } }], { placeholder: "sub" });
  for (const [i, name] of ["FaMicrochip", "FaSun", "FaPalette", "FaMobileAlt"].entries()) d.iconTile(s, await d.icon(name), 0.8 + i * 0.78, 5.5);
  const inner = d.phoneMock(s, 9.0, 1.0);
  s.addText("나만의 무드등", { x: inner.x, y: inner.y, w: inner.w, h: 0.4, fontSize: 15, bold: true, color: H.dk1, align: "center", valign: "middle", margin: 0, isTextBox: true });
  d.card(s, inner.x + 0.3, inner.y + 0.6, inner.w - 0.6, 1.6, H.lt2);

  // 2. 따라 하기 — 스크린샷 카드 + 번호 단계 + 콜아웃
  s = d.slide({ section: "실습", label: "① 배선", tag: "따라 하기", title: "실드에 3개만 꽂으면 끝", sub: "모듈 핀 이름을 보고 S·V·G에 한 가닥씩" });
  d.card(s, 0.6, 2.2, 7.3, 4.0);                       // 이 자리에 d.imageCard(s, "그림.png", 0.6, 2.2, 7.3, 4.0)
  d.step(s, 1, [{ text: "네오픽셀 → 실드 " }, { text: "6번", options: { bold: true, color: H.accent1 } }], 8.3, 2.2, 4.4);
  d.step(s, 2, "PIR → 실드 3번", 8.3, 2.8, 4.4);
  d.step(s, 3, "조도센서 → A3 · VCC는 3V3", 8.3, 3.4, 4.4);
  d.warnBox(s, "SEL 점퍼는 뺀다", "점퍼를 꽂은 채 외부 전원을 넣으면 합선.", 8.3, 4.3, 4.45, 1.3, await d.icon("FaExclamationTriangle", H.accent4));
  d.callout(s, [{ text: "성공 기준: ", options: { bold: true } }, { text: "전원을 넣어도 연기가 나지 않는다." }], 0.6, 6.3, 12.1, 0.45, 11);

  // 3. 코드 + 명령어 칩 + 증상 카드
  s = d.slide({ section: "실습", label: "② 출력 테스트", tag: "실습 · 20분", tagColor: H.accent1, title: "무지개가 흐르면 성공", sub: "아래 설정은 바꾸지 않습니다" });
  d.codeBar(s, ["#define LED_PIN    27", "#define LED_COUNT  16", "strip.setBrightness(50);"], 0.6, 2.2, 7.3, 1.3);
  d.head(s, "시리얼에 쳐 보기", 0.6, 3.7, 4);
  [["c red", "빨강"], ["rainbow", "무지개"], ["off", "끄기"]].forEach(([c, w], i) => { d.chip(s, c, 0.6, 4.15 + i * 0.42); d.para(s, w, 2.45, 4.15 + i * 0.42, 5, 0.36, 12, H.dk1); });
  d.head(s, "이렇게 보이면", 8.3, 2.15, 4);
  [["아예 안 켜짐", "전원 · 점퍼 · DIN 방향"], ["끝쪽만 안 켜짐", "LED_COUNT 확인"]].forEach(([a, b], i) => {
    d.card(s, 8.3, 2.6 + i * 1.05, 4.45, 0.92);
    d.head(s, a, 8.5, 2.68 + i * 1.05, 4.1, 11.5);
    d.para(s, "→ " + b, 8.5, 3.06 + i * 1.05, 4.1, 0.38, 11.5, H.accent1);
  });

  // 4. 큰 숫자 카드 + 표 + 마무리 배너
  s = d.slide({ section: "마무리", label: "마무리", title: "오늘 해낸 것", sub: "되는 것만 보여줍니다" });
  ["센서로 켠다", "폰으로 켠다", "말로 켠다", "색을 바꾼다"].forEach((t, i) => d.numberCard(s, i + 1, t, null, 0.6 + i * 3.07, 2.2, 2.85, 1.7, i === 3));
  d.table(s, ["시간", "할 일", "성공 기준"], [["0:00–0:15", "완성품 구경", "—"], ["0:15–0:45", "배선", "연기 안 남"]], 0.6, 4.2, 12.1, [1.6, 7.6, 2.9], 0.45,
    { cell: (c, i, j) => ({ align: j === 1 ? "left" : "center", color: j === 0 ? H.accent1 : H.dk1, bold: j === 0 }) });
  d.banner(s, [{ text: "오늘 만든 것은 진짜 제품의 축소판입니다.", options: { bold: true } }], 0.6, 6.0, 12.1, 0.6, H.accent1);

  await d.finish(OUT);
  console.log("written", OUT);
})().catch((e) => { console.error(e); process.exit(1); });
