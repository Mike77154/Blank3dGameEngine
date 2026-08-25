#!/usr/bin/env python3
import argparse
import json
from pathlib import Path

REQ = {
    "make": [
        "-Wstrict-prototypes",
        "-Wmissing-prototypes",
        "-Wshadow",
        "-Wpointer-arith",
        "-Wformat=2",
        "-Wundef",
        "-Warray-bounds",
        "-Wnull-dereference",
        "-Wmismatched-dealloc",
        "-Wstringop-overread",
        "-Wstringop-overflow=2",
        "-Warray-bounds=2",
        "-Wrestrict",
    ],
    "cmake": [
        "/W4 /WX /permissive- /external:anglebrackets /external:W0",
        "-Wstrict-prototypes",
        "-Wmissing-prototypes",
        "-Wshadow",
        "-Wpointer-arith",
        "-Wformat=2",
        "-Wundef",
        "-Warray-bounds",
        "-Wnull-dereference",
        "-Wstringop-overread",
        "-Wstringop-overflow=2",
        "-Warray-bounds=2",
        "-Wrestrict",
        "/analyze",
    ],
}


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument('--repo-root', required=True)
    ap.add_argument('--project', required=True)
    ap.add_argument('--json-out', required=True)
    ap.add_argument('--md-out', required=True)
    args = ap.parse_args()

    root = Path(args.repo_root)
    make = (root / 'Makefile').read_text()
    cmake = (root / 'CMakeLists.txt').read_text()
    missing = {
        'make': [flag for flag in REQ['make'] if flag not in make],
        'cmake': [flag for flag in REQ['cmake'] if flag not in cmake],
    }
    ok = not missing['make'] and not missing['cmake']
    out = {'ok': ok, 'missing': missing, 'project': args.project}
    Path(args.json_out).parent.mkdir(parents=True, exist_ok=True)
    Path(args.json_out).write_text(json.dumps(out, indent=2) + '\n')

    md = ['# Warning policy audit', '', f'- project: `{args.project}`', f'- ok: {ok}']
    for scope in ('make', 'cmake'):
        if missing[scope]:
            md.append(f'- missing {scope} flags:')
            md.extend([f'  - `{x}`' for x in missing[scope]])
    Path(args.md_out).write_text('\n'.join(md) + '\n')
    return 0 if ok else 1


if __name__ == '__main__':
    raise SystemExit(main())
