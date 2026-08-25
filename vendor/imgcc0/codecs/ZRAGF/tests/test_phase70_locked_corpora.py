#!/usr/bin/env python3
import json
import subprocess
import sys
from pathlib import Path


def rm_tree(path: Path) -> None:
    if not path.exists():
        return
    for p in sorted(path.rglob('*'), reverse=True):
        if p.is_file() or p.is_symlink():
            p.unlink()
        elif p.is_dir():
            p.rmdir()
    path.rmdir()


def main() -> int:
    if len(sys.argv) != 2:
        print('usage: test_phase70_locked_corpora.py <bench_exe>', file=sys.stderr)
        return 2
    exe = Path(sys.argv[1]).resolve()
    root = Path(__file__).resolve().parents[1]
    out_dir = root / 'tests' / '_phase70_locked_out'
    rm_tree(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    cmd = [
        sys.executable,
        str(root / 'tools' / 'run_phase70_public.py'),
        '--exe', str(exe),
        '--manifest', str(root / 'bench' / 'corpora' / 'phase70_local_locked_manifest.json'),
        '--corpus-root', str(root / 'bench' / 'corpora'),
        '--no-builtins',
        '--quick',
        '--out-dir', str(out_dir),
    ]
    proc = subprocess.run(cmd, cwd=str(root), check=False)
    if proc.returncode != 0:
        return proc.returncode

    lock_path = out_dir / 'phase70_corpus_lock.json'
    val_path = out_dir / 'phase70_png_tiff_validation.json'
    if not lock_path.exists() or not val_path.exists():
        print('missing outputs', file=sys.stderr)
        return 1

    lock = json.loads(lock_path.read_text(encoding='utf-8'))
    if len(lock.get('datasets', [])) != 1:
        print('unexpected dataset count', file=sys.stderr)
        return 1
    if len(lock['datasets'][0].get('files', [])) != 6:
        print('unexpected locked file count', file=sys.stderr)
        return 1

    validation = json.loads(val_path.read_text(encoding='utf-8'))
    if validation.get('failures') != 0:
        print('validation failures', file=sys.stderr)
        return 1
    if validation.get('count', 0) < 4:
        print('expected at least two png and two tiff validation entries', file=sys.stderr)
        return 1
    for entry in validation.get('results', []):
        if entry['suffix'] == '.png' and not entry['structural'].get('chunk_count'):
            print('missing png chunk parse', file=sys.stderr)
            return 1
        if entry['suffix'] in ('.tif', '.tiff') and not entry['structural'].get('variant'):
            print('missing tiff structural parse', file=sys.stderr)
            return 1
    print('phase70 locked corpora ok')
    return 0


if __name__ == '__main__':
    sys.exit(main())
