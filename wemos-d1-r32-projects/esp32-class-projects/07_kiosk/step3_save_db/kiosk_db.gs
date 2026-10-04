/*
  3단계 — 키오스크 DB 서버 (Apps Script)
  ----------------------------------------------------
  주문(action=order)을 받아 '주문로그' 시트에 저장하고 주문번호를 발급한다.

  준비: 구글 시트에 '주문로그' 탭 + 1행 제목 [시각 | 주문번호 | 내역 | 합계]
  배포: 배포 → 웹 앱 → 액세스 '모든 사용자' → URL 복사 (docs/setup.md 참고)
  테스트: 브라우저로  배포URL?action=order&items=아메리카노(핫):1500&total=1500
*/

function doGet(e) {
  const a = e.parameter.action;
  if (a == "order") return order(e);
  return ContentService.createTextOutput("unknown");
}

// 주문 저장 + 주문번호 발급
function order(e) {
  const ss = SpreadsheetApp.getActiveSpreadsheet();
  const log = ss.getSheetByName("주문로그");

  const no = log.getLastRow();               // 제목이 1행 → 첫 주문번호는 1
  const items = e.parameter.items;           // 예: "아메리카노(핫):1500|레몬티(아이스):1500"
  const total = Number(e.parameter.total);

  log.appendRow([new Date(), no, items, total]);   // 주문로그 1줄 = 매출 데이터!

  return ContentService.createTextOutput(String(no));  // 발급한 주문번호 회신
}
