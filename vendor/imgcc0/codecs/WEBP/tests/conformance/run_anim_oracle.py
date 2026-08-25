#!/usr/bin/env python3
import argparse
import json
import subprocess
import sys
from pathlib import Path
from PIL import Image, ImageSequence

ROOT = Path(__file__).resolve().parents[2]
COMPARE = ROOT / 'tests' / 'conformance' / 'compare_animation.py'


def read_manifest(path: Path):
    items = []
    for raw in path.read_text(encoding='utf-8').splitlines():
        line = raw.strip()
        if not line or line.startswith('#'):
            continue
        parts = line.split('\t')
        if len(parts) < 1:
            continue
        tags = parts[1] if len(parts) > 1 else ''
        items.append({'relpath': parts[0], 'tags': tags, 'manifest': str(path)})
    return items


def run(cmd, cwd=ROOT):
    return subprocess.run(cmd, cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)


def write_pam(path: Path, rgba: bytes, w: int, h: int):
    with path.open('wb') as f:
        f.write(f'P7\nWIDTH {w}\nHEIGHT {h}\nDEPTH 4\nMAXVAL 255\nTUPLTYPE RGB_ALPHA\nENDHDR\n'.encode('ascii'))
        f.write(rgba)


def dump_pillow(in_file: Path, out_prefix: Path):
    img = Image.open(in_file)
    ts = 0
    meta_path = Path(str(out_prefix) + '.jsonl')
    with meta_path.open('w', encoding='utf-8') as meta:
        for i, frame in enumerate(ImageSequence.Iterator(img)):
            rgba = frame.convert('RGBA')
            w, h = rgba.size
            write_pam(Path(f'{out_prefix}_{i:03d}.pam'), rgba.tobytes(), w, h)
            duration = int(frame.info.get('duration', img.info.get('duration', 0)))
            meta.write(json.dumps({'index': i, 'timestamp_ms': ts, 'duration_ms': duration}) + '\n')
            ts += duration
    return meta_path


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--decoder', default=str(ROOT / 'examples' / 'gwpanimdump'))
    ap.add_argument('--corpus-dir', required=True)
    ap.add_argument('--manifest', action='append', required=True)
    ap.add_argument('--out-dir', required=True)
    ap.add_argument('--mean-threshold', type=float, default=0.0)
    ap.add_argument('--max-threshold', type=int, default=0)
    args = ap.parse_args()

    items = []
    for manifest in args.manifest:
        items.extend(read_manifest(Path(manifest)))
    out_dir = Path(args.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    case_dir = out_dir / 'cases'
    case_dir.mkdir(parents=True, exist_ok=True)
    results_path = out_dir / 'results.jsonl'
    summary = {'PASS_EXACT': 0, 'PASS_VISUAL_BUT_NOT_EXACT': 0, 'FAIL': 0}

    with results_path.open('w', encoding='utf-8') as results:
        for item in items:
            src = Path(args.corpus_dir) / item['relpath']
            rec = {'case': item['relpath'], 'tags': item['tags'], 'manifest': item['manifest']}
            ours_prefix = case_dir / (src.stem + '.ours')
            ref_prefix = case_dir / (src.stem + '.ref')
            try:
                dec = run([args.decoder, str(src), str(ours_prefix)])
                rec['decoder_returncode'] = dec.returncode
                rec['decoder_stderr'] = dec.stderr.strip()
                if dec.returncode != 0:
                    rec['status'] = 'FAIL'
                    rec['reason'] = 'decoder-failed'
                else:
                    ours_meta = Path(str(ours_prefix) + '.jsonl')
                    ref_meta = dump_pillow(src, ref_prefix)
                    cmp = run([
                        sys.executable,
                        str(COMPARE),
                        str(ours_meta),
                        str(ref_meta),
                        '--lhs-prefix', str(ours_prefix),
                        '--rhs-prefix', str(ref_prefix),
                        '--mean-threshold', str(args.mean_threshold),
                        '--max-threshold', str(args.max_threshold),
                    ])
                    if cmp.returncode != 0:
                        rec['status'] = 'FAIL'
                        rec['reason'] = cmp.stderr.strip() or cmp.stdout.strip() or 'compare-failed'
                    else:
                        rec.update(json.loads(cmp.stdout))
            except Exception as exc:
                rec['status'] = 'FAIL'
                rec['reason'] = f'exception:{exc}'
            summary[rec['status']] = summary.get(rec['status'], 0) + 1
            results.write(json.dumps(rec, sort_keys=True) + '\n')
    (out_dir / 'summary.json').write_text(json.dumps(summary, indent=2, sort_keys=True) + '\n', encoding='utf-8')
    print(json.dumps(summary, indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
