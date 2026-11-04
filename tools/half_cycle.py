#!/usr/bin/env python3
"""Compare the positive and the negative half cycle of each raw dump in a folder.

    half_cycle.py <folder> <f0>

The negative half is the clean one on FX2607-0050. Its fundamental is fitted on the negative half only (least
squares on sin and cos), so the snap in the positive half does not change it.
A value per count: 3.32 V / 4095 / 20 ohm x 1000 turns = 40.5 mA.
"""
import csv
import glob
import os
import sys

import numpy as np

TS = 4 * 173.0 / 2e6
A_PER_COUNT = 3.32 / 4095 / 20 * 1000


def load(path):
    with open(path) as f:
        rows = list(csv.DictReader(f))
    return {k: np.array([float(r[k]) for r in rows]) for k in ('raw1', 'raw2', 'diff')}


def main():
    folder, f0 = sys.argv[1], float(sys.argv[2])
    print(' file        | neg-half fit A | quiet L | L min  L max | diff min  max | snap | '
          'noise neg  noise pos (counts) | knee neg  knee pos (counts) | knee A')
    paths = sorted(glob.glob(os.path.join(folder, 'a*_r0.csv')) + glob.glob(os.path.join(folder, 'a*/board_adc.csv')))
    for p in paths:
        a = load(p)
        t = np.arange(len(a['diff'])) * TS
        d = a['diff'] - np.median(a['raw1'] - a['raw2'])
        M = np.column_stack((np.sin(2 * np.pi * f0 * t), np.cos(2 * np.pi * f0 * t), np.ones_like(t)))
        # first pass: all samples, to find the half cycles
        c = np.linalg.lstsq(M, a['diff'], rcond=None)[0]
        fit = M @ c
        neg = fit < c[2] - 0.3 * np.hypot(c[0], c[1])
        c2 = np.linalg.lstsq(M[neg], a['diff'][neg], rcond=None)[0]
        fit2 = M @ c2
        amp_counts = np.hypot(c2[0], c2[1])
        res = a['diff'] - fit2
        pos = fit2 > c2[2] + 0.3 * amp_counts
        neg2 = fit2 < c2[2] - 0.3 * amp_counts
        quiet = np.median(a['raw2'])
        off = np.abs(a['raw2'] - quiet) > 12
        snap = int((a['raw2'] > quiet + 300).sum())
        knee_neg = a['diff'][neg2 & (a['raw2'] < quiet - 12)]
        knee_pos = a['diff'][(a['raw2'] <= quiet + 12) & (fit2 > c2[2])]
        kn = knee_neg.max() if len(knee_neg) else float('nan')
        kp = knee_pos.max() if len(knee_pos) else float('nan')
        name = os.path.basename(p) if p.endswith('_r0.csv') else os.path.basename(os.path.dirname(p))[:7]
        print(f"{name:13s}| {amp_counts / np.sqrt(2) * A_PER_COUNT:13.2f} | {quiet:7.0f} | "
              f"{a['raw2'].min():5.0f} {a['raw2'].max():5.0f} | {a['diff'].min():6.0f} {a['diff'].max():5.0f} | {snap:4d} | "
              f"{res[neg2].std():9.1f} {res[pos].std():10.1f} {'':10s}| {kn:8.0f} {kp:9.0f} {'':8s} | "
              f"{kp * A_PER_COUNT:6.1f}")
    return 0


if __name__ == '__main__':
    sys.exit(main())
