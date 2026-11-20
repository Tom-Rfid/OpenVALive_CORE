#!/usr/bin/env python3
"""Sets every synth sound's VOLUME so they all sound about as loud (tools/level_sounds.c measures them).

    python3 tools/level_sounds.py            # measure, set, measure again (3 rounds), rewrite the presets file
    python3 tools/level_sounds.py --check    # measure only: the spread, and the sounds far from the target

The target is the median loudness of the sounds (so the overall level stays where it is), or --target DB. A sound
goes no louder than its peak allows (--headroom, 3 dB under full scale by default: the output is a tanh, a sound
pushed past it only gets harsher) and moves at most 12 dB in a run. VOLUME is squared inside the engine
(gain = 2 v^2), so a change of d dB multiplies v by 10^(d/40). The output's tanh bends loud sounds: hence the rounds.
For a new sound: add it, run this, listen.
"""
import argparse, math, os, re, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
PRESETS = os.path.join(ROOT, 'core/engines/va/engine_va_presets.c')
EXE = os.path.join(ROOT, 'build/level_sounds')
DEFAULT_VOLUME = 0.7                                   # the presets' base (engine_va_presets.c BASE)


def build():
    os.makedirs(os.path.dirname(EXE), exist_ok=True)
    core = []
    for d in ('core/engines/va', 'core/engines/drum', 'core/live'):
        core += [os.path.join(ROOT, d, f) for f in sorted(os.listdir(os.path.join(ROOT, d))) if f.endswith('.c')]
    core += [os.path.join(ROOT, 'core/engines/bank.c'), os.path.join(ROOT, 'core/dsp/dsp_runtime.c'),
             os.path.join(ROOT, 'core/api/ols_synth.c'), os.path.join(HERE, 'level_sounds.c')]
    subprocess.run(['cc', '-std=c99', '-O2', '-D_DEFAULT_SOURCE', '-o', EXE] + core + ['-lm'], check=True)


def measure():
    out = subprocess.run([EXE], check=True, capture_output=True, text=True).stdout
    rows = {}
    for line in out.strip().splitlines():
        i, name, loud, peak, mean = line.split('\t')
        rows[int(i)] = (name, float(loud), float(peak), float(mean))
    return rows


def tables(src):
    """preset index -> its CHANGES table's name, in the enum's order"""
    enum = re.search(r'typedef enum \{(.*?)VA_PRESET_COUNT', open(os.path.join(ROOT, 'core/engines/va/engine_va.h')).read(), re.S).group(1)
    order = re.findall(r'VA_PRESET_(\w+)', enum)
    named = dict(re.findall(r'\[VA_PRESET_(\w+)\]\s*=\s*CHANGES\((\w+)\)', src))
    return {i: named.get(k) for i, k in enumerate(order)}


def volume_of(src, table):
    body = re.search(r'static const va_change_t %s\[\] = \{(.*?)\n\};' % table, src, re.S).group(1)
    m = re.search(r'\{VA_P_VOLUME,\s*([0-9.]+)f\}', body)
    return float(m.group(1)) if m else None


def set_volume(src, table, v):
    m = re.search(r'(static const va_change_t %s\[\] = \{)(.*?)(\n\};)' % table, src, re.S)
    body = m.group(2)
    text = ('%.3f' % v).rstrip('0')
    text = (text + '0' if text.endswith('.') else text) + 'f'
    if re.search(r'\{VA_P_VOLUME,\s*[0-9.]+f\}', body):
        body = re.sub(r'\{VA_P_VOLUME,\s*[0-9.]+f\}', '{VA_P_VOLUME, %s}' % text, body)
    else:
        body = body.rstrip() + ('' if body.rstrip().endswith(',') else ',') + '\n    {VA_P_VOLUME, %s},' % text
    return src[:m.start(2)] + body + src[m.end(2):]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--check', action='store_true')
    ap.add_argument('--target', type=float)
    ap.add_argument('--headroom', type=float, default=3.0)
    ap.add_argument('--rounds', type=int, default=3)
    a = ap.parse_args()
    build()
    rows = measure()
    target = a.target if a.target is not None else sorted(r[1] for r in rows.values())[len(rows) // 2]
    if a.check:
        for i, (name, loud, peak, _) in sorted(rows.items(), key=lambda x: x[1][1]):
            print('%-14s %6.1f dB  peak %6.1f  %+5.1f' % (name, loud, peak, loud - target))
        spread = [abs(r[1] - target) for r in rows.values() if r[2] < -a.headroom - 0.5]
        print('target %.1f dB; %d sounds, %d more than 1.5 dB away' % (target, len(rows), sum(d > 1.5 for d in spread)))
        return
    first = dict(rows)
    for _ in range(a.rounds):
        src = open(PRESETS).read()
        tab = tables(src)
        for i, (name, loud, peak, _) in rows.items():
            t = tab.get(i)
            if not t:
                continue
            d = min(target - loud, -a.headroom - peak)
            d = max(-12.0, min(12.0, d))
            v = volume_of(src, t) or DEFAULT_VOLUME
            src = set_volume(src, t, max(0.05, min(1.0, v * 10 ** (d / 40.0))))
        open(PRESETS, 'w').write(src)
        build()
        rows = measure()
    print('target %.1f dB' % target)
    for i, (name, loud, peak, _) in sorted(rows.items(), key=lambda x: x[1][1]):
        print('%-14s %6.1f -> %6.1f dB  (peak %5.1f)' % (name, first[i][1], loud, peak))


if __name__ == '__main__':
    main()
