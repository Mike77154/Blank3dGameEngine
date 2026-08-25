import subprocess
import tempfile
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, features

ROOT = Path(__file__).resolve().parents[1]


def must_have_webp() -> None:
    if not features.check("webp"):
        raise SystemExit("Pillow en este entorno no tiene soporte WebP")


def read_pam_rgba(path: Path) -> np.ndarray:
    with path.open("rb") as f:
        header = []
        while True:
            line = f.readline()
            if not line:
                raise AssertionError("PAM truncado")
            header.append(line)
            if line.strip() == b"ENDHDR":
                break
        payload = f.read()
    width = height = None
    for raw in header:
        parts = raw.decode("ascii", "ignore").strip().split()
        if len(parts) == 2 and parts[0] == "WIDTH":
            width = int(parts[1])
        elif len(parts) == 2 and parts[0] == "HEIGHT":
            height = int(parts[1])
    if width is None or height is None:
        raise AssertionError("PAM sin WIDTH/HEIGHT")
    return np.frombuffer(payload, dtype=np.uint8).reshape(height, width, 4)


def decode_with_example(webp_path: Path, out_path: Path) -> None:
    subprocess.run(
        [str(ROOT / "examples" / "gwpdecode"), str(webp_path), str(out_path)],
        cwd=ROOT,
        check=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )


def info_with_example(webp_path: Path) -> str:
    proc = subprocess.run(
        [str(ROOT / "examples" / "gwpinfo"), str(webp_path)],
        cwd=ROOT,
        check=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    return proc.stdout


def make_lossy_rgba(path: Path) -> None:
    img = Image.new("RGBA", (64, 48), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    for y in range(48):
        for x in range(64):
            a = (x * 4 + y * 3) % 256
            img.putpixel((x, y), ((x * 9) % 256, (y * 13) % 256, ((x + y) * 7) % 256, a))
    draw.rectangle((6, 6, 25, 28), fill=(240, 30, 20, 220))
    draw.ellipse((28, 8, 58, 38), fill=(20, 220, 80, 96))
    draw.line((0, 47, 63, 0), fill=(255, 255, 255, 180), width=3)
    img.save(path, format="WEBP", quality=70, lossless=False, method=4)


def mean_abs_diff(a: np.ndarray, b: np.ndarray) -> float:
    return float(np.abs(a.astype(np.int16) - b.astype(np.int16)).mean())


def main() -> None:
    must_have_webp()
    subprocess.run(["make", "clean"], cwd=ROOT, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    subprocess.run(["make"], cwd=ROOT, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    with tempfile.TemporaryDirectory() as tmpdir:
        tmp = Path(tmpdir)
        webp_path = tmp / "lossy_alpha.webp"
        out_path = tmp / "lossy_alpha.pam"
        make_lossy_rgba(webp_path)
        info = info_with_example(webp_path)
        assert "format: VP8" in info, info
        assert "alpha: 1" in info, info
        assert "ALPH" in info and "VP8 " in info, info
        decode_with_example(webp_path, out_path)
        ours = read_pam_rgba(out_path)
        ref = np.array(Image.open(webp_path).convert("RGBA"), dtype=np.uint8)
        assert ours.shape == ref.shape
        assert int(ours[:, :, 3].min()) < 255
        assert int(ours[:, :, 3].max()) > 0
        assert mean_abs_diff(ours[:, :, 3], ref[:, :, 3]) < 6.0
        assert mean_abs_diff(ours, ref) < 105.0
    print("ok: vp8 phase6 precision + alph")


if __name__ == "__main__":
    main()
