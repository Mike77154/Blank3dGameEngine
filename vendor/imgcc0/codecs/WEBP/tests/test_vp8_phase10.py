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
    data = path.read_bytes()
    return data.split(b'ENDHDR\n', 1)[1]


def main() -> None:
    subprocess.run(['make', 'clean'], cwd=ROOT, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    subprocess.run(['make'], cwd=ROOT, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    with tempfile.TemporaryDirectory() as tmpdir:
        tmp = Path(tmpdir)

        w, h = 5, 4
        still = bytearray()
        for y in range(h):
            for x in range(w):
                still += bytes([(x * 50) % 256, (y * 70) % 256, ((x + y) * 35) % 256, 255 if (x + y) % 2 == 0 else 90])
        still_pam = tmp / 'still.pam'
        still_webp = tmp / 'still.webp'
        still_out = tmp / 'still_out.pam'
        write_pam(still_pam, w, h, bytes(still))

        subprocess.run(
            [str(ROOT / 'examples' / 'gwpencode'), str(still_pam), str(still_webp)],
            cwd=ROOT,
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        subprocess.run(
            [str(ROOT / 'examples' / 'gwpdecode'), str(still_webp), str(still_out)],
            cwd=ROOT,
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        assert read_pam_payload(still_pam) == read_pam_payload(still_out)

        frames = []
        for i in range(3):
            buf = bytearray([20, 30, 40, 255] * (w * h))
            if i == 1:
                idx = (1 * w + 3) * 4
                buf[idx:idx + 4] = bytes([200, 100, 70, 180])
            elif i == 2:
                idx = (2 * w + 1) * 4
                buf[idx:idx + 4] = bytes([10, 220, 90, 255])
            frame = tmp / f'f{i}.pam'
            write_pam(frame, w, h, bytes(buf))
            frames.append(frame)

        anim_webp = tmp / 'anim.webp'
        dump_prefix = tmp / 'dump'
        subprocess.run(
            [str(ROOT / 'examples' / 'gwpanimframes'), str(anim_webp)] + [str(f) for f in frames],
            cwd=ROOT,
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        subprocess.run(
            [str(ROOT / 'examples' / 'gwpanimdump'), str(anim_webp), str(dump_prefix)],
            cwd=ROOT,
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        for i, frame in enumerate(frames):
            assert read_pam_payload(frame) == read_pam_payload(tmp / f'dump_{i:03d}.pam')

        meta = [json.loads(line) for line in (tmp / 'dump.jsonl').read_text(encoding='utf-8').splitlines()]
        assert meta[1]['x'] == 2 and meta[1]['y'] == 0 and meta[1]['width'] == 2 and meta[1]['height'] == 2, meta[1]
        assert meta[2]['width'] < w and meta[2]['height'] < h, meta[2]

    print('ok: phase10 raw-frame lossless still/anim encode round-trip')


if __name__ == '__main__':
    main()
