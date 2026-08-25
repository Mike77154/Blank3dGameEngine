import subprocess
import tempfile
from pathlib import Path

import numpy as np
from PIL import Image, features

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
    proc = subprocess.run(
        [str(ROOT / "examples" / "gwpdecode"), str(webp_path), str(out_path)],
        cwd=ROOT,
        check=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    if proc.stdout or proc.stderr:
        pass


def make_const(path: Path) -> None:
    img = Image.new("RGB", (16, 16), (128, 64, 32))
    img.save(path, format="WEBP", quality=72, lossless=False)


def make_busy(path: Path) -> None:
    img = Image.new("RGB", (48, 48))
    px = img.load()
    for y in range(48):
        for x in range(48):
            px[x, y] = (
                (x * 17 + y * 3) % 256,
                (x * 9 + y * 19) % 256,
                ((x ^ (y * 7)) * 11) % 256,
            )
    img.save(path, format="WEBP", quality=72, lossless=False)


def make_gradient(path: Path) -> None:
    img = Image.new("RGB", (48, 32))
    px = img.load()
    for y in range(32):
        for x in range(48):
            px[x, y] = ((x * 5) % 256, (y * 8) % 256, ((x + y) * 4) % 256)
    img.save(path, format="WEBP", quality=72, lossless=False)


def mean_abs_diff(a: np.ndarray, b: np.ndarray) -> float:
    return float(np.abs(a.astype(np.int16) - b.astype(np.int16)).mean())


def run_case(name: str, builder) -> None:
    with tempfile.TemporaryDirectory() as tmpdir:
        tmp = Path(tmpdir)
        webp_path = tmp / f"{name}.webp"
        out_path = tmp / f"{name}.pam"
        builder(webp_path)
        decode_with_example(webp_path, out_path)
        ours = read_pam_rgba(out_path)
        ref = np.array(Image.open(webp_path).convert("RGBA"), dtype=np.uint8)
        assert ours.shape == ref.shape
        assert np.any(ours[:, :, :3] != 0)
        assert np.all(ours[:, :, 3] == 255)
        assert mean_abs_diff(ours, ref) < 100.0


def main() -> None:
    must_have_webp()
    subprocess.run(["make", "clean"], cwd=ROOT, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    subprocess.run(["make"], cwd=ROOT, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    run_case("const", make_const)
    run_case("busy", make_busy)
    run_case("gradient", make_gradient)
    print("ok: vp8 phase5 frame reconstruction pipeline")


if __name__ == "__main__":
    main()
