#!/usr/bin/env python3
import argparse
import hashlib
import json
import subprocess
import sys
from pathlib import Path


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open('rb') as f:
        while True:
            chunk = f.read(65536)
            if not chunk:
                break
            h.update(chunk)
    return h.hexdigest()


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
    ap = argparse.ArgumentParser(description='Run the phase 70 public benchmark suite, lock corpora by hash, and emit enhanced PNG/TIFF validation results.')
    ap.add_argument('--exe', default='build/zragf_bench_phase70_public')
    ap.add_argument('--out-dir', default='bench/results/phase70')
    ap.add_argument('--repeat', type=int, default=3)
    ap.add_argument('--warmup', type=int, default=1)
    ap.add_argument('--quick', action='store_true')
    ap.add_argument('--manifest', required=True)
    ap.add_argument('--corpus-root', default='bench/corpora')
    ap.add_argument('--archive-cache')
    ap.add_argument('--cache-only', action='store_true')
    ap.add_argument('--fetch', action='store_true')
    ap.add_argument('--no-builtins', action='store_true')
    args = ap.parse_args()

    exe = Path(args.exe)
    out_dir = Path(args.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    manifest_path = Path(args.manifest)
    lock_path = out_dir / 'phase70_corpus_lock.json'
    frozen_manifest_path = out_dir / 'phase70_frozen_manifest.json'
    csv_path = out_dir / 'phase70_public.csv'
    summary_path = out_dir / 'phase70_public.md'
    validation_json = out_dir / 'phase70_png_tiff_validation.json'
    metadata_json = out_dir / 'phase70_run_metadata.json'

    fetch_cmd = [
        sys.executable,
        'tools/fetch_phase70_corpora.py',
        '--manifest', str(manifest_path),
        '--root', args.corpus_root,
        '--lock-out', str(lock_path),
        '--freeze-manifest-out', str(frozen_manifest_path),
    ]
    if args.archive_cache:
        fetch_cmd.extend(['--archive-cache', args.archive_cache])
    if args.cache_only:
        fetch_cmd.append('--cache-only')
    if args.fetch:
        fetch_cmd.append('--force')
    print('locking:', ' '.join(fetch_cmd))
    proc = subprocess.run(fetch_cmd, check=False)
    if proc.returncode != 0:
        return proc.returncode

    manifest = json.loads(manifest_path.read_text(encoding='utf-8'))
    manifest_files = [str(p) for p in collect_files(manifest, Path(args.corpus_root)) if p.exists()]

    bench_cmd = [
        str(exe),
        '--csv', str(csv_path),
        '--summary', str(summary_path),
        '--repeat', str(args.repeat),
        '--warmup', str(args.warmup),
    ]
    if args.quick:
        bench_cmd.append('--quick')
    if args.no_builtins:
        bench_cmd.append('--no-builtins')
    bench_cmd.extend(manifest_files)
    print('running:', ' '.join(bench_cmd))
    proc = subprocess.run(bench_cmd, check=False)
    if proc.returncode != 0:
        return proc.returncode

    validate_cmd = [
        sys.executable,
        'tools/validate_phase70_png_tiff.py',
        '--manifest', str(manifest_path),
        '--root', args.corpus_root,
        '--json-out', str(validation_json),
    ]
    print('validating:', ' '.join(validate_cmd))
    proc = subprocess.run(validate_cmd, check=False)
    if proc.returncode != 0:
        return proc.returncode

    metadata = {
        'phase': 70,
        'manifest': str(manifest_path),
        'corpus_root': args.corpus_root,
        'archive_cache': args.archive_cache,
        'lockfile': str(lock_path),
        'frozen_manifest': str(frozen_manifest_path),
        'csv': str(csv_path),
        'summary': str(summary_path),
        'validation_json': str(validation_json),
        'repeat': args.repeat,
        'warmup': args.warmup,
        'quick': bool(args.quick),
        'hashes': {},
    }
    for p in [lock_path, frozen_manifest_path, csv_path, summary_path, validation_json]:
        metadata['hashes'][p.name] = sha256_file(p)
    metadata_json.write_text(json.dumps(metadata, indent=2, sort_keys=False) + '\n', encoding='utf-8')
    print('metadata:', metadata_json)
    return 0


if __name__ == '__main__':
    sys.exit(main())
