#!/usr/bin/env python3
import argparse
import json
from pathlib import Path


def gather_headers(path: Path):
    if not path.exists():
        return []
    return sorted(str(p.relative_to(path)) for p in path.rglob('*.h'))


def scan_includes(path: Path):
    includes = []
    for line in path.read_text(encoding='utf-8').splitlines():
        line = line.strip()
        if line.startswith('#include'):
            includes.append(line)
    return includes


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--repo-root', required=True)
    ap.add_argument('--project', required=True)
    ap.add_argument('--stage-prefix', required=True)
    ap.add_argument('--json-out', required=True)
    ap.add_argument('--md-out', required=True)
    args = ap.parse_args()

    repo = Path(args.repo_root).resolve()
    project = args.project
    stage = Path(args.stage_prefix).resolve()
    public_header = repo / 'include' / project / f'{project}.h'
    stage_header_dir = stage / 'include' / project
    example_roots = [repo / 'examples', repo / 'test_package']

    findings = []
    ok = True

    if not public_header.exists():
        ok = False
        findings.append(f'missing public header: {public_header}')

    includes = scan_includes(public_header) if public_header.exists() else []
    allowed_local_include = f'#include "{project}_export.h"'
    quoted_includes = [inc for inc in includes if '"' in inc and inc != allowed_local_include]
    if quoted_includes:
        ok = False
        findings.append('public header contains unexpected quoted includes: ' + ', '.join(quoted_includes))

    installed_headers = gather_headers(stage_header_dir)
    expected_headers = sorted([f'{project}.h', f'{project}_export.h'])
    if installed_headers != expected_headers:
        ok = False
        findings.append('installed header set mismatch: expected ' + str(expected_headers) + ' got ' + str(installed_headers))

    bad_example_includes = []
    for root in example_roots:
        if not root.exists():
            continue
        for path in root.rglob('*'):
            if path.suffix not in {'.c', '.h', '.txt', '.md', '.cmake'}:
                continue
            text = path.read_text(encoding='utf-8', errors='ignore')
            for line in text.splitlines():
                s = line.strip()
                if not s.startswith('#include'):
                    continue
                if project in s and f'<{project}/{project}.h>' not in s:
                    bad_example_includes.append(f'{path.relative_to(repo)}: {s}')
    if bad_example_includes:
        ok = False
        findings.extend(['non-public include in consumer sample: ' + item for item in bad_example_includes])

    report = {
        'project': project,
        'ok': ok,
        'public_header': str(public_header.relative_to(repo)) if public_header.exists() else None,
        'installed_headers': installed_headers,
        'findings': findings,
    }
    Path(args.json_out).parent.mkdir(parents=True, exist_ok=True)
    Path(args.json_out).write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')

    result_text = 'PASS' if ok else 'FAIL'
    md = [
        f'# API surface audit for {project}',
        '',
        f'- result: {result_text}',
        f"- public header: `{report['public_header']}`",
        f'- installed headers: `{installed_headers}`',
        ''
    ]
    if findings:
        md.append('## Findings')
        for item in findings:
            md.append(f'- {item}')
    else:
        md.append('No issues found.')
    Path(args.md_out).parent.mkdir(parents=True, exist_ok=True)
    Path(args.md_out).write_text('\n'.join(md) + '\n', encoding='utf-8')

    raise SystemExit(0 if ok else 1)


if __name__ == '__main__':
    main()
