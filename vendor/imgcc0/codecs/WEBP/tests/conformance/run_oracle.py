#!/usr/bin/env python3
import argparse
import json
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Dict, Iterable, List, Tuple

ROOT = Path(__file__).resolve().parents[2]
COMPARE = ROOT / 'tests' / 'conformance' / 'compare_planes.py'


def read_manifest(path: Path) -> List[Dict[str, str]]:
    items = []
    for raw in path.read_text().splitlines():
        line = raw.strip()
        if not line or line.startswith('#'):
            continue
        parts = line.split('\t')
        if len(parts) < 2:
            raise SystemExit(f'manifest mal formado: {path}: {raw!r}')
        tags = parts[2] if len(parts) > 2 else ''
        items.append({'relpath': parts[0], 'mode': parts[1], 'tags': tags, 'manifest': str(path)})
    return items


def run(cmd: List[str], cwd: Path = ROOT) -> subprocess.CompletedProcess:
    return subprocess.run(cmd, cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)


def write_pam_from_pillow(in_file: Path, out_file: Path) -> None:
    from PIL import Image
    img = Image.open(in_file).convert('RGBA')
    w, h = img.size
    rgba = img.tobytes()
    with out_file.open('wb') as f:
        f.write(f'P7\nWIDTH {w}\nHEIGHT {h}\nDEPTH 4\nMAXVAL 255\nTUPLTYPE RGB_ALPHA\nENDHDR\n'.encode('ascii'))
        f.write(rgba)


def get_features(webp_path: Path) -> Tuple[int, int, str]:
    proc = run([str(ROOT / 'examples' / 'gwpinfo'), str(webp_path)])
    if proc.returncode != 0:
        raise RuntimeError(proc.stderr.strip() or 'gwpinfo falló')
    width = height = 0
    fmt = 'UNKNOWN'
    for line in proc.stdout.splitlines():
        if line.startswith('format: '):
            fmt = line.split(':', 1)[1].strip()
        elif line.startswith('size: '):
            part = line.split(':', 1)[1].strip()
            w_s, h_s = [p.strip() for p in part.split('x')]
            width, height = int(w_s), int(h_s)
    return width, height, fmt


def compare(mode: str, lhs: Path, rhs: Path, width: int, height: int, mean_threshold: float, max_threshold: int) -> Dict:
    cmd = [sys.executable, str(COMPARE), '--format', mode, str(lhs), str(rhs), '--mean-threshold', str(mean_threshold), '--max-threshold', str(max_threshold)]
    if mode == 'yuv420p':
        cmd.extend(['--width', str(width), '--height', str(height)])
    proc = run(cmd)
    if proc.returncode != 0:
        raise RuntimeError(proc.stderr.strip() or proc.stdout.strip() or 'compare falló')
    return json.loads(proc.stdout)


def ensure_tool(path: str) -> str:
    resolved = shutil.which(path) or path
    if not Path(resolved).exists():
        raise SystemExit(f'no encontré el ejecutable: {path}')
    return resolved


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument('--oracle', choices=['dwebp', 'pillow'], default='dwebp')
    ap.add_argument('--dwebp', default='dwebp')
    ap.add_argument('--decoder-pam', default=str(ROOT / 'examples' / 'gwpdecode'))
    ap.add_argument('--decoder-yuv', default=str(ROOT / 'examples' / 'gwpdumpyuv'))
    ap.add_argument('--corpus-dir', required=True)
    ap.add_argument('--manifest', action='append', required=True)
    ap.add_argument('--out-dir', required=True)
    ap.add_argument('--mean-threshold', type=float, default=1.50)
    ap.add_argument('--max-threshold', type=int, default=8)
    args = ap.parse_args()

    out_dir = Path(args.out_dir)
    corpus_dir = Path(args.corpus_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    case_dir = out_dir / 'cases'
    case_dir.mkdir(parents=True, exist_ok=True)
    if args.oracle == 'dwebp':
        dwebp = ensure_tool(args.dwebp)
    else:
        dwebp = ''

    items: List[Dict[str, str]] = []
    for manifest in args.manifest:
        items.extend(read_manifest(Path(manifest)))

    summary = {'PASS_EXACT': 0, 'PASS_VISUAL_BUT_NOT_EXACT': 0, 'FAIL': 0}
    results_path = out_dir / 'results.jsonl'
    with results_path.open('w', encoding='utf-8') as results_file:
        for item in items:
            rel = item['relpath']
            mode = item['mode']
            src = corpus_dir / rel
            width = height = 0
            ours = case_dir / (Path(rel).name + '.ours.' + ('pam' if mode == 'pam' else 'yuv'))
            ref = case_dir / (Path(rel).name + '.ref.' + ('pam' if mode == 'pam' else 'yuv'))
            probe = case_dir / (Path(rel).name + '.vp8probe.txt')
            record = {'case': rel, 'mode': mode, 'tags': item['tags'], 'manifest': item['manifest']}
            if not src.exists():
                record['status'] = 'FAIL'
                record['reason'] = 'missing-input'
            else:
                try:
                    width, height, fmt = get_features(src)
                    record['bitstream_format'] = fmt
                    if mode == 'pam':
                        proc = run([args.decoder_pam, str(src), str(ours)])
                    elif mode == 'yuv420p':
                        proc = run([args.decoder_yuv, str(src), str(ours)])
                    else:
                        raise RuntimeError(f'modo no soportado: {mode}')
                    record['decoder_returncode'] = proc.returncode
                    record['decoder_stderr'] = proc.stderr.strip()
                    if proc.returncode != 0:
                        record['status'] = 'FAIL'
                        record['reason'] = 'decoder-failed'
                    else:
                        if args.oracle == 'dwebp':
                            if mode == 'pam':
                                ref_proc = run([dwebp, str(src), '-quiet', '-pam', '-o', str(ref)])
                            else:
                                ref_proc = run([dwebp, str(src), '-quiet', '-yuv', '-o', str(ref)])
                            record['oracle_returncode'] = ref_proc.returncode
                            record['oracle_stderr'] = ref_proc.stderr.strip()
                            if ref_proc.returncode != 0:
                                record['status'] = 'FAIL'
                                record['reason'] = 'oracle-failed'
                            else:
                                cmp = compare(mode, ours, ref, width, height, args.mean_threshold, args.max_threshold)
                                record.update(cmp)
                        else:
                            if mode != 'pam':
                                record['status'] = 'FAIL'
                                record['reason'] = 'pillow-only-supports-pam'
                            else:
                                write_pam_from_pillow(src, ref)
                                cmp = compare('pam', ours, ref, width, height, args.mean_threshold, args.max_threshold)
                                record.update(cmp)
                    if fmt == 'VP8':
                        p = run([str(ROOT / 'examples' / 'gwpvp8probe'), str(src)])
                        probe.write_text((p.stdout or '') + ('\n' + p.stderr if p.stderr else ''), encoding='utf-8')
                        record['probe_file'] = str(probe)
                except Exception as exc:  # pragma: no cover - operational fallback
                    record['status'] = 'FAIL'
                    record['reason'] = f'exception:{exc}'
            summary[record['status']] = summary.get(record['status'], 0) + 1
            results_file.write(json.dumps(record, sort_keys=True) + '\n')
    (out_dir / 'summary.json').write_text(json.dumps(summary, indent=2, sort_keys=True) + '\n', encoding='utf-8')
    print(json.dumps(summary, indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
