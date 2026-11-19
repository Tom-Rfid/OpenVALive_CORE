#!/bin/sh
# OpenVALive - every automatic test
#
#   sh tests/run_tests.sh                       PC tests (C compiler; numpy for the audio measurements)
#   FM1_BASE=~/fm1-base sh tests/run_tests.sh   + device checks (the DSP without a C library, fastmath = libm)
#
# Prints one PASS / FAIL / SKIP line per test; exits with 1 if any test fails.
set -u
cd "$(dirname "$0")/.."

CC="${CC:-cc}"
B=build/tests
mkdir -p "$B"
FAILS=0

pass() { echo "PASS  $1"; }
fail() { echo "FAIL  $1"; FAILS=$((FAILS + 1)); }
skip() { echo "SKIP  $1"; }
have_numpy() { python3 -c "import numpy" 2>/dev/null; }

CORE="$(ls core/engines/va/*.c core/engines/drum/*.c core/engines/bank.c core/live/*.c) core/dsp/dsp_runtime.c"
STRICT="-std=c99 -O2 -Wall -Wextra -Wno-unused-parameter -Wdouble-promotion -Werror -ffp-contract=off"

# 1. The core compiles without a single warning (including hidden doubles)
STRICT_OK=1
for f in $CORE core/api/ols_synth.c; do
    $CC $STRICT -c "$f" -o "$B/strict.o" || STRICT_OK=0
done
if [ $STRICT_OK = 1 ]; then pass "core: strict build (-Werror -Wdouble-promotion)"; else fail "core: strict build"; fi

# 1b. Live Core: clock, sequencer timing to the sample, record, capture, undo, Creative Engine,
#     scenes, morph, project files, MIDI clock in, stutter
if $CC -std=c99 -O2 -Wall -Wextra -Wno-unused-parameter -ffp-contract=off -o $B/live_test tests/live_test.c $CORE -lm \
   && out=$($B/live_test); then
    pass "$out"
else
    echo "$out"
    fail "live core"
fi

# 1c. Every synth sound: a held note plays (no NaN, not silent, not blowing up) and ends quiet
if $CC -std=c99 -O2 -Wall -Wextra -Wno-unused-parameter -o $B/presets_test tests/presets_test.c $CORE core/api/ols_synth.c -lm \
   && out=$($B/presets_test); then
    pass "$out"
else
    echo "$out"
    fail "synth sounds"
fi

# 2. Simulator: full demo, no NaN, no clipping
if make -s build/ols_sim >/dev/null 2>&1; then
    out=$(build/ols_sim demo $B/demo.wav)
    if echo "$out" | grep -q "NaN/Inf: 0 |" && echo "$out" | grep -q "clipped: 0"; then
        pass "simulator: 33 s demo, no NaN, no clipping"
    else
        echo "$out"
        fail "simulator: demo"
    fi
else
    fail "simulator: build"
fi

# 3. C API: the example builds and runs
if make -s build/api_example >/dev/null 2>&1 && build/api_example $B/api.wav >/dev/null; then
    pass "C API: example at 44.1 kHz"
else
    fail "C API: example"
fi

# 4. Desktop app: builds and renders offline
if make -s build/openvalive-desktop >/dev/null 2>&1 \
   && build/openvalive-desktop --render $B/desktop.wav --seconds 3 --preset pad --demo >/dev/null; then
    pass "desktop app: build + offline render"
else
    fail "desktop app"
fi

# 5. Pitch at several sample rates + aliasing (numpy)
if $CC -std=c99 -O2 -ffp-contract=off -o $B/tone tests/tone.c core/api/ols_synth.c $CORE -lm 2>/dev/null; then
    if have_numpy; then
        for sr in 44100 48000 96000; do
            $B/tone $sr 69 1.5 > $B/tone_$sr.f32
            if python3 tests/analyze.py pitch $B/tone_$sr.f32 $sr 440; then pass "pitch: A440 at $sr Hz"; else fail "pitch at $sr Hz"; fi
        done
        $B/tone 48000 93 1.0 > $B/alias.f32
        if python3 tests/analyze.py alias $B/alias.f32 48000 1760 -25; then pass "aliasing: saw at 1760 Hz < -25 dB"; else fail "aliasing"; fi
    else
        skip "audio measurements (pip install numpy)"
    fi
else
    fail "tone: build"
fi

# 6-7. device checks (when fm1-base is available: its fastmath.h)
if [ -n "${FM1_BASE:-}" ] && [ -f "$FM1_BASE/include/fastmath.h" ]; then
    # 6. built like on the device: no C library, fastmath, fixed sample rate
    GINC=$($CC -print-file-name=include)
    if $CC -std=c99 -O2 -ffreestanding -nostdinc -isystem "$GINC" -I"$FM1_BASE/include" $STRICT \
         -c app/ols_dsp.c -o $B/fm1_dsp.o; then
        pass "FM-1: DSP builds without a C library (like the JieLi compiler)"
    else
        fail "FM-1: no-libc build"
    fi

    # 7. same sound with fastmath (device) as with libm (PC)
    if have_numpy && $CC -std=c99 -O2 -ffp-contract=off -DOLS_FASTMATH -I"$FM1_BASE/include" -o $B/sim_fast \
         connectors/pc-sim/*.c $CORE -lm 2>/dev/null; then
        $B/sim_fast demo $B/fast.wav >/dev/null
        if python3 - "$B/demo.wav" "$B/fast.wav" <<'EOF'
import sys, wave, numpy as np
def read(p):
    w = wave.open(p)
    return np.frombuffer(w.readframes(w.getnframes()), dtype=np.int16).astype(float)
a, b = read(sys.argv[1]), read(sys.argv[2])
d = a - b
snr = 10 * np.log10((a ** 2).sum() / max((d ** 2).sum(), 1e-9))
print(f"  libm / fastmath difference: {snr:.1f} dB below the signal")
sys.exit(0 if snr > 40 else 1)
EOF
        then pass "FM-1: fastmath = libm (> 40 dB)"; else fail "FM-1: fastmath vs libm"; fi
    else
        skip "FM-1: fastmath vs libm (numpy)"
    fi

else
    skip "device checks (set FM1_BASE=path/to/fm1-base)"
fi

echo
if [ $FAILS -eq 0 ]; then
    echo "All tests pass."
else
    echo "$FAILS test(s) failed."
    exit 1
fi
