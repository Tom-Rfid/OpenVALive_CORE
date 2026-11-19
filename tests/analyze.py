#!/usr/bin/env python3
"""Automatic audio measurements (numpy): pitch accuracy and aliasing.
   analyze.py pitch FILE.f32 SAMPLE_RATE EXPECTED_HZ    -> error in cents (fails above 2)
   analyze.py alias FILE.f32 SAMPLE_RATE F0 MAX_DB      -> energy outside the harmonics (fails above MAX_DB)"""
import sys
import numpy as np


def load(path, skip):
    x = np.fromfile(path, dtype=np.float32)
    return x[skip:]


def pitch(path, sr, expected):
    x = load(path, int(0.2 * sr))
    n = 1 << int(np.log2(len(x)))
    x = x[:n] * np.hanning(n)
    s = np.abs(np.fft.rfft(x))
    k = int(np.argmax(s[1:])) + 1
    a, b, c = np.log(s[k - 1] + 1e-12), np.log(s[k] + 1e-12), np.log(s[k + 1] + 1e-12)
    k = k + 0.5 * (a - c) / (a - 2 * b + c)          # parabolic interpolation
    f = k * sr / n
    cents = 1200 * np.log2(f / expected)
    print(f"  pitch: {f:.2f} Hz, expected {expected:.2f} Hz -> {cents:+.2f} cents")
    return abs(cents) <= 2.0


def alias(path, sr, f0, max_db):
    x = load(path, int(0.2 * sr))[:8192]
    s = np.abs(np.fft.rfft(x * np.blackman(len(x)))) ** 2
    fr = np.fft.rfftfreq(len(x), 1 / sr)
    m = np.ones_like(s, bool)
    for k in range(1, int(sr / 2 / f0) + 2):
        m &= ~(np.abs(fr - k * f0) < 40)
    db = 10 * np.log10(s[m & (fr > 20)].sum() / s.sum())
    print(f"  aliasing at {f0:.0f} Hz: {db:.1f} dB (max {max_db} dB)")
    return db <= max_db


if __name__ == "__main__":
    mode, path, sr = sys.argv[1], sys.argv[2], float(sys.argv[3])
    ok = pitch(path, sr, float(sys.argv[4])) if mode == "pitch" else alias(path, sr, float(sys.argv[4]), float(sys.argv[5]))
    sys.exit(0 if ok else 1)
