#!/usr/bin/env python3
import argparse
import os
import subprocess
import sys
from pathlib import Path

def main() -> int:
    parser = argparse.ArgumentParser(description='Run the phase 67 public benchmark suite and save CSV/summary outputs.')
    parser.add_argument('--exe', default='build/zragf_bench_phase67_public', help='Path to benchmark executable')
    parser.add_argument('--out-dir', default='bench/results/phase67', help='Directory where CSV and summary are written')
    parser.add_argument('--repeat', type=int, default=3)
    parser.add_argument('--warmup', type=int, default=1)
    parser.add_argument('--quick', action='store_true')
    parser.add_argument('files', nargs='*', help='Optional external corpus files')
    args = parser.parse_args()

    exe = Path(args.exe)
    out_dir = Path(args.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    csv_path = out_dir / 'phase67_public.csv'
    summary_path = out_dir / 'phase67_public.md'

    cmd = [str(exe), '--csv', str(csv_path), '--summary', str(summary_path), '--repeat', str(args.repeat), '--warmup', str(args.warmup)]
    if args.quick:
        cmd.append('--quick')
    cmd.extend(args.files)
    print('running:', ' '.join(cmd))
    proc = subprocess.run(cmd, check=False)
    if proc.returncode != 0:
        return proc.returncode
    print('csv   :', csv_path)
    print('summary:', summary_path)
    return 0

if __name__ == '__main__':
    sys.exit(main())
