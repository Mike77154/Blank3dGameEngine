#!/usr/bin/env python3
import json
import subprocess
import sys
from pathlib import Path


def main() -> int:
    if len(sys.argv) != 2:
        print('usage: test_phase69_locked_corpora.py <bench_exe>', file=sys.stderr)
        return 2
    exe = Path(sys.argv[1]).resolve()
    root = Path(__file__).resolve().parents[1]
    out_dir = root / 'tests' / '_phase69_locked_out'
    if out_dir.exists():
        for p in sorted(out_dir.rglob('*'), reverse=True):
            if p.is_file() or p.is_symlink():
                p.unlink()
            elif p.is_dir():
                p.rmdir()
    out_dir.mkdir(parents=True, exist_ok=True)

    cmd = [
        sys.executable,
        str(root / 'tools' / 'run_phase69_public.py'),
        '--exe', str(exe),
        '--manifest', str(root / 'bench' / 'corpora' / 'phase69_local_locked_manifest.json'),
        '--corpus-root', str(root / 'bench' / 'corpora'),
        '--no-builtins',
        '--quick',
        '--out-dir', str(out_dir),
    ]
    proc = subprocess.run(cmd, cwd=str(root), check=False)
    if proc.returncode != 0:
        return proc.returncode

    lock_path = out_dir / 'phase69_corpus_lock.json'
    val_path = out_dir / 'phase69_png_tiff_validation.json'
    if not lock_path.exists() or not val_path.exists():
        print('missing outputs', file=sys.stderr)
        return 1

    lock = json.loads(lock_path.read_text(encoding='utf-8'))
    if len(lock.get('datasets', [])) != 1:
        print('unexpected dataset count', file=sys.stderr)
        return 1
    if len(lock['datasets'][0].get('files', [])) != 4:
        print('unexpected locked file count', file=sys.stderr)
        return 1

    validation = json.loads(val_path.read_text(encoding='utf-8'))
    if validation.get('failures') != 0:
        print('validation failures', file=sys.stderr)
        return 1
    if validation.get('count', 0) < 2:
        print('expected png+tiff validation entries', file=sys.stderr)
        return 1
    print('phase69 locked corpora ok')
    return 0


if __name__ == '__main__':
    sys.exit(main())
