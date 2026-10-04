// HTTPS 흉내 (시뮬레이터 전용) — 요청 URL을 가짜 서버(sim::http)에 넘긴다
#pragma once
#include "Arduino.h"
#include "WiFiClientSecure.h"
#define HTTPC_STRICT_FOLLOW_REDIRECTS 1
namespace sim { int http(const std::string& url, std::string& body); }
class HTTPClient {
  std::string url_, body_;
 public:
  void setTimeout(int) {}
  void setFollowRedirects(int) {}
  bool begin(WiFiClientSecure&, const String& url) { url_ = url.s; return true; }
  int GET() { return sim::http(url_, body_); }
  String getString() { return String(body_); }
  void end() {}
};
