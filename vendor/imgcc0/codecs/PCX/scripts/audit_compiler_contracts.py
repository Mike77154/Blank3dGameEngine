#!/usr/bin/env python3
import argparse, json, re
from pathlib import Path

PREFIX = "PCX"
PROJECT = "pcx"

REQUIRED_EXPORT_MACROS = [
    f"{PREFIX}_HAS_BUILTIN",
    f"{PREFIX}_HAS_DECLSPEC_ATTRIBUTE",
    f"{PREFIX}_ANALYZER_ACTIVE",
    f"{PREFIX}_PURE_FN",
    f"{PREFIX}_CONST_FN",
    f"{PREFIX}_COLD",
    f"{PREFIX}_NORETURN",
    f"{PREFIX}_NOINLINE",
    f"{PREFIX}_ATTR_ACCESS_RO_2",
    f"{PREFIX}_ATTR_ACCESS_WO_2",
    f"{PREFIX}_SAL_IN_READS_BYTES",
    f"{PREFIX}_SAL_OUT_WRITES_BYTES",
    f"{PREFIX}_LIFETIMEBOUND",
]

REQUIRED_PUBLIC_SNIPPETS = [
    r"pcx_warning_mask_has\(pcx_u32 mask, pcx_u32 flag\) PCX_CONST_FN;",
    r"pcx_version_string\(void\) PCX_RETURNS_NONNULL PCX_CONST_FN;",
    r"pcx_palette_is_valid\(const PCXPalette \*pal\) PCX_PURE_FN;",
    r"pcx_read_bytes\(FILE \*f,\s*PCX_SAL_OUT_WRITES_BYTES\(count\) void \*buffer,\s*pcx_size count\) PCX_ATTR_NONNULL_2\(1, 2\) PCX_ATTR_ACCESS_WO_2\(2, 3\);",
]

REQUIRED_MAKE_SNIPPETS = [
    'header-matrix-smoke',
    'analyzer-smoke',
    'compiler-contracts-audit',
    'buffer-contract-smoke',
    'buffer-contracts-audit',
]

REQUIRED_WORKFLOW_SNIPPETS = [
    'analysis-contracts',
    'header-matrix-smoke analyzer-smoke compiler-contracts-audit',
    'ANALYSIS_CONTRACTS_RESULT',
    'buffer-contracts-audit',
]

def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument('--repo-root', required=True)
    ap.add_argument('--json-out', required=True)
    ap.add_argument('--md-out', required=True)
    args = ap.parse_args()
    root = Path(args.repo_root)
    export_h = (root / 'include' / PROJECT / f'{PROJECT}_export.h').read_text()
    public_h = (root / 'include' / PROJECT / f'{PROJECT}.h').read_text()
    makefile = (root / 'Makefile').read_text()
    workflow = (root / '.github/workflows/ci.yml').read_text()

    missing = [m for m in REQUIRED_EXPORT_MACROS if m not in export_h]
    missing += [p for p in REQUIRED_PUBLIC_SNIPPETS if re.search(p, public_h, re.S) is None]
    missing += [s for s in REQUIRED_MAKE_SNIPPETS if s not in makefile]
    missing += [s for s in REQUIRED_WORKFLOW_SNIPPETS if s not in workflow]

    payload = {'ok': not missing, 'missing': missing}
    Path(args.json_out).parent.mkdir(parents=True, exist_ok=True)
    Path(args.json_out).write_text(json.dumps(payload, indent=2) + '\n')
    md = ['# Compiler-contract audit', '', f'- ok: {payload["ok"]}']
    if missing:
        md.append('- missing:')
        md.extend([f'  - `{m}`' for m in missing])
    else:
        md.append('- compiler-aware contracts, smoke targets, and CI hooks are present.')
    Path(args.md_out).write_text('\n'.join(md) + '\n')
    return 0 if not missing else 1

if __name__ == '__main__':
    raise SystemExit(main())
