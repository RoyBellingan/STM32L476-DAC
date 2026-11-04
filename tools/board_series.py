#!/usr/bin/env python3
"""Drive the loop and read the board's raw buffer at each level. No scope.

    board_series.py --amps 0.1,0.2 --n 2 --out DIR [--hold]

Each read: amp, settle, optional hold, `sample hp`, `print adc`. The DAQ is read first with `read 10`.
Files: DIR/a<amp>_r<k>.csv with n, raw1, raw2, diff. The drive goes off in a finally block.
"""
import argparse
import csv
import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, '/home/roy/STM32CubeIDE/workspace_2.0.0/BatTester/tools')
from board_read import Shell, read_once          # noqa: E402
from loop_snap import PORT, Board, field         # noqa: E402

MAX_AMP = 1.0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--amps', required=True)
    ap.add_argument('--n', type=int, default=2)
    ap.add_argument('--hold', action='store_true')
    ap.add_argument('--freq', type=float, default=62.5)
    ap.add_argument('--settle', type=float, default=1.0)
    ap.add_argument('--out', required=True)
    args = ap.parse_args()
    amps = [float(a) for a in args.amps.split(',')]
    if any(not 0.0 <= a <= MAX_AMP for a in amps):
        raise SystemExit(f'amps must be 0 .. {MAX_AMP} V')
    os.makedirs(args.out, exist_ok=True)

    sh = Shell()
    daq = Board(PORT)
    try:
        daq.cmd(f'freq {args.freq:g}', ['@F'], timeout=5)
        daq.cmd('pga 8 ext', ['@S'], timeout=5)
        print('amp    DAQ_i_mV  read  board_A  rmsRaw     DPF')
        for amp in amps:
            daq.cmd('off' if amp == 0 else f'amp {amp:.5f}', ['@S'], timeout=5)
            time.sleep(args.settle)
            if args.hold:
                sh.cmd('pin stage1 on', 'ok: pin')
                sh.cmd('pin optohp on', 'ok: pin')
                time.sleep(0.2)
            lines = daq.cmd('read 10', ['@E read'], timeout=20)
            i_line = next((l for l in lines if l.startswith('@C i')), None)
            i_mv = field(i_line, 'a1') * 1e3 if i_line else float('nan')
            for k in range(args.n):
                if args.hold and k:
                    sh.cmd('pin stage1 on', 'ok: pin')
                    sh.cmd('pin optohp on', 'ok: pin')
                    time.sleep(0.2)
                rows, amps_read, rms, dpf = read_once(sh)
                print(f'{amp:5.3f} {i_mv:9.3f} {k:5d} {amps_read:8.3f} {rms:7.1f} {dpf:7.4f}', flush=True)
                with open(os.path.join(args.out, f'a{amp:.3f}_r{k}.csv'), 'w', newline='') as f:
                    w = csv.writer(f)
                    w.writerow(['n', 'raw1', 'raw2', 'diff', 'e1', 'e2'])
                    w.writerows(rows.tolist())
                time.sleep(0.2)
    finally:
        for _ in range(3):
            try:
                if field(daq.cmd('off', ['@S'], timeout=3)[-1], 'amp') == 0.0:
                    print('drive off')
                    break
            except Exception as e:  # noqa: BLE001: the next try, then the warning
                print(f'off: {e}')
        else:
            print('WARNING: the DAQ did not confirm `off`. THE DRIVE CAN BE ON.')
        if args.hold:
            sh.cmd('pin optohp off', 'ok: pin')
            sh.cmd('pin stage1 off', 'ok: pin')
    return 0


if __name__ == '__main__':
    sys.exit(main())
