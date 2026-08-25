#!/usr/bin/env python3
import argparse
import json
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
COMPARE = ROOT / 'tests' / 'conformance' / 'compare_planes.py'


def run(cmd, cwd=ROOT):
    return subprocess.run(cmd, cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)


def read_pam_dims(path: Path):
    width = height = None
    for line in path.read_text('latin1').splitlines():
        if line.startswith('WIDTH '):
            width = int(line.split()[1])
        elif line.startswith('HEIGHT '):
            height = int(line.split()[1])
        elif line == 'ENDHDR':
            break
    if width is None or height is None:
        raise RuntimeError(f'no pude leer dimensiones de {path}')
    return width, height


def ensure_tool(name: str, provided: str) -> str:
    if not provided:
        raise SystemExit(f'falta ruta para {name}')
    resolved = shutil.which(provided) or provided
    if not Path(resolved).exists():
        raise SystemExit(f'no encontré {name}: {provided}')
    return resolved


def main() -> None:
    ap = argparse.ArgumentParser(description='Compara el encoder still propio contra cwebp + dwebp.')
    ap.add_argument('--inputs', nargs='+', help='archivos PAM RGBA de entrada')
    ap.add_argument('--encoder', default=str(ROOT / 'examples' / 'gwpencode'))
    ap.add_argument('--decoder', default=str(ROOT / 'examples' / 'gwpdecode'))
    ap.add_argument('--cwebp', default='cwebp')
    ap.add_argument('--dwebp', default='dwebp')
    ap.add_argument('--quality', default='75')
    ap.add_argument('--method', default='4')
    ap.add_argument('--preset', default='default')
    ap.add_argument('--alpha-q', default='100')
    ap.add_argument('--filter-strength', default='35')
    ap.add_argument('--mode', choices=['lossless', 'lossy'], default='lossless')
    ap.add_argument('--sharp-yuv', action='store_true')
    ap.add_argument('--out-dir', required=False)
    args = ap.parse_args()

    if not args.inputs:
        ap.print_help()
        return

    cwebp = ensure_tool('cwebp', args.cwebp)
    dwebp = ensure_tool('dwebp', args.dwebp)

    base_out = Path(args.out_dir) if args.out_dir else None
    if base_out:
        base_out.mkdir(parents=True, exist_ok=True)

    summary = []
    for src_name in args.inputs:
        src = Path(src_name)
        with tempfile.TemporaryDirectory() as tmpdir:
            tmp = Path(tmpdir)
            ours = tmp / 'ours.webp'
            ref = tmp / 'ref.webp'
            ours_pam = tmp / 'ours.pam'
            ref_pam = tmp / 'ref.pam'

            own_cmd = [args.encoder, '--method', args.method, '--preset', args.preset,
                       '--alpha-q', args.alpha_q, '--filter-strength', args.filter_strength]
            ref_cmd = [cwebp, '-preset', args.preset, '-q', args.quality, '-m', args.method,
                       '-alpha_q', args.alpha_q, '-f', args.filter_strength]
            if args.sharp_yuv:
                own_cmd.append('--sharp-yuv')
                ref_cmd.append('-sharp_yuv')
            if args.mode == 'lossless':
                own_cmd += ['--near-lossless', '80']
                ref_cmd += ['-lossless', '-exact']
            else:
                own_cmd += ['--lossy', '--allow-tools', '--cwebp', cwebp, '--quality', args.quality]
            own_cmd += [str(src), str(ours)]
            ref_cmd += [str(src), '-o', str(ref)]

            proc = run(own_cmd)
            if proc.returncode != 0:
                raise SystemExit(proc.stderr.strip() or proc.stdout.strip() or 'encoder propio falló')
            proc = run(ref_cmd)
            if proc.returncode != 0:
                raise SystemExit(proc.stderr.strip() or 'cwebp falló')
            run([args.decoder, str(ours), str(ours_pam)])
            proc = run([dwebp, str(ref), '-quiet', '-pam', '-o', str(ref_pam)])
            if proc.returncode != 0:
                raise SystemExit(proc.stderr.strip() or 'dwebp falló')
            width, height = read_pam_dims(src)
            cmp_proc = run([sys.executable, str(COMPARE), '--format', 'pam', str(ours_pam), str(ref_pam), '--mean-threshold', '8.0', '--max-threshold', '64'])
            if cmp_proc.returncode != 0:
                raise SystemExit(cmp_proc.stderr.strip() or cmp_proc.stdout.strip() or 'compare falló')
            record = json.loads(cmp_proc.stdout)
            record['input'] = str(src)
            record['width'] = width
            record['height'] = height
            record['our_size'] = ours.stat().st_size
            record['ref_size'] = ref.stat().st_size
            record['mode'] = args.mode
            summary.append(record)
            if base_out:
                (base_out / (src.stem + '.json')).write_text(json.dumps(record, indent=2, sort_keys=True) + '\n', encoding='utf-8')
    print(json.dumps(summary, indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
