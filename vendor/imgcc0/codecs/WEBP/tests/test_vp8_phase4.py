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


def make_busy_image(path: Path, size: tuple[int, int]) -> None:
    w, h = size
    img = Image.new("RGB", (w, h))
    px = img.load()
    for y in range(h):
        for x in range(w):
            px[x, y] = (
                (x * 17 + y * 3) % 256,
                (x * 9 + y * 19) % 256,
                ((x ^ (y * 7)) * 11) % 256,
            )
    img.save(path, format="WEBP", quality=68, lossless=False)


def parse_hist_sum(text: str, prefix: str) -> int:
    vals = [int(v) for v in re.findall(rf"^{re.escape(prefix)}\[\d+\]: (\d+)$", text, re.M)]
    if not vals:
        raise AssertionError(f"missing histogram: {prefix}\n--- output ---\n{text}")
    return sum(vals)


def main() -> None:
    must_have_webp()
    subprocess.run(["make", "clean"], cwd=ROOT, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    subprocess.run(["make"], cwd=ROOT, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    with tempfile.TemporaryDirectory() as tmpdir:
        webp_path = Path(tmpdir) / "phase4.webp"
        make_busy_image(webp_path, (48, 48))
        out = run_probe(webp_path)

        macroblocks = read_scalar(out, "modes.macroblock_count")
        bpred = read_scalar(out, "modes.bpred_macroblocks")
        y2_macroblocks = read_scalar(out, "residual.y2_macroblocks")
        coeff_blocks = read_scalar(out, "residual.coeff_block_count")
        blocks_with_coeffs = read_scalar(out, "residual.blocks_with_coeffs")
        y_blocks = read_scalar(out, "residual.y_blocks_with_coeffs")
        uv_blocks = read_scalar(out, "residual.uv_blocks_with_coeffs")
        y2_blocks = read_scalar(out, "residual.y2_blocks_with_coeffs")
        token_count = parse_hist_sum(out, "residual.token_hist")
        touched0 = read_scalar(out, "residual.token_partition_bytes_touched[0]")
        size0 = read_scalar(out, "token_partition[0].size")

        assert y2_macroblocks == macroblocks - bpred
        assert coeff_blocks == macroblocks * 24 + y2_macroblocks
        assert blocks_with_coeffs > 0
        assert y_blocks + uv_blocks + y2_blocks == blocks_with_coeffs
        assert token_count >= blocks_with_coeffs
        assert touched0 > 0
        assert touched0 <= size0
        assert read_scalar(out, "residual.nonzero_coeff_count") > 0
        assert read_scalar(out, "residual.max_abs_coeff") > 0

    proc = subprocess.run(
        [str(ROOT / "examples" / "gwpvp8kernels")],
        cwd=ROOT,
        check=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    kout = proc.stdout
    assert "idct.sum:" in kout
    assert "wht.sum:" in kout
    assert "recon4.sum:" in kout
    assert "filter.edge_pixels:" in kout
    assert read_scalar(kout, "idct.first") != 0
    assert read_scalar(kout, "wht.first") != 0
    assert read_scalar(kout, "recon4.sum") > 0

    print("ok: vp8 phase4 residual + transform + recon + filter")


if __name__ == "__main__":
    main()
