import json
import struct
import subprocess
import sys
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


def extract_bitstream_chunk(path: Path):
    data = path.read_bytes()
    assert data[:4] == b'RIFF' and data[8:12] == b'WEBP'
    pos = 12
    while pos + 8 <= len(data):
        tag = data[pos:pos + 4]
        size = struct.unpack_from('<I', data, pos + 4)[0]
        payload = data[pos + 8:pos + 8 + size]
        if tag in (b'VP8 ', b'VP8L'):
            return tag, payload
        pos += 8 + ((size + 1) & ~1)
    raise AssertionError('no encontré VP8/VP8L')


def riff_chunk(tag: bytes, payload: bytes) -> bytes:
    return tag + struct.pack('<I', len(payload)) + payload + (b'\x00' if len(payload) & 1 else b'')


def anmf_chunk(x: int, y: int, w: int, h: int, duration_ms: int, blend: int, dispose: int, subchunks: list[bytes]) -> bytes:
    flags = ((blend & 1) << 1) | (dispose & 1)
    payload = (
        (x // 2).to_bytes(3, 'little')
        + (y // 2).to_bytes(3, 'little')
        + (w - 1).to_bytes(3, 'little')
        + (h - 1).to_bytes(3, 'little')
        + duration_ms.to_bytes(3, 'little')
        + bytes([flags])
        + b''.join(subchunks)
    )
    return riff_chunk(b'ANMF', payload)


def build_anim_webp(canvas_w: int, canvas_h: int, bg_bgra: int, loop_count: int, frames: list[bytes]) -> bytes:
    vp8x_flags = 0x12  # alpha + animation
    vp8x = riff_chunk(
        b'VP8X',
        bytes([vp8x_flags, 0, 0, 0]) + (canvas_w - 1).to_bytes(3, 'little') + (canvas_h - 1).to_bytes(3, 'little'),
    )
    anim = riff_chunk(b'ANIM', struct.pack('<I', bg_bgra) + struct.pack('<H', loop_count))
    body = vp8x + anim + b''.join(frames)
    return b'RIFF' + struct.pack('<I', len(body) + 4) + b'WEBP' + body


def make_anim(path: Path) -> None:
    f1 = path.with_name('f1.webp')
    f2 = path.with_name('f2.webp')
    f3 = path.with_name('f3.webp')
    Image.new('RGBA', (4, 2), (255, 0, 0, 255)).save(f1, format='WEBP', lossless=True, method=6)
    Image.new('RGBA', (2, 2), (0, 0, 255, 128)).save(f2, format='WEBP', lossless=True, method=6)
    Image.new('RGBA', (2, 2), (0, 255, 0, 255)).save(f3, format='WEBP', lossless=True, method=6)
    t1, p1 = extract_bitstream_chunk(f1)
    t2, p2 = extract_bitstream_chunk(f2)
    t3, p3 = extract_bitstream_chunk(f3)
    anim = build_anim_webp(
        4,
        2,
        0x00000000,
        0,
        [
            anmf_chunk(0, 0, 4, 2, 100, 1, 0, [riff_chunk(t1, p1)]),
            anmf_chunk(2, 0, 2, 2, 80, 0, 1, [riff_chunk(t2, p2)]),
            anmf_chunk(0, 0, 2, 2, 60, 1, 0, [riff_chunk(t3, p3)]),
        ],
    )
    path.write_bytes(anim)


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
        anim_path = tmp / 'anim.webp'
        prefix = tmp / 'dump'
        manifest = tmp / 'animated_manifest.txt'
        report_dir = tmp / 'report'

        make_anim(anim_path)
        info = subprocess.run([str(ROOT / 'examples' / 'gwpinfo'), str(anim_path)], cwd=ROOT, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        assert 'format: ANIMATION' in info.stdout
        assert 'frames: 3' in info.stdout
        assert 'blend=0 dispose=1' in info.stdout

        subprocess.run([str(ROOT / 'examples' / 'gwpanimdump'), str(anim_path), str(prefix)], cwd=ROOT, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        exp = expected_frames()
        for i, expected in enumerate(exp):
            got = read_pam(tmp / f'dump_{i:03d}.pam')
            assert np.array_equal(got, expected), (i, got.tolist(), expected.tolist())

        meta_lines = (tmp / 'dump.jsonl').read_text(encoding='utf-8').splitlines()
        metas = [json.loads(line) for line in meta_lines]
        assert [m['timestamp_ms'] for m in metas] == [0, 100, 180]
        assert [m['duration_ms'] for m in metas] == [100, 80, 60]

        manifest.write_text('anim.webp\tsmoke,animated,lossless\n', encoding='utf-8')
        oracle = subprocess.run(
            [
                sys.executable,
                str(ROOT / 'tests' / 'conformance' / 'run_anim_oracle.py'),
                '--corpus-dir', str(tmp),
                '--manifest', str(manifest),
                '--out-dir', str(report_dir),
                '--mean-threshold', '1',
                '--max-threshold', '1',
            ],
            cwd=ROOT,
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        summary = json.loads(oracle.stdout)
        assert summary['FAIL'] == 0
        assert summary['PASS_EXACT'] + summary['PASS_VISUAL_BUT_NOT_EXACT'] == 1
    print('ok: vp8 phase8 animation decode + animated conformance harness')


if __name__ == '__main__':
    main()
