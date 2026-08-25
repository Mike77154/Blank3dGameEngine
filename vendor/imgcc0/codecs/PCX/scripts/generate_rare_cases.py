#!/usr/bin/env python3
import json
import pathlib
import shutil
from typing import List


def u16le(v: int) -> bytes:
    return bytes((v & 0xFF, (v >> 8) & 0xFF))


def pack_bits(indices, bit):
    out = 0
    for x, idx in enumerate(indices[:8]):
        if ((idx >> bit) & 1) != 0:
            out |= 1 << (7 - x)
    return out


def rle_escape(byte_values):
    out = bytearray()
    for value in byte_values:
        if value >= 0xC0:
            out.append(0xC1)
        out.append(value)
    return bytes(out)


def build_4plane() -> bytes:
    row0 = [0, 1, 2, 3, 4, 5, 6, 7]
    row1 = [15, 14, 13, 12, 11, 10, 9, 8]
    rgb16 = [
        (0, 0, 0), (255, 0, 0), (0, 255, 0), (0, 0, 255),
        (255, 255, 0), (255, 0, 255), (0, 255, 255), (255, 255, 255),
        (128, 0, 0), (0, 128, 0), (0, 0, 128), (128, 128, 0),
        (128, 0, 128), (0, 128, 128), (192, 192, 192), (64, 64, 64),
    ]
    header = bytearray(128)
    header[0] = 0x0A
    header[1] = 5
    header[2] = 1
    header[3] = 1
    header[8:10] = u16le(7)
    header[10:12] = u16le(1)
    header[12:14] = u16le(72)
    header[14:16] = u16le(72)
    for i, (r, g, b) in enumerate(rgb16):
        base = 16 + i * 3
        header[base:base + 3] = bytes((r, g, b))
    header[65] = 4
    header[66:68] = u16le(2)
    header[68:70] = u16le(1)
    plane_bytes = []
    for bit in range(4):
        plane_bytes.extend([pack_bits(row0, bit), 0])
    for bit in range(4):
        plane_bytes.extend([pack_bits(row1, bit), 0])
    # order per row, all planes: rearrange
    row_major = []
    for row in [row0, row1]:
        for bit in range(4):
            row_major.extend([pack_bits(row, bit), 0])
    return bytes(header) + rle_escape(row_major)


def build_raw_odd_stride() -> bytes:
    indices = [1, 2, 3, 4, 5, 9, 8, 7, 6, 5]
    out = bytearray(128 + 10 + 769)
    out[0] = 0x0A
    out[1] = 5
    out[2] = 0
    out[3] = 8
    out[8:10] = u16le(4)
    out[10:12] = u16le(1)
    out[12:14] = u16le(72)
    out[14:16] = u16le(72)
    out[65] = 1
    out[66:68] = u16le(5)
    out[68:70] = u16le(1)
    out[128:138] = bytes(indices)
    out[138] = 0x0C
    for i in range(256):
        base = 139 + i * 3
        out[base:base + 3] = bytes((i & 0xFF, (255 - i) & 0xFF, (i * 5) & 0xFF))
    return bytes(out)


def write_manifest(root: pathlib.Path, cases: List[dict]) -> None:
    (root / "manifest.json").write_text(json.dumps({"cases": cases}, indent=2) + "\n")


def main() -> int:
    repo = pathlib.Path(__file__).resolve().parent.parent
    out_dir = repo / "tests" / "generated_corpus"
    out_dir.mkdir(parents=True, exist_ok=True)
    (out_dir / "generated_4plane.pcx").write_bytes(build_4plane())
    (out_dir / "generated_raw_odd_stride.pcx").write_bytes(build_raw_odd_stride())

    copied = []
    for name in ["valid_indexed8_palette.pcx", "invalid_reserved_round3.pcx", "malformed_truncated_rle.pcx"]:
        src = repo / "tests" / "fuzz_corpus" / name
        if src.exists():
            shutil.copy2(src, out_dir / name)
            copied.append(name)

    cases = [
        {"id": "generated-4plane", "path": "generated_4plane.pcx", "expected": {"inspect_ok": True, "rgb_ok": True, "indexed_ok": True, "strict_ok": True}, "tags": ["generated", "4-plane"]},
        {"id": "generated-raw-odd-stride", "path": "generated_raw_odd_stride.pcx", "expected": {"inspect_ok": True, "rgb_ok": True, "indexed_ok": True, "strict_ok": False}, "tags": ["generated", "raw", "odd-stride"]},
        {"id": "generated-valid-indexed8", "path": "valid_indexed8_palette.pcx", "expected": {"inspect_ok": True, "rgb_ok": True, "indexed_ok": True, "strict_ok": True}, "tags": ["generated", "baseline"]},
        {"id": "generated-invalid-reserved", "path": "invalid_reserved_round3.pcx", "expected": {"inspect_ok": True, "rgb_ok": True, "strict_ok": False}, "tags": ["generated", "reserved-nonzero"]},
        {"id": "generated-malformed-truncated-rle", "path": "malformed_truncated_rle.pcx", "expected": {"inspect_ok": True, "rgb_ok": False}, "tags": ["generated", "bad", "truncated-rle"]},
    ]
    write_manifest(out_dir, cases)
    print(f"generated corpus in {out_dir} ({len(cases)} manifest entries, {len(copied)} copied fixtures)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
