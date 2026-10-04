# step5_locker.ino 시뮬레이터 — 보드 없이 PC에서 스케치를 돌려봅니다.
#   make        빌드
#   make run    빌드 후 실행
#   make clean  정리
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -O1
INCLUDES  = -I stubs -I stubs/freertos

sim: sim_main.cpp ../step5_locker.ino $(wildcard stubs/*.h stubs/freertos/*.h)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ sim_main.cpp

run: sim
	./sim

clean:
	rm -f sim

.PHONY: run clean
