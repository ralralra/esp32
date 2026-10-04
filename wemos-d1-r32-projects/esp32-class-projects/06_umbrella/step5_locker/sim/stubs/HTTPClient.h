#pragma once
#include "Arduino.h"
#include "WiFiClientSecure.h"
#define HTTP_CODE_OK 200
#define HTTPC_STRICT_FOLLOW_REDIRECTS 1
class HTTPClient {
 public:
  void setTimeout(int) {}
  void setFollowRedirects(int) {}
  bool begin(WiFiClientSecure&, const String&) { return true; }
  int  GET() { return 200; }
  String getString() { return String(""); }
  void end() {}
};
