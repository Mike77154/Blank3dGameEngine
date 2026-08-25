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
    r"pcx_read_bytes\(FILE \*f,\s*PCX_SAL_OUT_WRITES_BYTES\(count\) void \*buffer,\s*pcx_size count\) PCX_ATTR_NONNULL_2\(1, 2\) PCX_ATTR_ACCESS_WO_2\(2, 3\);",
    r"pcx_load_memory\(PCX_SAL_IN_READS_BYTES\(size\) const void \*data, pcx_size size, PCXImage \*outImage\) PCX_ATTR_NONNULL_2\(1, 3\) PCX_ATTR_ACCESS_RO_2\(1, 2\);",
    r"pcx_format_diagnostics_text\(const PCXFileInfo \*info,\s*PCX_SAL_OUT_WRITES_BYTES\(bufferSize\) char \*buffer,\s*pcx_size bufferSize\) PCX_ATTR_NONNULL_1\(1\) PCX_ATTR_ACCESS_WO_2\(2, 3\);",
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
    export_h = (root/'include/pcx/pcx_export.h').read_text()
    public_h = (root/'include/pcx/pcx.h').read_text()
    make = (root/'Makefile').read_text()
    wf = (root/'.github/workflows/ci.yml').read_text()
    missing = []
    missing += [m.format(P='PCX') for m in REQ_EXPORT if m.format(P='PCX') not in export_h]
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
