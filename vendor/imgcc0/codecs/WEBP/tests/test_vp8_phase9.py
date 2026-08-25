import json
import subprocess
import tempfile
from pathlib import Path

import numpy as np
from PIL import Image, features

ROOT = Path(__file__).resolve().parents[1]


def must_have_webp() -> None:
    if not features.check('webp'):
        raise SystemExit('Pillow en este entorno no tiene soporte WebP')


def read_pam(path: Path) -> np.ndarray:
    data = path.read_bytes()
    end = data.index(b'ENDHDR\n') + 7
    payload = data[end:]
    header = data[:end].decode('ascii', 'ignore').splitlines()
    width = height = depth = None
    for line in header:
        parts = line.strip().split()
        if len(parts) == 2 and parts[0] == 'WIDTH':
            width = int(parts[1])
        elif len(parts) == 2 and parts[0] == 'HEIGHT':
            height = int(parts[1])
        elif len(parts) == 2 and parts[0] == 'DEPTH':
            depth = int(parts[1])
    assert width is not None and height is not None and depth is not None
    arr = np.frombuffer(payload, dtype=np.uint8)
    return arr.reshape((height, width, depth))


def make_stills(tmp: Path) -> list[Path]:
    f1 = tmp / 'f1.webp'
    f2 = tmp / 'f2.webp'
    f3 = tmp / 'f3.webp'
    Image.new('RGBA', (4, 2), (255, 0, 0, 255)).save(f1, format='WEBP', lossless=True, method=6)
    Image.new('RGBA', (2, 2), (0, 0, 255, 128)).save(f2, format='WEBP', lossless=True, method=6)
    Image.new('RGBA', (2, 2), (0, 255, 0, 255)).save(f3, format='WEBP', lossless=True, method=6)
    return [f1, f2, f3]


def expected_frames() -> list[np.ndarray]:
    f0 = np.zeros((2, 4, 4), dtype=np.uint8)
    f0[:, :, :] = np.array([255, 0, 0, 255], dtype=np.uint8)
    f1 = f0.copy()
    f1[:, 2:, :] = np.array([127, 0, 128, 255], dtype=np.uint8)
    f2 = np.zeros((2, 4, 4), dtype=np.uint8)
    f2[:, :2, :] = np.array([0, 255, 0, 255], dtype=np.uint8)
    return [f0, f1, f2]


def main() -> None:
    must_have_webp()
    subprocess.run(['make', 'clean'], cwd=ROOT, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    subprocess.run(['make'], cwd=ROOT, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    with tempfile.TemporaryDirectory() as tmpdir:
        tmp = Path(tmpdir)
        frames = make_stills(tmp)
        manifest = tmp / 'frames.tsv'
        anim = tmp / 'anim_muxed.webp'
        prefix = tmp / 'dump'

        manifest.write_text(
            '\n'.join(
                [
                    f'{frames[0]}\t100\t0\t0\t1\t0',
                    f'{frames[1]}\t80\t3\t0\t0\t1',
                    f'{frames[2]}\t60\t0\t0\t1\t0',
                ]
            ) + '\n',
            encoding='utf-8',
        )

        subprocess.run(
            [str(ROOT / 'examples' / 'gwpanimux'), str(anim), '4', '2', '0', '0', str(manifest)],
            cwd=ROOT,
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )

        info = subprocess.run(
            [str(ROOT / 'examples' / 'gwpinfo'), str(anim)],
            cwd=ROOT,
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        assert 'format: ANIMATION' in info.stdout
        assert 'frames: 3' in info.stdout
        assert 'frame 1: x=2 y=0 w=2 h=2 dur=80 blend=0 dispose=1' in info.stdout, info.stdout
        assert 'ANIM size=6' in info.stdout
        assert info.stdout.count('ANMF') == 3

        subprocess.run(
            [str(ROOT / 'examples' / 'gwpanimdump'), str(anim), str(prefix)],
            cwd=ROOT,
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        exp = expected_frames()
        for i, expected in enumerate(exp):
            got = read_pam(tmp / f'dump_{i:03d}.pam')
            assert np.array_equal(got, expected), (i, got.tolist(), expected.tolist())

        metas = [json.loads(line) for line in (tmp / 'dump.jsonl').read_text(encoding='utf-8').splitlines()]
        assert [m['timestamp_ms'] for m in metas] == [0, 100, 180]
        assert [m['duration_ms'] for m in metas] == [100, 80, 60]

        with Image.open(anim) as im:
            assert getattr(im, 'n_frames', 1) == 3
    print('ok: vp8 phase9 anim mux/encoder round-trip')


if __name__ == '__main__':
    main()
