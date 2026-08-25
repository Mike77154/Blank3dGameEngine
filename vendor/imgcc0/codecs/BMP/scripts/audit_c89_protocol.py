#!/usr/bin/env python3
import argparse
import json
import pathlib
import re

BANNED = {
    'dynamic-allocation-call': re.compile(r'\b(?:malloc|calloc|realloc|free)\s*\('),
    'wide-size-type': re.compile(r'\bsize_t\b'),
    'floating-type': re.compile(r'\b(?:float|double)\b'),
    'native-wide-type': re.compile(r'\b(?:long|int64_t|uint64_t|bmp_u64|bmp_s64)\b'),
    'wide-integer-suffix': re.compile(r'\b[0-9A-Fa-fx]+(?:ULL|LLU|LL|UL)\b'),
}


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument('--repo-root', default='.')
    ap.add_argument('--json-out', default='verification/c89_protocol_audit.json')
    args = ap.parse_args()
    root = pathlib.Path(args.repo_root).resolve()
    findings = []
    for path in sorted(root.rglob('*')):
        if path.suffix not in {'.c', '.h'} or not path.is_file():
            continue
        rel = path.relative_to(root).as_posix()
        text = path.read_text(encoding='utf-8', errors='ignore')
        for lineno, line in enumerate(text.splitlines(), 1):
            for rule, rx in BANNED.items():
                if rx.search(line):
                    findings.append({'file': rel, 'line': lineno, 'rule': rule, 'text': line.strip()})
    payload = {'ok': not findings, 'files_scanned': sum(1 for p in root.rglob('*') if p.is_file() and p.suffix in {'.c', '.h'}), 'findings': findings}
    out = root / args.json_out
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(payload, indent=2) + '\n')
    print(f"C89 protocol audit: {'PASS' if payload['ok'] else 'FAIL'} ({payload['files_scanned']} C/header files)")
    if findings:
        for item in findings[:20]:
            print(f"{item['file']}:{item['line']}: {item['rule']}: {item['text']}")
    return 0 if payload['ok'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
