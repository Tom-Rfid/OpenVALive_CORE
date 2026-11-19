# OpenVALive - PC builds (simulator, C API example, desktop app, tests)
#
#   make                 build/ols_sim        engine simulator (WAV, benchmark)
#   make demo            build/demo.wav
#   make bench           CPU and RAM on this PC
#   make desktop         build/openvalive-desktop   play the synth live (audio + MIDI + computer keyboard)
#   make windows         build/openvalive-desktop.exe   (cross-compiled with mingw-w64)
#   make test            every automatic test (tests/run_tests.sh)

CC       ?= cc
MINGW_CC ?= x86_64-w64-mingw32-gcc
CFLAGS   ?= -std=c99 -O2 -Wall -Wextra -Wno-unused-parameter -ffp-contract=off
LDLIBS    = -lm
B         = build

CORE_SRC    = $(wildcard core/engines/va/*.c core/engines/drum/*.c) core/engines/bank.c $(wildcard core/live/*.c) core/dsp/dsp_runtime.c
CORE_HDR    = $(wildcard core/dsp/*.h core/engines/*.h core/engines/va/*.h core/engines/drum/*.h core/live/*.h core/api/*.h)
API_SRC     = core/api/ols_synth.c
SIM_SRC     = $(wildcard connectors/pc-sim/*.c)
DESKTOP_SRC = $(wildcard connectors/desktop/*.c)
DESKTOP_HDR = $(wildcard connectors/desktop/*.h)

all: $(B)/ols_sim

$(B):
	mkdir -p $(B)

$(B)/ols_sim: $(SIM_SRC) $(CORE_SRC) $(CORE_HDR) connectors/pc-sim/sim.h | $(B)
	$(CC) $(CFLAGS) -o $@ $(SIM_SRC) $(CORE_SRC) $(LDLIBS)

$(B)/api_example: connectors/c-api/example.c $(API_SRC) $(CORE_SRC) $(CORE_HDR) | $(B)
	$(CC) $(CFLAGS) -o $@ connectors/c-api/example.c $(API_SRC) $(CORE_SRC) $(LDLIBS)

# Desktop app: miniaudio needs pthread and dl on Linux, nothing extra on macOS.
$(B)/openvalive-desktop: $(DESKTOP_SRC) $(DESKTOP_HDR) $(API_SRC) $(CORE_SRC) $(CORE_HDR) | $(B)
	$(CC) $(CFLAGS) -std=gnu99 -o $@ $(DESKTOP_SRC) $(API_SRC) $(CORE_SRC) -lm -lpthread -ldl

# Windows: one static .exe, WinMM for MIDI, nothing to install.
$(B)/openvalive-desktop.exe: $(DESKTOP_SRC) $(DESKTOP_HDR) $(API_SRC) $(CORE_SRC) $(CORE_HDR) | $(B)
	$(MINGW_CC) $(CFLAGS) -std=gnu99 -static -o $@ $(DESKTOP_SRC) $(API_SRC) $(CORE_SRC) -lwinmm -lole32 -lm

desktop: $(B)/openvalive-desktop
windows: $(B)/openvalive-desktop.exe

demo: $(B)/ols_sim
	$(B)/ols_sim demo $(B)/demo.wav

bench: $(B)/ols_sim
	$(B)/ols_sim bench

test:
	sh tests/run_tests.sh

clean:
	rm -rf $(B)

.PHONY: all desktop windows demo bench test clean
