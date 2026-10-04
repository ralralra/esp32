# IoT 무드등 시뮬레이터 — 보드 없이 PC에서 펌웨어를 시나리오대로 돌려봅니다.
#   make run    V1·V2 빌드 후 실행 + 배선 점검
#   make clean  정리
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -O1
INCLUDES  = -I stubs

all: sim_v1 sim_v2

sim_v1: sim_v1.cpp sim_common.h ../v1_relay_bulb/firmware/step3_auto_lamp/step3_auto_lamp.ino $(wildcard stubs/*.h)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ sim_v1.cpp

sim_v2: sim_v2.cpp sim_common.h ../v2_neopixel_color/firmware/step2_auto_mood_lamp/step2_auto_mood_lamp.ino $(wildcard stubs/*.h)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ sim_v2.cpp

run: all
	python3 check_wiring.py
	./sim_v1
	./sim_v2

clean:
	rm -f sim_v1 sim_v2

.PHONY: all run clean
