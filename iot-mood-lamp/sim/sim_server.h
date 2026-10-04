// 가짜 중계 서버 — apps_script/mood_lamp.gs 와 같은 규칙(팀별 대기열·같은 종류는 마지막 것만)을 흉내
#pragma once
#include "sim_common.h"
#include "WiFi.h"
#include <map>
#include <vector>

namespace sim {
  bool wifiUp = false;
  int wifiBegins = 0;
  std::map<std::string, int> nvs;
  bool serverDown = false;

  struct Item { std::string cmd, value; int dark = 0, off = 0; };
  std::map<std::string, std::vector<Item>> queues;
  std::map<std::string, std::map<std::string, std::string>> state;   // team → 마지막 보고
  std::map<std::string, int> calls;                                   // mode별 요청 수
  std::string lastUrl;

  std::map<std::string, std::string> query(const std::string& url) {
    std::map<std::string, std::string> q;
    size_t p = url.find('?');
    if (p == std::string::npos) return q;
    std::string rest = url.substr(p + 1);
    size_t i = 0;
    while (i < rest.size()) {
      size_t amp = rest.find('&', i); if (amp == std::string::npos) amp = rest.size();
      std::string kv = rest.substr(i, amp - i);
      size_t eq = kv.find('=');
      if (eq != std::string::npos) q[kv.substr(0, eq)] = kv.substr(eq + 1);
      i = amp + 1;
    }
    return q;
  }
  bool powerGroup(const std::string& c) { return c == "ON" || c == "OFF" || c == "AUTO"; }

  // 앱이 명령을 넣는 것 (?mode=set)
  void appSet(const std::string& team, const std::string& cmd, const std::string& value = "", int dark = 0, int off = 0) {
    auto& q = queues[team];
    std::vector<Item> kept;
    for (auto& it : q) if (!(it.cmd == cmd || (powerGroup(it.cmd) && powerGroup(cmd)))) kept.push_back(it);
    kept.push_back({cmd, value, dark, off});
    q = kept;
  }

  int http(const std::string& url, std::string& body) {
    lastUrl = url;
    if (serverDown) { calls["fail"]++; return -1; }
    auto q = query(url);
    std::string mode = q["mode"], team = q["team"];
    calls[mode]++;
    if (mode == "next") {
      auto& qu = queues[team];
      if (qu.empty()) { body = "{\"cmd\":\"\"}"; return 200; }
      Item it = qu.front(); qu.erase(qu.begin());
      body = "{\"cmd\":\"" + it.cmd + "\",\"value\":\"" + it.value + "\"";
      if (it.cmd == "CONFIG") body += ",\"dark\":" + std::to_string(it.dark) + ",\"off\":" + std::to_string(it.off);
      body += "}";
      return 200;
    }
    if (mode == "report") { q.erase("mode"); state[team] = q; body = "{\"ok\":true}"; return 200; }
    if (mode == "state") { body = "{\"ok\":true}"; return 200; }
    body = "{\"ok\":false}"; return 200;
  }
}
WiFiSim WiFi;
