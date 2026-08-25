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
        w, h = 32, 24
        buf = bytearray()
        for y in range(h):
            for x in range(w):
                r = (x * 7 + y * 13) % 256
                g = (x * 11 + y * 5) % 256
                b = (x * 3 + y * 17) % 256
                a = 255
                buf += bytes([r, g, b, a])
        still = tmp / 'still.pam'
        write_pam(still, w, h, bytes(buf))

        vp8_raw = tmp / 'out.vp8'
        proc = run([str(ROOT / 'examples' / 'gwpvp8bitstream'), '--quality', '62', '--dump-json', str(still), str(vp8_raw)])
        meta = json.loads(proc.stdout)
        assert meta['width'] == w and meta['height'] == h
        assert meta['part0'] > 0 and meta['token_part'] > 0
        raw = vp8_raw.read_bytes()
        assert raw[3:6] == bytes([0x9d, 0x01, 0x2a])

        webp = tmp / 'native.webp'
        out = run([str(ROOT / 'examples' / 'gwpencode'), '--lossy', '--quality', '62', str(still), str(webp)])
        decoded = tmp / 'decoded.pam'
        run([str(ROOT / 'examples' / 'gwpdecode'), str(webp), str(decoded)])
        assert webp.read_bytes()[12:16] == b'VP8 '
        assert len(read_payload(decoded)) == w * h * 4
        assert read_payload(decoded) != bytes(buf)

        # transparent input should stay on fallback path today
        tr = bytearray(buf)
        tr[3] = 0
        tr_in = tmp / 'trans.pam'
        tr_webp = tmp / 'trans.webp'
        write_pam(tr_in, w, h, bytes(tr))
        run([str(ROOT / 'examples' / 'gwpencode'), '--lossy', '--quality', '62', str(tr_in), str(tr_webp)])
        assert tr_webp.read_bytes()[12:16] != b'VP8 '

    print('ok: phase14 native vp8 bool/token writer path')


if __name__ == '__main__':
    main()
