#!/usr/bin/env python3
import argparse
import json
from pathlib import Path


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--repo-root', required=True)
    ap.add_argument('--project', required=True)
    ap.add_argument('--json-out', required=True)
    ap.add_argument('--md-out', required=True)
    args = ap.parse_args()

    root = Path(args.repo_root).resolve()
    project = args.project
    upper = project.upper()
    public_header = root / 'include' / project / f'{project}.h'
    export_header = root / 'include' / project / f'{project}_export.h'
    cmake = (root / 'CMakeLists.txt').read_text(encoding='utf-8', errors='ignore')
    makefile = (root / 'Makefile').read_text(encoding='utf-8', errors='ignore')
    findings = []
    ok = True

    if not export_header.exists():
        ok = False
        findings.append(f'missing export header: {export_header}')
    else:
        export_text = export_header.read_text(encoding='utf-8', errors='ignore')
        for token in [f'{upper}_EXPORT', f'{upper}_NO_EXPORT', f'{upper}_DEPRECATED', f'{upper}_DEPRECATED_EXPORT', f'{upper}_NO_DEPRECATED']:
            if token not in export_text:
                ok = False
                findings.append(f'missing token in export header: {token}')

    if public_header.exists():
        public_text = public_header.read_text(encoding='utf-8', errors='ignore')
        include_line = f'#include "{project}_export.h"'
        if include_line not in public_text:
            ok = False
            findings.append(f'public header missing export include: {include_line}')
        if f'{upper}_EXPORT' not in public_text:
            ok = False
            findings.append('public header contains no explicit export annotations')
    else:
        ok = False
        findings.append(f'missing public header: {public_header}')

    if 'C_VISIBILITY_PRESET hidden' not in cmake:
        ok = False
        findings.append('CMake target does not set C_VISIBILITY_PRESET hidden')
    if f'{upper}_BUILDING_LIBRARY' not in cmake:
        ok = False
        findings.append('CMake target does not define build-time export macro')
    if '-fvisibility=hidden' not in makefile:
        ok = False
        findings.append('Makefile does not enable hidden visibility for shared builds')
    if f'-D{upper}_BUILDING_LIBRARY' not in makefile:
        ok = False
        findings.append('Makefile does not define build-time export macro for shared builds')
    if f'-D{upper}_STATIC_DEFINE' not in makefile:
        ok = False
        findings.append('Makefile does not define static macro for direct-source tools/tests')

    report = {
        'project': project,
        'ok': ok,
        'public_header': str(public_header.relative_to(root)) if public_header.exists() else None,
        'export_header': str(export_header.relative_to(root)) if export_header.exists() else None,
        'findings': findings,
    }
    Path(args.json_out).parent.mkdir(parents=True, exist_ok=True)
    Path(args.json_out).write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    lines = [
        f'# Visibility audit for {project}',
        '',
        f"- result: {'PASS' if ok else 'FAIL'}",
        f"- public header: `{report['public_header']}`",
        f"- export header: `{report['export_header']}`",
        ''
    ]
    if findings:
        lines.append('## Findings')
        for item in findings:
            lines.append(f'- {item}')
    else:
        lines.append('No issues found.')
    Path(args.md_out).parent.mkdir(parents=True, exist_ok=True)
    Path(args.md_out).write_text('\n'.join(lines) + '\n', encoding='utf-8')
    raise SystemExit(0 if ok else 1)


if __name__ == '__main__':
    main()
