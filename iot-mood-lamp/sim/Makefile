# IoT 무드등 시뮬레이터 — 보드 없이 PC에서 펌웨어를 시나리오대로 돌려봅니다.
#   make run    배선 점검 + 오프라인 V1·V2 + 앱 연동 V1·V2 + Apps Script 테스트
#   make clean  정리
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -O1
INCLUDES  = -I stubs

all: sim_v1 sim_v2 sim_v1_app sim_v2_app

sim_v1: sim_v1.cpp sim_common.h ../v1_relay_bulb/firmware/step3_auto_lamp/step3_auto_lamp.ino $(wildcard stubs/*.h)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ sim_v1.cpp

sim_v2: sim_v2.cpp sim_common.h ../v2_neopixel_color/firmware/step2_auto_mood_lamp/step2_auto_mood_lamp.ino $(wildcard stubs/*.h)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ sim_v2.cpp

sim_v1_app: sim_v1_app.cpp sim_server.h sim_common.h ../v1_relay_bulb/firmware/step4_app_lamp/step4_app_lamp.ino $(wildcard stubs/*.h)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ sim_v1_app.cpp

sim_v2_app: sim_v2_app.cpp sim_server.h sim_common.h ../v2_neopixel_color/firmware/step3_app_mood_lamp/step3_app_mood_lamp.ino $(wildcard stubs/*.h)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ sim_v2_app.cpp

run: all
	python3 check_wiring.py
	./sim_v1
	./sim_v2
	./sim_v1_app
	./sim_v2_app
	node ../apps_script/test/gs_test.js

clean:
	rm -f sim_v1 sim_v2 sim_v1_app sim_v2_app

.PHONY: all run clean
