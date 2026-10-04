/*
  4단계 — 키오스크 DB 서버 완성본 (Apps Script)
  ----------------------------------------------------
  action=order : 주문 저장 + 주문번호 발급 (3단계와 동일)
  action=sales : 매출 집계 JSON — 총 주문 수·총매출·인기 메뉴·최근 주문

  ※ 3단계 코드를 이 파일로 교체하고 다시 배포하세요 (손님 앱 연결은 그대로 동작).
  테스트: 브라우저로  배포URL?action=sales
*/

function doGet(e) {
  const a = e.parameter.action;
  if (a == "order") return order(e);
  if (a == "sales") return sales();
  return ContentService.createTextOutput("unknown");
}

// 주문 저장 + 주문번호 발급
function order(e) {
  const ss = SpreadsheetApp.getActiveSpreadsheet();
  const log = ss.getSheetByName("주문로그");
  const no = log.getLastRow();               // 제목이 1행 → 첫 주문번호는 1
  log.appendRow([new Date(), no, e.parameter.items, Number(e.parameter.total)]);
  return ContentService.createTextOutput(String(no));
}

// 매출 집계 — 주문로그 전체를 읽어 합계·순위 계산 (6회차 통계와 같은 원리)
function sales() {
  const ss = SpreadsheetApp.getActiveSpreadsheet();
  const rows = ss.getSheetByName("주문로그").getDataRange().getValues();

  let revenue = 0;        // 총매출
  const count = {};       // 메뉴별 판매 수

  for (let i = 1; i < rows.length; i++) {          // 0번째 줄은 제목 행!
    revenue += Number(rows[i][3]);                 // 합계 열
    String(rows[i][2]).split("|").forEach(function(one) {   // 내역: "아메리카노(핫):1500|..."
      const name = one.split(":")[0].split("(")[0];         // 옵션 떼고 메뉴명만
      if (name) count[name] = (count[name] || 0) + 1;
    });
  }

  // 인기 메뉴 TOP 5
  const top = Object.keys(count)
    .map(function(k) { return { name: k, count: count[k] }; })
    .sort(function(a, b) { return b.count - a.count; })
    .slice(0, 5);

  // 최근 주문 5건 (최신순)
  const recent = [];
  for (let i = rows.length - 1; i >= 1 && recent.length < 5; i--) {
    recent.push({
      no: rows[i][1],
      items: rows[i][2],
      total: rows[i][3],
      time: Utilities.formatDate(new Date(rows[i][0]), "Asia/Seoul", "MM/dd HH:mm")
    });
  }

  const result = { orders: rows.length - 1, revenue: revenue, top: top, recent: recent };
  return ContentService.createTextOutput(JSON.stringify(result))
    .setMimeType(ContentService.MimeType.JSON);
}
