import shutil
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def write_pam(path: Path, width: int, height: int, payload: bytes) -> None:
    path.write_bytes(
        f'P7\nWIDTH {width}\nHEIGHT {height}\nDEPTH 4\nMAXVAL 255\nTUPLTYPE RGB_ALPHA\nENDHDR\n'.encode('ascii') + payload
    )


def read_pam_payload(path: Path) -> bytes:
    data = path.read_bytes()
    return data.split(b'ENDHDR\n', 1)[1]


def run(cmd, **kwargs):
    return subprocess.run(cmd, cwd=ROOT, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, **kwargs)


def main() -> None:
    run(['make', 'clean'])
    run(['make'])

    with tempfile.TemporaryDirectory() as tmpdir:
        tmp = Path(tmpdir)

        # Better VP8L compression than method 0 on repetitive content.
        w, h = 64, 32
        buf = bytearray()
        for y in range(h):
            for x in range(w):
                if x < 16:
                    px = (255, 0, 0, 255)
                elif x < 32:
                    px = (0, 255, 0, 255)
                elif x < 48:
                    px = (0, 0, 255, 255)
                else:
                    px = (255, 255, 0, 255)
                if (y // 8) % 2:
                    px = (px[0], px[1], px[2], 180)
                buf += bytes(px)
        still = tmp / 'still.pam'
        exact_webp = tmp / 'exact.webp'
        better_webp = tmp / 'better.webp'
        write_pam(still, w, h, bytes(buf))
        run([str(ROOT / 'examples' / 'gwpencode'), '--method', '0', str(still), str(exact_webp)])
        run([str(ROOT / 'examples' / 'gwpencode'), '--method', '4', '--near-lossless', '80', str(still), str(better_webp)])
        assert better_webp.stat().st_size < exact_webp.stat().st_size, (better_webp.stat().st_size, exact_webp.stat().st_size)

        # Mixed mode should still assemble and round-trip even when no external tools are available.
        frames = []
        for i in range(3):
            anim = bytearray([10, 20, 30, 255] * (w * h))
            if i == 1:
                for yy in range(10, 18):
                    for xx in range(20, 28):
                        off = (yy * w + xx) * 4
                        anim[off:off + 4] = bytes([220, 30, 90, 255])
            elif i == 2:
                for yy in range(4, 12):
                    for xx in range(5, 17):
                        off = (yy * w + xx) * 4
                        anim[off:off + 4] = bytes([30, 200, 120, 200])
            frame = tmp / f'f{i}.pam'
            write_pam(frame, w, h, bytes(anim))
            frames.append(frame)
        anim_webp = tmp / 'anim.webp'
        dump_prefix = tmp / 'animdump'
        run([str(ROOT / 'examples' / 'gwpanimframes'), '--mixed', str(anim_webp)] + [str(f) for f in frames])
        run([str(ROOT / 'examples' / 'gwpanimdump'), str(anim_webp), str(dump_prefix)])
        for i, frame in enumerate(frames):
            assert read_pam_payload(frame) == read_pam_payload(tmp / f'animdump_{i:03d}.pam')

        # Optional cwebp-backed lossy path when official tools are present.
        cwebp = shutil.which('cwebp')
        if cwebp:
            lossy_webp = tmp / 'lossy.webp'
            lossy_out = tmp / 'lossy_out.pam'
            run([str(ROOT / 'examples' / 'gwpencode'), '--lossy', '--allow-tools', '--cwebp', cwebp, '--quality', '60', str(still), str(lossy_webp)])
            run([str(ROOT / 'examples' / 'gwpdecode'), str(lossy_webp), str(lossy_out)])
            assert lossy_webp.stat().st_size > 0
            assert len(read_pam_payload(lossy_out)) == w * h * 4

        # New encode-oracle scripts should be wired and callable.
        subprocess.run(['python3', str(ROOT / 'tests' / 'conformance' / 'run_encode_oracle.py'), '--help'], cwd=ROOT, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        subprocess.run(['python3', str(ROOT / 'tests' / 'conformance' / 'run_anim_encode_oracle.py'), '--help'], cwd=ROOT, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    print('ok: phase11 vp8l compression upgrades, mixed anim path, encode-oracle scaffolding')


if __name__ == '__main__':
    main()
