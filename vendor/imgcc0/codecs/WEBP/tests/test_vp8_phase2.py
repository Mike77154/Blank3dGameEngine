import struct
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def build_synthetic_vp8_webp() -> bytes:
    part0 = b"\x00" * 14
    frame_tag = bytes([0xC0, 0x01, 0x00])
    key_hdr = bytes([0x9D, 0x01, 0x2A, 0x10, 0x00, 0x10, 0x00])
    vp8_payload = frame_tag + key_hdr + part0
    chunk = b"VP8 " + struct.pack("<I", len(vp8_payload)) + vp8_payload
    riff_size = 4 + len(chunk)
    return b"RIFF" + struct.pack("<I", riff_size) + b"WEBP" + chunk


def assert_contains(text: str, needle: str) -> None:
    if needle not in text:
        raise AssertionError(f"missing line: {needle}\n--- output ---\n{text}")


def main() -> None:
    subprocess.run(["make", "clean"], cwd=ROOT, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    subprocess.run(["make"], cwd=ROOT, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    with tempfile.TemporaryDirectory() as tmpdir:
        webp_path = Path(tmpdir) / "synthetic_phase2.webp"
        webp_path.write_bytes(build_synthetic_vp8_webp())
        proc = subprocess.run(
            [str(ROOT / "examples" / "gwpvp8probe"), str(webp_path)],
            cwd=ROOT,
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        out = proc.stdout
        assert_contains(out, "width: 16")
        assert_contains(out, "height: 16")
        assert_contains(out, "mb_cols: 1")
        assert_contains(out, "mb_rows: 1")
        assert_contains(out, "token_partitions.count: 1")
        assert_contains(out, "token_partition[0].size: 0")
        assert_contains(out, "quant.q_index: 0")
        assert_contains(out, "entropy.coeff_update_count: 0")
        assert_contains(out, "modes.macroblock_count: 1")
        assert_contains(out, "modes.y_mode_hist[4]: 1")
        assert_contains(out, "modes.b_mode_hist[0]: 16")
        assert_contains(out, "dequant[0].y1_dc: 4")
        assert_contains(out, "dequant[0].y1_ac: 4")

    print("ok: vp8 synthetic compatibility parser")


if __name__ == "__main__":
    main()
