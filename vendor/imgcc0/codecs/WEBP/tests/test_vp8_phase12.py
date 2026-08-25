import json
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
    return path.read_bytes().split(b'ENDHDR\n', 1)[1]


def run(cmd, **kwargs):
    return subprocess.run(cmd, cwd=ROOT, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, **kwargs)


def main() -> None:
    run(['make', 'clean'])
    run(['make'])

    with tempfile.TemporaryDirectory() as tmpdir:
        tmp = Path(tmpdir)
        w, h = 32, 24
        buf = bytearray()
        for y in range(h):
            for x in range(w):
                if (x // 8 + y // 8) % 2 == 0:
                    px = (230, 40, 20, 255)
                else:
                    px = (20, 80, 220, 180 if x > 20 else 255)
                buf += bytes(px)
        still = tmp / 'still.pam'
        write_pam(still, w, h, bytes(buf))

        # New native lossy planner should report meaningful summary.
        proc = run([str(ROOT / 'examples' / 'gwpvp8plan'), '--quality', '72', '--preset', 'photo', str(still)])
        text = proc.stdout
        assert 'macroblocks=' in text
        assert 'suggested_segments=' in text
        assert 'suggested_partitions=' in text
        vals = dict(line.split('=', 1) for line in text.strip().splitlines() if '=' in line)
        assert int(vals['macroblocks']) == 4
        assert 1 <= int(vals['suggested_segments']) <= 4
        assert int(vals['suggested_partitions']) in (1, 2, 4, 8)

        # New encode flags should still produce a valid lossless file.
        out_webp = tmp / 'out.webp'
        out_pam = tmp / 'out.pam'
        run([str(ROOT / 'examples' / 'gwpencode'), '--preset', 'picture', '--alpha-q', '90', '--filter-strength', '42', str(still), str(out_webp)])
        run([str(ROOT / 'examples' / 'gwpdecode'), str(out_webp), str(out_pam)])
        assert read_pam_payload(still) == read_pam_payload(out_pam)

        # Scheduler fix path: no-min-size should still allow delta frames instead of forcing all-keyframes.
        frames = []
        for i in range(3):
            frame_buf = bytearray([0, 0, 0, 255] * (w * h))
            for yy in range(6 + i, 12 + i):
                for xx in range(4 + i * 3, 10 + i * 3):
                    off = (yy * w + xx) * 4
                    frame_buf[off:off + 4] = bytes([30 + i * 50, 120, 220 - i * 40, 255])
            frame = tmp / f'f{i}.pam'
            write_pam(frame, w, h, bytes(frame_buf))
            frames.append(frame)
        anim = tmp / 'anim.webp'
        dump_prefix = tmp / 'animdump'
        run([str(ROOT / 'examples' / 'gwpanimframes'), '--no-min-size', '--kmin', '3', '--kmax', '5', str(anim)] + [str(f) for f in frames])
        run([str(ROOT / 'examples' / 'gwpanimdump'), str(anim), str(dump_prefix)])
        for i, frame in enumerate(frames):
            assert read_pam_payload(frame) == read_pam_payload(tmp / f'animdump_{i:03d}.pam')

        # Tool discovery script should be callable and JSON-shaped.
        proc = run(['python3', str(ROOT / 'tests' / 'conformance' / 'discover_official_tools.py')])
        tools = json.loads(proc.stdout)
        assert 'cwebp' in tools and 'img2webp' in tools

        # Optional lossy bridge with richer knobs when cwebp exists.
        cwebp = shutil.which('cwebp')
        if cwebp:
            lossy = tmp / 'lossy.webp'
            lossy_pam = tmp / 'lossy.pam'
            run([str(ROOT / 'examples' / 'gwpencode'), '--lossy', '--allow-tools', '--cwebp', cwebp,
                 '--quality', '60', '--preset', 'photo', '--filter-strength', '44', '--alpha-q', '80',
                 str(still), str(lossy)])
            run([str(ROOT / 'examples' / 'gwpdecode'), str(lossy), str(lossy_pam)])
            assert len(read_pam_payload(lossy_pam)) == w * h * 4

        # Updated oracle CLIs should expose help.
        subprocess.run(['python3', str(ROOT / 'tests' / 'conformance' / 'run_encode_oracle.py'), '--help'], cwd=ROOT, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        subprocess.run(['python3', str(ROOT / 'tests' / 'conformance' / 'run_anim_encode_oracle.py'), '--help'], cwd=ROOT, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    print('ok: phase12 native lossy planning, tool knobs, scheduler fix, conformance tooling')


if __name__ == '__main__':
    main()
