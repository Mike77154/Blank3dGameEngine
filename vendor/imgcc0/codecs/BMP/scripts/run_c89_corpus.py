#!/usr/bin/env python3
import argparse
import json
import pathlib
import subprocess


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument('--runner', default='tests/fuzz_bmp_c89_file')
    ap.add_argument('--corpus', default='tests/fuzz_corpus')
    ap.add_argument('--json-out', default='tests/fuzz_reports/c89_corpus.json')
    args = ap.parse_args()
    root = pathlib.Path(__file__).resolve().parent.parent
    runner = (root / args.runner).resolve()
    corpus = (root / args.corpus).resolve()
    results = []
    for path in sorted(p for p in corpus.iterdir() if p.is_file() and p.name != 'README.txt'):
        proc = subprocess.run([str(runner), str(path)], check=False, capture_output=True, text=True)
        results.append({'case': path.name, 'returncode': proc.returncode})
    passed = sum(1 for x in results if x['returncode'] == 0)
    payload = {'passed': passed, 'total': len(results), 'results': results}
    out = root / args.json_out
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(payload, indent=2) + '\n')
    print(f'C89 corpus: {passed}/{len(results)} runner exits clean')
    return 0 if passed == len(results) else 1


if __name__ == '__main__':
    raise SystemExit(main())
