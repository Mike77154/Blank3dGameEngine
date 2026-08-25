#!/usr/bin/env python3
import argparse
import json
from collections import Counter, defaultdict
from pathlib import Path


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument('results_jsonl')
    args = ap.parse_args()

    rows = [json.loads(line) for line in Path(args.results_jsonl).read_text(encoding='utf-8').splitlines() if line.strip()]
    status_counter = Counter(row.get('status', 'UNKNOWN') for row in rows)
    reason_counter = Counter(row.get('reason', '') for row in rows if row.get('status') == 'FAIL')
    fail_by_tag = defaultdict(int)
    for row in rows:
        if row.get('status') == 'FAIL':
            for tag in row.get('tags', '').split(','):
                tag = tag.strip()
                if tag:
                    fail_by_tag[tag] += 1

    print('# Resumen')
    for status, count in sorted(status_counter.items()):
        print(f'- {status}: {count}')
    if reason_counter:
        print('\n# Razones de fallo')
        for reason, count in reason_counter.most_common():
            print(f'- {reason}: {count}')
    if fail_by_tag:
        print('\n# Fallos por tag')
        for tag, count in sorted(fail_by_tag.items(), key=lambda kv: (-kv[1], kv[0])):
            print(f'- {tag}: {count}')
    print('\n# Primeros desvíos')
    shown = 0
    for row in rows:
        if row.get('status') == 'PASS_EXACT':
            continue
        first = row.get('first_mismatch')
        if first:
            print(f"- {row['case']}: plane={first.get('plane')} offset={first.get('offset')} lhs={first.get('lhs')} rhs={first.get('rhs')}")
            shown += 1
        elif row.get('status') == 'FAIL':
            print(f"- {row['case']}: {row.get('reason', 'sin detalle')}")
            shown += 1
        if shown >= 20:
            break


if __name__ == '__main__':
    main()
