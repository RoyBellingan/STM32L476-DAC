#!/usr/bin/env python3
"""Analyse the board_adc.csv of each capture of a loop_scope run.

    board_wave.py <raw folder of the run> [--show 0.75]

ADC: 12 bit, VDDA about 3.32 V. Scan period 346 us (4 x 173 cycles at 2 MHz).
"""
import csv
import glob
import os
import sys

import numpy as np

TS = 4 * 173.0 / 2e6
HEAD = 12


def load(path):
    with open(path) as f:
        rows = list(csv.DictReader(f))
    return {k: np.array([float(r[k]) for r in rows]) for k in rows[0]}


def dft(x, f0):
    t = np.arange(len(x)) * TS
    y = x - x.mean()
    return np.array([2.0 / len(y) * np.sum(y * np.exp(-2j * np.pi * k * f0 * t)) for k in range(0, 9)])


def main():
    root = sys.argv[1]
    show = float(sys.argv[sys.argv.index('--show') + 1]) if '--show' in sys.argv else None
    print(' amp  | raw1 min max  | raw2 min max  | mean1 mean2 | diff min  max | rms all rest | fund rms | thd% | head peak')
    for d in sorted(glob.glob(os.path.join(root, 'a*_*'))):
        p = os.path.join(d, 'board_adc.csv')
        if not os.path.exists(p):
            continue
        amp = float(os.path.basename(d)[1:7])
        a = load(p)
        diff = a['diff'] - a['diff'].mean()
        c = dft(a['diff'][HEAD:], 62.5)
        fund = abs(c[1]) / np.sqrt(2)
        thd = np.sqrt(np.sum(np.abs(c[2:]) ** 2)) / abs(c[1]) * 100
        print(f"{amp:5.3f} | {a['raw1'].min():5.0f} {a['raw1'].max():5.0f}   | {a['raw2'].min():5.0f} {a['raw2'].max():5.0f}   | "
              f"{a['raw1'].mean():5.0f} {a['raw2'].mean():5.0f} | {a['diff'].min():6.0f} {a['diff'].max():6.0f} | "
              f"{diff.std():7.1f} {diff[HEAD:].std():7.1f} | {fund:8.1f} | {thd:5.1f} | {np.abs(diff[:HEAD]).max():6.0f}")
        if show is not None and abs(amp - show) < 1e-6:
            print('   n raw1 raw2 diff')
            for i in range(len(diff)):
                print(f"{int(a['n'][i]):4d} {a['raw1'][i]:5.0f} {a['raw2'][i]:5.0f} {a['diff'][i]:6.0f}")
    return 0


if __name__ == '__main__':
    sys.exit(main())
