#!/usr/bin/env python3
import argparse
import json
import subprocess
import sys
from pathlib import Path


def collect_files(manifest: dict, root: Path):
    out = []
    for ds in manifest.get('datasets', []):
        kind = ds.get('kind')
        if kind == 'existing':
            base = root / ds.get('root', '')
            for item in ds.get('files', []):
                out.append(base / item['path'])
        elif kind == 'files':
            base = root / ds['id']
            for item in ds.get('files', []):
                out.append(base / item['output'])
        elif kind == 'archive':
            base = root / ds['id']
            for item in ds.get('members', []):
                out.append(base / item['output'])
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description='Run the phase 68 public benchmark suite and save CSV/summary outputs.')
    ap.add_argument('--exe', default='build/zragf_bench_phase68_public')
    ap.add_argument('--out-dir', default='bench/results/phase68')
    ap.add_argument('--repeat', type=int, default=3)
    ap.add_argument('--warmup', type=int, default=1)
    ap.add_argument('--quick', action='store_true')
    ap.add_argument('--manifest', help='Optional corpus manifest JSON')
    ap.add_argument('--corpus-root', default='bench/corpora/downloaded')
    ap.add_argument('--fetch', action='store_true', help='Fetch manifest corpora before benchmarking')
    ap.add_argument('--no-builtins', action='store_true')
    ap.add_argument('--list-only', action='store_true')
    ap.add_argument('files', nargs='*')
    args = ap.parse_args()

    exe = Path(args.exe)
    out_dir = Path(args.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    csv_path = out_dir / 'phase68_public.csv'
    summary_path = out_dir / 'phase68_public.md'

    manifest_files = []
    if args.manifest:
        manifest_path = Path(args.manifest)
        if args.fetch:
            fetch_cmd = [sys.executable, 'tools/fetch_phase68_corpora.py', '--manifest', str(manifest_path), '--root', args.corpus_root]
            print('fetching:', ' '.join(fetch_cmd))
            proc = subprocess.run(fetch_cmd, check=False)
            if proc.returncode != 0:
                return proc.returncode
        manifest = json.loads(manifest_path.read_text(encoding='utf-8'))
        manifest_files = [str(p) for p in collect_files(manifest, Path(args.corpus_root)) if p.exists()]
        if args.list_only:
            for p in manifest_files:
                print(p)
            return 0

    cmd = [str(exe), '--csv', str(csv_path), '--summary', str(summary_path), '--repeat', str(args.repeat), '--warmup', str(args.warmup)]
    if args.quick:
        cmd.append('--quick')
    if args.no_builtins:
        cmd.append('--no-builtins')
    cmd.extend(manifest_files)
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
