#!/usr/bin/env python3
import argparse, json, re
from pathlib import Path

REQ_EXPORT = [
    '{P}_SAL_IN_READS_BYTES',
    '{P}_SAL_OUT_WRITES_BYTES',
    '{P}_ATTR_ACCESS_RO_2',
    '{P}_ATTR_ACCESS_WO_2',
    '{P}_LIFETIMEBOUND',
]
REQ_PUBLIC = [
    r"bmp_stream_init\(bmp_stream \*s,\s*BMP_SAL_IN_READS_BYTES\(size\) const bmp_u8 \*data,\s*bmp_u32 size\) BMP_ATTR_NONNULL_2\(1, 2\) BMP_ATTR_ACCESS_RO_2\(2, 3\);",
    r"bmp_parse_memory\(BMP_SAL_IN_READS_BYTES\(size\) const bmp_u8 \*data, bmp_u32 size, bmp_image \*out_img\) BMP_ATTR_NONNULL_2\(1, 3\) BMP_ATTR_ACCESS_RO_2\(1, 2\);",
    r"bmp_format_diagnostics_text\(const bmp_image \*img,\s*BMP_SAL_OUT_WRITES_BYTES\(buffer_size\) char \*buffer,\s*bmp_u32 buffer_size\) BMP_ATTR_NONNULL_1\(1\) BMP_ATTR_ACCESS_WO_2\(2, 3\);",
    r"bmp_stream_peek\(bmp_stream \*s BMP_LIFETIMEBOUND, bmp_u32 count\) BMP_ATTR_NONNULL_1\(1\);",
]
REQ_MAKE = ['buffer-contract-smoke', 'buffer-contracts-audit', 'test-round16']
REQ_WORKFLOW = ['buffer-contract-smoke', 'buffer-contracts-audit']

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--repo-root', required=True)
    ap.add_argument('--json-out', required=True)
    ap.add_argument('--md-out', required=True)
    args = ap.parse_args()
    root = Path(args.repo_root)
    export_h = (root/'include/bmp/bmp_export.h').read_text()
    public_h = (root/'include/bmp/bmp.h').read_text()
    make = (root/'Makefile').read_text()
    wf = (root/'.github/workflows/ci.yml').read_text()
    missing = []
    missing += [m.format(P='BMP') for m in REQ_EXPORT if m.format(P='BMP') not in export_h]
    missing += [p for p in REQ_PUBLIC if re.search(p, public_h, re.S) is None]
    missing += [m for m in REQ_MAKE if m not in make]
    missing += [w for w in REQ_WORKFLOW if w not in wf]
    payload = {'ok': not missing, 'missing': missing}
    Path(args.json_out).parent.mkdir(parents=True, exist_ok=True)
    Path(args.json_out).write_text(json.dumps(payload, indent=2)+'\n')
    md = ['# Buffer contract audit', '', f'- ok: {payload["ok"]}']
    if missing:
        md.append('- missing:')
        md.extend([f'  - `{m}`' for m in missing])
    else:
        md.append('- buffer access macros, public annotations, Makefile targets, and CI hooks are present.')
    Path(args.md_out).write_text('\n'.join(md)+'\n')
    return 0 if not missing else 1

if __name__ == '__main__':
    raise SystemExit(main())
