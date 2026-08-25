import json
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def write_pam(path: Path, width: int, height: int, payload: bytes) -> None:
    path.write_bytes(
        f'P7\nWIDTH {width}\nHEIGHT {height}\nDEPTH 4\nMAXVAL 255\nTUPLTYPE RGB_ALPHA\nENDHDR\n'.encode('ascii') + payload
    )


def read_pam_payload(path: Path) -> bytes:
    return path.read_bytes().split(b'ENDHDR\n', 1)[1]


def run(cmd):
    return subprocess.run(cmd, cwd=ROOT, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)


def main() -> None:
    run(['make', 'clean'])
    run(['make'])

    with tempfile.TemporaryDirectory() as tmpdir:
        tmp = Path(tmpdir)
        w, h = 48, 32
        buf = bytearray()
        for y in range(h):
            for x in range(w):
                r = (x * 17 + y * 11) % 256
                g = (x * 7 + y * 19) % 256
                b = (x * 13 + y * 5 + (x ^ y)) % 256
                a = 255 if (x + y) % 9 else 180
                buf += bytes([r, g, b, a])
        still = tmp / 'still.pam'
        write_pam(still, w, h, bytes(buf))

        proc = run([str(ROOT / 'examples' / 'gwpvp8emit'), '--quality', '58', '--preset', 'photo', '--dump-json', str(still)])
        meta = json.loads(proc.stdout)
        assert meta['cell_count'] > 0
        assert meta['average_qindex'] > 0
        assert meta['suggested_partitions'] in (1, 2, 4, 8)
        assert any(cell['w'] in (4, 8, 16) for cell in meta['cells'])

        lossless_webp = tmp / 'lossless.webp'
        native_lossy_webp = tmp / 'native_lossy.webp'
        native_lossy_pam = tmp / 'native_lossy.pam'
        run([str(ROOT / 'examples' / 'gwpencode'), str(still), str(lossless_webp)])
        run([str(ROOT / 'examples' / 'gwpencode'), '--lossy', '--quality', '55', '--preset', 'photo', str(still), str(native_lossy_webp)])
        run([str(ROOT / 'examples' / 'gwpdecode'), str(native_lossy_webp), str(native_lossy_pam)])
        assert native_lossy_webp.stat().st_size < lossless_webp.stat().st_size
        assert read_pam_payload(native_lossy_pam) != read_pam_payload(still)
        assert len(read_pam_payload(native_lossy_pam)) == w * h * 4

        proxy_webp = tmp / 'proxy.webp'
        run([str(ROOT / 'examples' / 'gwpvp8emit'), '--proxy-webp', str(proxy_webp), str(still)])
        assert proxy_webp.exists() and proxy_webp.stat().st_size > 0

        frames = []
        for i in range(3):
            frame_buf = bytearray()
            for y in range(h):
                for x in range(w):
                    base = 20 + i * 40
                    r = (base + x * 3) % 256
                    g = (90 + y * 5 + i * 9) % 256
                    b = (150 + x * 2 + y * 3 + i * 11) % 256
                    a = 255
                    if 10 + i * 4 <= x < 22 + i * 4 and 8 <= y < 20:
                        r, g, b = 240 - i * 30, 40 + i * 20, 20 + i * 40
                    frame_buf += bytes([r, g, b, a])
            frame = tmp / f'f{i}.pam'
            write_pam(frame, w, h, bytes(frame_buf))
            frames.append(frame)
        anim = tmp / 'anim_lossy.webp'
        dump_prefix = tmp / 'animdump'
        run([str(ROOT / 'examples' / 'gwpanimframes'), '--lossy', '--quality', '52', str(anim)] + [str(f) for f in frames])
        run([str(ROOT / 'examples' / 'gwpanimdump'), str(anim), str(dump_prefix)])
        dumped = list(tmp.glob('animdump_*.pam'))
        assert len(dumped) == 3
        assert (tmp / 'animdump.jsonl').exists()

    print('ok: phase13 native intra emitter foundation and no-tool lossy proxy path')


if __name__ == '__main__':
    main()
