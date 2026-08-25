#!/usr/bin/env python3
import argparse, json, re
from pathlib import Path
REQUIRED_EXPORT_MACROS = [
    "PCX_WARN_UNUSED_RESULT", "PCX_RETURNS_NONNULL",
    "PCX_ATTR_NONNULL_1", "PCX_ATTR_NONNULL_2",
    "PCX_ATTR_NONNULL_3", "PCX_ATTR_NONNULL_4", "PCX_ATTR_NONNULL_5",
    "PCX_ATTR_ACCESS_RO_2", "PCX_ATTR_ACCESS_WO_2",
    "PCX_SAL_IN_READS_BYTES", "PCX_SAL_OUT_WRITES_BYTES",
    "PCX_PURE_FN", "PCX_CONST_FN",
]
REQUIRED_SNIPPETS = [
    r"pcx_image_use_buffer\(PCXImage \*img,.*?pcx_u8 \*pixels\) PCX_ATTR_NONNULL_2\(1, 5\);",
    r"pcx_indexed_image_use_buffer\(PCXIndexedImage \*img,.*?pcx_u8 \*indices\) PCX_ATTR_NONNULL_2\(1, 5\);",
    r"PCX_EXPORT\s+const char \*pcx_version_string\(void\)\s+PCX_RETURNS_NONNULL(?:\s+PCX_[A-Z0-9_]+)*;",
    r"PCX_EXPORT\s+const char \*pcx_result_description\(PCXResult result\)\s+PCX_RETURNS_NONNULL(?:\s+PCX_[A-Z0-9_]+)*;",
]

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--repo-root', required=True)
    ap.add_argument('--json-out', required=True)
    ap.add_argument('--md-out', required=True)
    args = ap.parse_args()
    root = Path(args.repo_root)
    export_h = (root / 'include/pcx/pcx_export.h').read_text()
    public_h = (root / 'include/pcx/pcx.h').read_text()
    missing = [m for m in REQUIRED_EXPORT_MACROS if m not in export_h]
    missing += [pat for pat in REQUIRED_SNIPPETS if re.search(pat, public_h, re.S) is None]
    payload = {'ok': not missing, 'missing': missing}
    Path(args.json_out).parent.mkdir(parents=True, exist_ok=True)
    Path(args.json_out).write_text(json.dumps(payload, indent=2) + '\n')
    md = ['# Contract annotation audit', '', f'- ok: {payload["ok"]}']
    if missing:
        md.append('- missing:')
        md.extend([f'  - `{m}`' for m in missing])
    else:
        md.append('- all required macros and annotated declarations were found.')
    Path(args.md_out).write_text('\n'.join(md) + '\n')
    return 0 if not missing else 1
if __name__ == '__main__':
    raise SystemExit(main())
