#!/usr/bin/env python3
import argparse
import shutil
import sys
import urllib.request
from pathlib import Path

FILES = {
    'miniz.c': [
        'https://raw.githubusercontent.com/richgel999/miniz/3.1.0/miniz.c',
        'https://raw.githubusercontent.com/richgel999/miniz/master/miniz.c',
    ],
    'miniz.h': [
        'https://raw.githubusercontent.com/richgel999/miniz/3.1.0/miniz.h',
        'https://raw.githubusercontent.com/richgel999/miniz/master/miniz.h',
    ],
}


def download_first(urls, dst: Path, force: bool) -> str:
    dst.parent.mkdir(parents=True, exist_ok=True)
    if dst.exists() and not force:
        return 'cached'
    last = None
    tmp = dst.with_suffix(dst.suffix + '.tmp')
    if tmp.exists():
        tmp.unlink()
    for url in urls:
        try:
            with urllib.request.urlopen(url) as r, tmp.open('wb') as w:
                shutil.copyfileobj(r, w)
            tmp.replace(dst)
            return url
        except Exception as exc:  # pragma: no cover - network dependent
            last = exc
    raise RuntimeError(f'failed to fetch {dst.name}: {last}')


def main() -> int:
    ap = argparse.ArgumentParser(description='Fetch a pinned miniz drop for the optional phase 68 benchmark comparator.')
    ap.add_argument('--dest', default='third_party/miniz')
    ap.add_argument('--force', action='store_true')
    args = ap.parse_args()
    dest = Path(args.dest)
    dest.mkdir(parents=True, exist_ok=True)
    for name, urls in FILES.items():
        used = download_first(urls, dest / name, args.force)
        print(f'{name}: {used}')
    return 0


if __name__ == '__main__':
    sys.exit(main())
