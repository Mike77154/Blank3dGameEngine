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


def ensure_tool(name: str, provided: str) -> str:
    resolved = shutil.which(provided) or provided
    if not Path(resolved).exists():
        raise SystemExit(f'no encontré {name}: {provided}')
    return resolved


def read_payload(path: Path) -> bytes:
    return path.read_bytes().split(b'ENDHDR\n', 1)[1]


def main() -> None:
    ap = argparse.ArgumentParser(description='Compara animación propia contra img2webp o gif2webp.')
    ap.add_argument('--frames', nargs='*', help='frames PAM RGBA para comparar contra img2webp')
    ap.add_argument('--gif', help='GIF opcional para probar gif2webp + demux/decode')
    ap.add_argument('--anim-encoder', default=str(ROOT / 'examples' / 'gwpanimframes'))
    ap.add_argument('--anim-dump', default=str(ROOT / 'examples' / 'gwpanimdump'))
    ap.add_argument('--still-encoder', default=str(ROOT / 'examples' / 'gwpencode'))
    ap.add_argument('--img2webp', default='img2webp')
    ap.add_argument('--gif2webp', default='gif2webp')
    ap.add_argument('--quality', default='75')
    ap.add_argument('--method', default='4')
    ap.add_argument('--preset', default='default')
    ap.add_argument('--kmin', default='0')
    ap.add_argument('--kmax', default='0')
    ap.add_argument('--mixed', action='store_true')
    ap.add_argument('--lossy', action='store_true')
    ap.add_argument('--min-size', action='store_true')
    args = ap.parse_args()

    if not args.frames and not args.gif:
        ap.print_help()
        return

    result = {}
    with tempfile.TemporaryDirectory() as tmpdir:
        tmp = Path(tmpdir)
        if args.frames:
            img2webp = ensure_tool('img2webp', args.img2webp)
            ours = tmp / 'ours_anim.webp'
            ref = tmp / 'ref_anim.webp'
            own_dump = tmp / 'ours_dump'
            ref_dump = tmp / 'ref_dump'
            own_cmd = [args.anim_encoder]
            if args.mixed:
                own_cmd.append('--mixed')
            if args.lossy:
                own_cmd.append('--lossy')
            if args.min_size:
                own_cmd.append('--min-size')
            else:
                own_cmd.append('--no-min-size')
            own_cmd += ['--quality', args.quality, '--method', args.method, '--preset', args.preset]
            if args.kmin != '0':
                own_cmd += ['--kmin', args.kmin]
            if args.kmax != '0':
                own_cmd += ['--kmax', args.kmax]
            own_cmd += [str(ours)] + list(args.frames)
            subprocess.run(own_cmd, cwd=ROOT, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            stills = []
            for i, frame in enumerate(args.frames):
                still = tmp / f'still_{i}.webp'
                still_cmd = [args.still_encoder, '--method', args.method, '--preset', args.preset]
                if args.lossy:
                    still_cmd += ['--lossy', '--allow-tools', '--cwebp', 'cwebp', '--quality', args.quality]
                still_cmd += [frame, str(still)]
                subprocess.run(still_cmd, cwd=ROOT, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                stills.extend(['-d', '100', str(still)])
            ref_cmd = [img2webp]
            if args.mixed:
                ref_cmd.append('-mixed')
            elif args.lossy:
                ref_cmd.append('-lossy')
            if args.min_size:
                ref_cmd.append('-min_size')
            ref_cmd += ['-q', args.quality, '-m', args.method]
            if args.kmin != '0':
                ref_cmd += ['-kmin', args.kmin]
            if args.kmax != '0':
                ref_cmd += ['-kmax', args.kmax]
            proc = run(ref_cmd + stills + ['-o', str(ref)])
            if proc.returncode != 0:
                raise SystemExit(proc.stderr.strip() or 'img2webp falló')
            subprocess.run([args.anim_dump, str(ours), str(own_dump)], cwd=ROOT, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            subprocess.run([args.anim_dump, str(ref), str(ref_dump)], cwd=ROOT, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            diffs = []
            for i, frame in enumerate(args.frames):
                ours_pam = tmp / f'ours_dump_{i:03d}.pam'
                ref_pam = tmp / f'ref_dump_{i:03d}.pam'
                if not ours_pam.exists() or not ref_pam.exists():
                    diffs.append({'frame': i, 'status': 'missing'})
                    continue
                proc = run([sys.executable, str(COMPARE), '--format', 'pam', str(ours_pam), str(ref_pam), '--mean-threshold', '8.0', '--max-threshold', '64'])
                if proc.returncode != 0:
                    raise SystemExit(proc.stderr.strip() or proc.stdout.strip() or 'compare falló')
                item = json.loads(proc.stdout)
                item['frame'] = i
                item['input_matches_ours'] = read_payload(Path(frame)) == read_payload(ours_pam)
                diffs.append(item)
            result['img2webp'] = {'our_size': ours.stat().st_size, 'ref_size': ref.stat().st_size, 'frames': diffs}
        if args.gif:
            gif2webp = ensure_tool('gif2webp', args.gif2webp)
            out = tmp / 'gif_tool.webp'
            ref_cmd = [gif2webp]
            if args.mixed:
                ref_cmd.append('-mixed')
            elif args.lossy:
                ref_cmd.append('-lossy')
            if args.min_size:
                ref_cmd.append('-min_size')
            ref_cmd += [args.gif, '-o', str(out)]
            proc = run(ref_cmd)
            if proc.returncode != 0:
                raise SystemExit(proc.stderr.strip() or 'gif2webp falló')
            subprocess.run([args.anim_dump, str(out), str(tmp / 'gifdump')], cwd=ROOT, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            result['gif2webp'] = {'size': out.stat().st_size, 'decoded': True}
    print(json.dumps(result, indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
