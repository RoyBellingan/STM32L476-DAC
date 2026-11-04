#!/usr/bin/env python3
"""E-field channels of the raw dumps: amplitude at 50 and 62.5 Hz.

    efield_bins.py <folder>

231 scans x 346 us = 79.9 ms, so the bins are 12.51 Hz: 50 Hz is bin 4, 62.5 Hz is bin 5. They do not leak into each
other. A straight line is removed first (the channels drift after the stage1 turn-on).
Values are peak counts, and the phase is against the fundamental of `diff` at the same bin.
"""
import csv
import glob
import os
import sys

import numpy as np


def load(path):
    with open(path) as f:
        rows = list(csv.DictReader(f))
    return {k: np.array([float(r[k]) for r in rows]) for k in ('diff', 'e1', 'e2')}


def bin_c(x, k):
    n = np.arange(len(x))
    y = x - np.polyval(np.polyfit(n, x, 1), n)
    return 2.0 / len(y) * np.sum(y * np.exp(-2j * np.pi * k * n / len(y)))


def main():
    folder = sys.argv[1]
    read = sys.argv[2] if len(sys.argv) > 2 else 'r0'
    paths = sorted(glob.glob(os.path.join(folder, f'a*_{read}.csv')) + glob.glob(os.path.join(folder, 'a*/board_adc.csv')))
    print('file     | e1 mean min max  | e1@50 e1@62.5 ph   | e2 mean min max | e2@50 e2@62.5 | diff@62.5')
    for p in paths:
        a = load(p)
        name = os.path.basename(p)[:6] if not p.endswith('board_adc.csv') else os.path.basename(os.path.dirname(p))[:7]
        d5 = bin_c(a['diff'], 5)
        e50, e62 = bin_c(a['e1'], 4), bin_c(a['e1'], 5)
        f50, f62 = bin_c(a['e2'], 4), bin_c(a['e2'], 5)
        ph = np.degrees(np.angle(e62 / d5)) if abs(d5) > 0 else 0.0
        print(f"{name:8s} | {a['e1'].mean():5.0f} {a['e1'].min():4.0f} {a['e1'].max():4.0f} | {abs(e50):6.1f} {abs(e62):6.1f} {ph:6.0f} | "
              f"{a['e2'].mean():5.0f} {a['e2'].min():4.0f} {a['e2'].max():4.0f} | {abs(f50):6.1f} {abs(f62):6.1f} | {abs(d5):7.1f}")
    return 0


if __name__ == '__main__':
    sys.exit(main())
