import re
import subprocess
import tempfile
from pathlib import Path

from PIL import Image, features

ROOT = Path(__file__).resolve().parents[1]


def must_have_webp() -> None:
    if not features.check("webp"):
        raise SystemExit("Pillow en este entorno no tiene soporte WebP")


def run_probe(path: Path) -> str:
    proc = subprocess.run(
        [str(ROOT / "examples" / "gwpvp8probe"), str(path)],
        cwd=ROOT,
        check=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    return proc.stdout


def read_scalar(text: str, key: str) -> int:
    m = re.search(rf"^{re.escape(key)}: (-?\d+)$", text, re.M)
    if not m:
        raise AssertionError(f"missing scalar: {key}\n--- output ---\n{text}")
    return int(m.group(1))


def sum_hist(text: str, prefix: str) -> int:
    vals = [int(v) for v in re.findall(rf"^{re.escape(prefix)}\[\d+\]: (\d+)$", text, re.M)]
    if not vals:
        raise AssertionError(f"missing histogram: {prefix}\n--- output ---\n{text}")
    return sum(vals)


def make_gradient(path: Path, size: tuple[int, int]) -> None:
    w, h = size
    img = Image.new("RGB", (w, h))
    px = img.load()
    for y in range(h):
        for x in range(w):
            px[x, y] = ((x * 7) % 256, (y * 5) % 256, ((x ^ y) * 13) % 256)
    img.save(path, format="WEBP", quality=70, lossless=False)


def make_checker(path: Path, size: tuple[int, int]) -> None:
    w, h = size
    img = Image.new("RGB", (w, h))
    px = img.load()
    for y in range(h):
        for x in range(w):
            c = 255 if ((x // 4) + (y // 4)) & 1 else 0
            px[x, y] = (c, 255 - c, (x * 9 + y * 3) % 256)
    img.save(path, format="WEBP", quality=75, lossless=False)


def validate_common(text: str, expected_w: int, expected_h: int) -> None:
    mb_cols = read_scalar(text, "mb_cols")
    mb_rows = read_scalar(text, "mb_rows")
    macroblocks = read_scalar(text, "modes.macroblock_count")
    y_sum = sum_hist(text, "modes.y_mode_hist")
    uv_sum = sum_hist(text, "modes.uv_mode_hist")
    seg_sum = sum_hist(text, "modes.segment_hist")
    bpred = read_scalar(text, "modes.bpred_macroblocks")
    b_sum = sum_hist(text, "modes.b_mode_hist")
    token_partitions = read_scalar(text, "token_partitions.count")

    assert read_scalar(text, "width") == expected_w
    assert read_scalar(text, "height") == expected_h
    assert macroblocks == mb_cols * mb_rows
    assert y_sum == macroblocks
    assert uv_sum == macroblocks
    assert seg_sum == macroblocks
    assert token_partitions in (1, 2, 4, 8)
    assert read_scalar(text, "entropy_header_bytes_touched") >= read_scalar(text, "control_header_bytes_touched")
    assert read_scalar(text, "part0_bytes_touched") >= read_scalar(text, "entropy_header_bytes_touched")
    assert read_scalar(text, "part0_bytes_touched") <= read_scalar(text, "first_partition_size")
    if bpred > 0:
        assert b_sum == bpred * 16
    else:
        assert b_sum == 0


def main() -> None:
    must_have_webp()
    subprocess.run(["make", "clean"], cwd=ROOT, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    subprocess.run(["make"], cwd=ROOT, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    with tempfile.TemporaryDirectory() as tmpdir:
        tmp = Path(tmpdir)
        gradient = tmp / "gradient.webp"
        checker = tmp / "checker.webp"

        make_gradient(gradient, (32, 32))
        make_checker(checker, (48, 32))

        out_a = run_probe(gradient)
        out_b = run_probe(checker)

        validate_common(out_a, 32, 32)
        validate_common(out_b, 48, 32)

    print("ok: vp8 phase3 entropy + modes + token scaffold")


if __name__ == "__main__":
    main()
