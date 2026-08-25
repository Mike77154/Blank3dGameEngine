#!/usr/bin/env python3
import argparse
import json
import re
from pathlib import Path

REQUIRED_EXPORT_MACROS = [
    "BMP_WARN_UNUSED_RESULT", "BMP_RETURNS_NONNULL",
    "BMP_ATTR_NONNULL_1", "BMP_ATTR_NONNULL_2",
    "BMP_ATTR_NONNULL_3", "BMP_ATTR_NONNULL_4", "BMP_ATTR_NONNULL_5", "BMP_ATTR_NONNULL_6",
    "BMP_ATTR_ACCESS_RO_2", "BMP_ATTR_ACCESS_WO_2", "BMP_ATTR_ACCESS_RW_2",
    "BMP_SAL_IN_READS_BYTES", "BMP_SAL_OUT_WRITES_BYTES",
]
REQUIRED_SNIPPETS = [
    r"bmp_decode_to_rgba32\(.*out_rgba,.*bmp_u32 out_stride\).*BMP_ATTR_ACCESS_WO_1\(2\)",
    r"bmp_encode_rgba32_into\(.*out_capacity.*workspace_size\).*BMP_ATTR_ACCESS_WO_2\(6, 7\).*BMP_ATTR_ACCESS_RW_2\(9, 10\)",
    r"bmp_encode_indexed_into\(.*out_capacity.*workspace_size\).*BMP_ATTR_ACCESS_WO_2\(8, 9\).*BMP_ATTR_ACCESS_RW_2\(11, 12\)",
    r"bmp_copy_embedded_payload_into\(.*out_capacity.*out_size\).*BMP_ATTR_ACCESS_WO_2\(3, 4\)",
    r"bmp_version_string\(void\) BMP_RETURNS_NONNULL",
    r"bmp_error_description\(int code\) BMP_RETURNS_NONNULL",
]


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument('--repo-root', required=True)
    ap.add_argument('--json-out', required=True)
    ap.add_argument('--md-out', required=True)
    args = ap.parse_args()
    root = Path(args.repo_root)
    export_h = (root / 'include/bmp/bmp_export.h').read_text()
    public_h = (root / 'include/bmp/bmp.h').read_text()
    missing = [m for m in REQUIRED_EXPORT_MACROS if m not in export_h]
    missing += [pat for pat in REQUIRED_SNIPPETS if re.search(pat, public_h, re.S) is None]
    payload = {'ok': not missing, 'missing': missing, 'model': 'caller-owned/static-buffer contracts'}
    Path(args.json_out).parent.mkdir(parents=True, exist_ok=True)
    Path(args.json_out).write_text(json.dumps(payload, indent=2) + '\n')
    md = ['# Contract annotation audit', '', f'- ok: {payload["ok"]}', '- ownership model: caller-owned/static buffers']
    if missing:
        md.append('- missing:')
        md.extend([f'  - `{m}`' for m in missing])
    else:
        md.append('- required non-owning buffer and result contracts were found.')
    Path(args.md_out).write_text('\n'.join(md) + '\n')
    return 0 if not missing else 1


if __name__ == '__main__':
    raise SystemExit(main())
