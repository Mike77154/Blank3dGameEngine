#!/usr/bin/env python3
import argparse
import pathlib
import subprocess
import sys

def load_allowlist(path):
    items = []
    for raw in pathlib.Path(path).read_text().splitlines():
        line = raw.split('#', 1)[0].strip()
        if line:
            items.append(line)
    return items

def dynamic_symbols(library):
    out = subprocess.check_output(['nm', '-D', '--defined-only', '--format=posix', library], text=True)
    names = []
    for line in out.splitlines():
        parts = line.split()
        if not parts:
            continue
        name = parts[0]
        name = name.split('@@', 1)[0].split('@', 1)[0]
        if name.startswith('_'):
            continue
        if name and name.upper() == name and any(ch.isdigit() for ch in name):
            continue
        names.append(name)
    return sorted(set(names))

def verify_soname(library, expected):
    out = subprocess.check_output(['readelf', '-d', library], text=True)
    needle = f'Library soname: [{expected}]'
    return needle in out

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--library', required=True)
    ap.add_argument('--allowlist', required=True)
    ap.add_argument('--expected-soname')
    args = ap.parse_args()

    exported = dynamic_symbols(args.library)
    expected = sorted(set(load_allowlist(args.allowlist)))

    missing = [name for name in expected if name not in exported]
    extra = [name for name in exported if name not in expected]

    ok = True
    if missing:
        ok = False
        print('Missing exported symbols:')
        for name in missing:
            print(f'  {name}')
    if extra:
        ok = False
        print('Unexpected exported symbols:')
        for name in extra:
            print(f'  {name}')
    if args.expected_soname and not verify_soname(args.library, args.expected_soname):
        ok = False
        print(f'SONAME mismatch: expected {args.expected_soname}')

    if ok:
        print(f'OK: export set matches allowlist for {args.library}')
        if args.expected_soname:
            print(f'OK: SONAME = {args.expected_soname}')
        return 0
    return 1

if __name__ == '__main__':
    raise SystemExit(main())
