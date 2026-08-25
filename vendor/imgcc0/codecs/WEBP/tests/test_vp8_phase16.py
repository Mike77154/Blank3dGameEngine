import json
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def run(cmd):
    return subprocess.run(cmd, cwd=ROOT, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)


def write_pam(path: Path, width: int, height: int, payload: bytes) -> None:
    path.write_bytes(
        f'P7\nWIDTH {width}\nHEIGHT {height}\nDEPTH 4\nMAXVAL 255\nTUPLTYPE RGB_ALPHA\nENDHDR\n'.encode('ascii') + payload
    )


def read_payload(path: Path) -> bytes:
    return path.read_bytes().split(b'ENDHDR\n', 1)[1]


def main() -> None:
    run(['make', 'clean'])
    run(['make'])
    with tempfile.TemporaryDirectory() as tmpdir:
        tmp = Path(tmpdir)
        w = h = 64

        smooth_buf = bytearray()
        for y in range(h):
            for x in range(w):
                if x < 32 and y < 32:
                    r, g, b = 80, 90, 100
                elif x >= 32 and y < 32:
                    r, g, b = 120, 120, 120
                elif x < 32:
                    r, g, b = 40, 80, 120
                else:
                    r, g, b = 200, 180, 160
                smooth_buf += bytes([r, g, b, 255])
        smooth = tmp / 'smooth.pam'
        write_pam(smooth, w, h, bytes(smooth_buf))

        grad_buf = bytearray()
        for y in range(h):
            for x in range(w):
                r = (x * 4 + y * 2) % 256
                g = (x * 2 + y * 5) % 256
                b = (x * 3 + y * 7) % 256
                grad_buf += bytes([r, g, b, 255])
        grad = tmp / 'gradient.pam'
        write_pam(grad, w, h, bytes(grad_buf))

        smooth_native = tmp / 'smooth_native.vp8'
        smooth_legacy = tmp / 'smooth_legacy.vp8'
        smooth_meta = json.loads(run([str(ROOT / 'examples' / 'gwpvp8bitstream'), '--quality', '62', '--dump-json', str(smooth), str(smooth_native)]).stdout)
        legacy_meta = json.loads(run([str(ROOT / 'examples' / 'gwpvp8bitstream'), '--quality', '62', '--legacy-native', '--dump-json', str(smooth), str(smooth_legacy)]).stdout)

        assert smooth_meta['y_mode_mask'] != 16, smooth_meta
        assert smooth_meta['y2_non_zero'] > 0, smooth_meta
        assert smooth_native.stat().st_size <= smooth_legacy.stat().st_size, (smooth_native.stat().st_size, smooth_legacy.stat().st_size)

        grad_native = tmp / 'grad_native.vp8'
        grad_dc = tmp / 'grad_dc_only.vp8'
        grad_meta = json.loads(run([str(ROOT / 'examples' / 'gwpvp8bitstream'), '--quality', '62', '--dump-json', str(grad), str(grad_native)]).stdout)
        grad_dc_meta = json.loads(run([str(ROOT / 'examples' / 'gwpvp8bitstream'), '--quality', '62', '--dc-only', '--dump-json', str(grad), str(grad_dc)]).stdout)

        assert grad_meta['y_non_zero'] >= grad_dc_meta['y_non_zero'], (grad_meta, grad_dc_meta)
        assert grad_meta['uv_non_zero'] >= grad_dc_meta['uv_non_zero'], (grad_meta, grad_dc_meta)
        assert grad_native.stat().st_size > 0 and grad_dc.stat().st_size > 0

        webp = tmp / 'phase16.webp'
        decoded = tmp / 'phase16.pam'
        run([str(ROOT / 'examples' / 'gwpencode'), '--lossy', '--quality', '62', str(grad), str(webp)])
        run([str(ROOT / 'examples' / 'gwpdecode'), str(webp), str(decoded)])
        assert webp.read_bytes()[12:16] == b'VP8 '
        assert len(read_payload(decoded)) == w * h * 4

    print('ok: phase16 adds intra16/y2 and sparse ac residuals on native vp8 writer')


if __name__ == '__main__':
    main()
