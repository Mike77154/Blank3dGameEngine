import argparse
import json
import math
import shutil
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def run(cmd, cwd=ROOT):
    return subprocess.run(cmd, cwd=cwd, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)


def read_pam(path: Path):
    data = path.read_bytes()
    header, payload = data.split(b'ENDHDR\n', 1)
    width = height = None
    for line in header.decode('ascii').splitlines():
        if line.startswith('WIDTH '):
            width = int(line.split()[1])
        if line.startswith('HEIGHT '):
            height = int(line.split()[1])
    return width, height, payload


def mse(a: bytes, b: bytes) -> float:
    n = min(len(a), len(b))
    if n == 0:
        return 0.0
    acc = 0
    for i in range(n):
        d = a[i] - b[i]
        acc += d * d
    return acc / float(n)


def psnr(a: bytes, b: bytes) -> float:
    e = mse(a, b)
    if e <= 0.0:
        return 999.0
    return 10.0 * math.log10((255.0 * 255.0) / e)


def encode_local(pam: str, out_webp: Path, out_pam: Path, quality: str, legacy: bool):
    cmd = [str(ROOT / 'examples' / 'gwpencode'), '--lossy', '--quality', quality]
    if legacy:
        cmd.append('--legacy-native')
    cmd += [pam, str(out_webp)]
    run(cmd)
    run([str(ROOT / 'examples' / 'gwpdecode'), str(out_webp), str(out_pam)])


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument('pam')
    ap.add_argument('--cwebp', default='cwebp')
    ap.add_argument('--dwebp', default='dwebp')
    ap.add_argument('--quality', default='60')
    ap.add_argument('--require-official-tools', action='store_true')
    args = ap.parse_args()

    have_cwebp = shutil.which(args.cwebp) is not None
    have_dwebp = shutil.which(args.dwebp) is not None
    if args.require_official_tools and (not have_cwebp or not have_dwebp):
        raise SystemExit('cwebp/dwebp no disponibles en PATH')

    with tempfile.TemporaryDirectory() as tmpdir:
        tmp = Path(tmpdir)
        phase15_webp = tmp / 'phase15.webp'
        phase15_pam = tmp / 'phase15.pam'
        legacy_webp = tmp / 'legacy.webp'
        legacy_pam = tmp / 'legacy.pam'
        ref_webp = tmp / 'ref.webp'
        ref_pam = tmp / 'ref.pam'

        run(['make'])
        encode_local(args.pam, phase15_webp, phase15_pam, args.quality, legacy=False)
        encode_local(args.pam, legacy_webp, legacy_pam, args.quality, legacy=True)

        p15w, p15h, p15px = read_pam(phase15_pam)
        lgw, lgh, lgpx = read_pam(legacy_pam)
        out = {
            'phase15_size': phase15_webp.stat().st_size,
            'legacy_size': legacy_webp.stat().st_size,
            'width': p15w,
            'height': p15h,
            'phase15_vs_legacy_psnr': psnr(p15px, lgpx),
            'size_delta_vs_legacy': int(phase15_webp.stat().st_size - legacy_webp.stat().st_size),
            'have_cwebp': bool(have_cwebp),
            'have_dwebp': bool(have_dwebp),
        }

        if have_cwebp and have_dwebp:
            run([args.cwebp, '-q', args.quality, args.pam, '-o', str(ref_webp)])
            run([args.dwebp, str(ref_webp), '-pam', '-o', str(ref_pam)])
            rw, rh, rpx = read_pam(ref_pam)
            out.update({
                'cwebp_size': ref_webp.stat().st_size,
                'psnr_phase15_vs_cwebp_decode': psnr(p15px, rpx),
                'psnr_legacy_vs_cwebp_decode': psnr(lgpx, rpx),
                'official_width': rw,
                'official_height': rh,
            })
        print(json.dumps(out, indent=2))


if __name__ == '__main__':
    main()
