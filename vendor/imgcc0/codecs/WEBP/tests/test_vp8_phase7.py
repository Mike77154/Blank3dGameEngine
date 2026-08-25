import json
import subprocess
import tempfile
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, features

ROOT = Path(__file__).resolve().parents[1]


def must_have_webp() -> None:
    if not features.check("webp"):
        raise SystemExit("Pillow en este entorno no tiene soporte WebP")


def write_pam(path: Path, arr: np.ndarray) -> None:
    h, w, d = arr.shape
    with path.open("wb") as f:
        f.write(f"P7\nWIDTH {w}\nHEIGHT {h}\nDEPTH {d}\nMAXVAL 255\nTUPLTYPE RGB_ALPHA\nENDHDR\n".encode("ascii"))
        f.write(arr.tobytes())


def make_odd_lossy_alpha(path: Path) -> None:
    img = Image.new("RGBA", (31, 13), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    for y in range(13):
        for x in range(31):
            img.putpixel((x, y), ((x * 17) % 256, (y * 29) % 256, ((x * 11 + y * 7) % 256), (x * 9 + y * 13) % 256))
    draw.rectangle((1, 1, 11, 8), fill=(220, 30, 40, 200))
    draw.line((0, 12, 30, 0), fill=(255, 255, 255, 128), width=1)
    img.save(path, format="WEBP", quality=72, lossless=False, method=4)


def main() -> None:
    must_have_webp()
    subprocess.run(["make", "clean"], cwd=ROOT, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    subprocess.run(["make"], cwd=ROOT, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    with tempfile.TemporaryDirectory() as tmpdir:
        tmp = Path(tmpdir)
        webp_path = tmp / "odd_alpha.webp"
        yuv_path = tmp / "odd_alpha.yuv"
        pam_a = tmp / "a.pam"
        pam_b = tmp / "b.pam"
        pam_c = tmp / "c.pam"
        manifest = tmp / "manifest.txt"
        report_dir = tmp / "report"

        make_odd_lossy_alpha(webp_path)
        subprocess.run([str(ROOT / "examples" / "gwpdumpyuv"), str(webp_path), str(yuv_path)], cwd=ROOT, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        expected_yuv = 31 * 13 + 2 * (((31 + 1) // 2) * ((13 + 1) // 2))
        assert yuv_path.stat().st_size == expected_yuv

        arr = np.zeros((2, 3, 4), dtype=np.uint8)
        arr[:, :, 0] = 7
        arr[:, :, 1] = 9
        arr[:, :, 2] = 11
        arr[:, :, 3] = 13
        write_pam(pam_a, arr)
        write_pam(pam_b, arr)
        arr2 = arr.copy()
        arr2[1, 2, 3] = 99
        write_pam(pam_c, arr2)

        exact = subprocess.run(
            [
                "python",
                str(ROOT / "tests" / "conformance" / "compare_planes.py"),
                "--format",
                "pam",
                str(pam_a),
                str(pam_b),
            ],
            cwd=ROOT,
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        exact_obj = json.loads(exact.stdout)
        assert exact_obj["status"] == "PASS_EXACT"

        fail = subprocess.run(
            [
                "python",
                str(ROOT / "tests" / "conformance" / "compare_planes.py"),
                "--format",
                "pam",
                str(pam_a),
                str(pam_c),
                "--mean-threshold",
                "0",
                "--max-threshold",
                "0",
            ],
            cwd=ROOT,
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        fail_obj = json.loads(fail.stdout)
        assert fail_obj["status"] == "FAIL"
        assert fail_obj["first_mismatch"]["plane"] == "A"

        manifest.write_text(f"{webp_path.name}\tpam\tsmoke,odd,alpha\n")
        oracle = subprocess.run(
            [
                "python",
                str(ROOT / "tests" / "conformance" / "run_oracle.py"),
                "--oracle",
                "pillow",
                "--corpus-dir",
                str(tmp),
                "--manifest",
                str(manifest),
                "--out-dir",
                str(report_dir),
                "--mean-threshold",
                "255",
                "--max-threshold",
                "255",
            ],
            cwd=ROOT,
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        summary = json.loads(oracle.stdout)
        assert summary["PASS_EXACT"] + summary["PASS_VISUAL_BUT_NOT_EXACT"] == 1
        report = subprocess.run(
            ["python", str(ROOT / "tests" / "conformance" / "report_failures.py"), str(report_dir / "results.jsonl")],
            cwd=ROOT,
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        assert "Resumen" in report.stdout
    print("ok: vp8 phase7 conformance harness + plane diff + yuv dump")


if __name__ == "__main__":
    main()
