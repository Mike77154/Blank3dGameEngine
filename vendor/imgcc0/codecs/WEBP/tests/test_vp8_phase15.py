import json
import shutil
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


def bit_count(v: int) -> int:
    out = 0
    while v:
        out += v & 1
        v >>= 1
    return out


def main() -> None:
    run(['make', 'clean'])
    run(['make'])
    with tempfile.TemporaryDirectory() as tmpdir:
        tmp = Path(tmpdir)
        w, h = 128, 128
        buf = bytearray()
        for y in range(h):
            for x in range(w):
                if x < 64 and y < 64:
                    r, g, b = 40, 50, 60
                elif x < 64:
                    r, g, b = (x * 2) % 256, (y * 3) % 256, 100
                elif y < 64:
                    r, g, b = (x * 5 + y * 7) % 256, (x * 3) % 256, (y * 11) % 256
                else:
                    r = (200 + ((x ^ y) & 15)) % 256
                    g = (30 + (x // 4) % 50) % 256
                    b = (80 + (y // 4) % 70) % 256
                buf += bytes([r, g, b, 255])
        still = tmp / 'still.pam'
        write_pam(still, w, h, bytes(buf))

        new_raw = tmp / 'new.vp8'
        legacy_raw = tmp / 'legacy.vp8'
        new_meta = json.loads(run([str(ROOT / 'examples' / 'gwpvp8bitstream'), '--quality', '62', '--dump-json', str(still), str(new_raw)]).stdout)
        legacy_meta = json.loads(run([str(ROOT / 'examples' / 'gwpvp8bitstream'), '--quality', '62', '--legacy-native', '--dump-json', str(still), str(legacy_raw)]).stdout)

        assert new_meta['partitions'] >= 2
        assert new_meta['segments'] >= 2
        assert new_meta['skip_mbs'] > 0
        assert new_meta['coeff_updates'] > 0
        assert bit_count(new_meta['b_mode_mask']) > 1
        assert bit_count(new_meta['uv_mode_mask']) > 1
        assert legacy_meta['partitions'] == 1
        assert legacy_meta['segments'] == 1
        assert legacy_meta['coeff_updates'] == 0
        assert legacy_meta['b_mode_mask'] == 1
        assert legacy_meta['uv_mode_mask'] == 1
        assert new_raw.stat().st_size < legacy_raw.stat().st_size

        webp = tmp / 'phase15.webp'
        decoded = tmp / 'phase15.pam'
        run([str(ROOT / 'examples' / 'gwpencode'), '--lossy', '--quality', '62', str(still), str(webp)])
        run([str(ROOT / 'examples' / 'gwpdecode'), str(webp), str(decoded)])
        assert webp.read_bytes()[12:16] == b'VP8 '
        assert len(read_payload(decoded)) == w * h * 4

        oracle = ROOT / 'tests' / 'conformance' / 'run_native_lossy_oracle.py'
        if shutil.which('python3'):
            proc = run(['python3', str(oracle), str(still), '--quality', '62'])
            report = json.loads(proc.stdout)
            assert report['phase15_size'] <= report['legacy_size']

    print('ok: phase15 native vp8 writer uses partitions/segments/skip/prob updates and beats legacy size')


if __name__ == '__main__':
    main()
