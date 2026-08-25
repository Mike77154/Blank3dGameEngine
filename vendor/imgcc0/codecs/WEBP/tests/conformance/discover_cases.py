#!/usr/bin/env python3
import argparse
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def probe(path: Path):
    proc = subprocess.run([str(ROOT / 'examples' / 'gwpinfo'), str(path)], cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if proc.returncode != 0:
        return None
    info = {'format': 'UNKNOWN', 'animation': 0, 'alpha': 0}
    for line in proc.stdout.splitlines():
        if line.startswith('format: '):
            info['format'] = line.split(':', 1)[1].strip()
        elif line.startswith('animation: '):
            info['animation'] = int(line.split(':', 1)[1].strip())
        elif line.startswith('alpha: '):
            info['alpha'] = int(line.split(':', 1)[1].strip())
    return info


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--corpus-dir', required=True)
    ap.add_argument('--out-dir', required=True)
    args = ap.parse_args()

    corpus = Path(args.corpus_dir)
    out_dir = Path(args.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    animated = []
    still_lossy = []
    still_lossless = []
    for path in sorted(corpus.rglob('*.webp')):
        info = probe(path)
        if info is None:
            continue
        rel = path.relative_to(corpus).as_posix()
        tags = []
        if info['alpha']:
            tags.append('alpha')
        if info['animation']:
            tags.append('animated')
            animated.append((rel, ','.join(tags) or 'animated'))
        elif info['format'] == 'VP8':
            still_lossy.append((rel, 'pam', 'lossy,' + (','.join(tags) if tags else 'still')))
        elif info['format'] == 'VP8L':
            still_lossless.append((rel, 'pam', 'lossless,' + (','.join(tags) if tags else 'still')))
    (out_dir / 'animated_pam.txt').write_text(''.join(f'{rel}\t{tags}\n' for rel, tags in animated), encoding='utf-8')
    (out_dir / 'still_lossy_pam.txt').write_text(''.join(f'{rel}\t{mode}\t{tags}\n' for rel, mode, tags in still_lossy), encoding='utf-8')
    (out_dir / 'still_lossless_pam.txt').write_text(''.join(f'{rel}\t{mode}\t{tags}\n' for rel, mode, tags in still_lossless), encoding='utf-8')


if __name__ == '__main__':
    main()
