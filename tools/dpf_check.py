#!/usr/bin/env python3
"""Distortion PF from the raw dumps: fundamental / rms of `diff`, at a chosen frequency.

    dpf_check.py <folder> <f0>

The firmware's P28 DPF is the same ratio, with a sine table fixed at 50 Hz. At f0 = 62.5 Hz the table looks at an
empty bin, so only a value from the raw buffer means something there.
I x DPF is the fundamental, in A: what the power calculation of the `full` mode uses.
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
    return np.array([float(r['diff']) for r in rows])


def main():
    folder, f0 = sys.argv[1], float(sys.argv[2])
    print('file            rms_counts  I_rms_A  DPF  I_x_DPF_A')
    for p in sorted(glob.glob(os.path.join(folder, 'a*_r?.csv'))):
        x = load(p)
        y = x - x.mean()
        t = np.arange(len(y)) * TS
        fund = abs(2.0 / len(y) * np.sum(y * np.exp(-2j * np.pi * f0 * t))) / np.sqrt(2)
        rms = y.std()
        print(f'{os.path.basename(p):15s} {rms:10.1f} {rms * A_PER_COUNT:8.3f} {fund / rms:6.4f} {fund * A_PER_COUNT:8.3f}')
    return 0


if __name__ == '__main__':
    sys.exit(main())
