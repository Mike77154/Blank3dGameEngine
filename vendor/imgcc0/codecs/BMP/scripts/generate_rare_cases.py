#!/usr/bin/env python3
import json
import pathlib
import shutil
from typing import List


def u16le(v: int) -> bytes:
    return bytes((v & 0xFF, (v >> 8) & 0xFF))


def u32le(v: int) -> bytes:
    return bytes((v & 0xFF, (v >> 8) & 0xFF, (v >> 16) & 0xFF, (v >> 24) & 0xFF))


def s32le(v: int) -> bytes:
    return u32le(v & 0xFFFFFFFF)


def build_indexed2_bmp() -> bytes:
    palette = [
        (0, 0, 0, 0),
        (0, 0, 255, 0),
        (0, 255, 0, 0),
        (255, 0, 0, 0),
    ]
    top = [0, 1, 2, 3, 1]
    bottom = [3, 2, 1, 0, 2]
    width = 5
    height = 2
    row_stride = 4
    pixel_offset = 14 + 40 + 16
    total = pixel_offset + row_stride * height
    out = bytearray(total)
    out[0:2] = b"BM"
    out[2:6] = u32le(total)
    out[10:14] = u32le(pixel_offset)
    out[14:18] = u32le(40)
    out[18:22] = s32le(width)
    out[22:26] = s32le(height)
    out[26:28] = u16le(1)
    out[28:30] = u16le(2)
    out[30:34] = u32le(0)
    out[34:38] = u32le(row_stride * height)
    out[38:42] = s32le(2835)
    out[42:46] = s32le(2835)
    out[46:50] = u32le(4)
    out[50:54] = u32le(4)
    for i, pe in enumerate(palette):
        base = 54 + i * 4
        out[base:base + 4] = bytes(pe)
    row = pixel_offset
    for x, idx in enumerate(bottom):
        out[row + (x >> 2)] |= (idx & 0x03) << (6 - ((x & 3) * 2))
    row += row_stride
    for x, idx in enumerate(top):
        out[row + (x >> 2)] |= (idx & 0x03) << (6 - ((x & 3) * 2))
    return bytes(out)


def build_os2v2_64_bmp() -> bytes:
    palette = [
        (0, 0, 0, 0),
        (0, 0, 255, 0),
        (0, 255, 0, 0),
        (255, 255, 0, 0),
    ]
    top = [0, 1, 2, 3]
    bottom = [3, 2, 1, 0]
    width = 4
    height = 2
    row_stride = 4
    pixel_offset = 14 + 64 + 16
    total = pixel_offset + row_stride * height
    out = bytearray(total)
    out[0:2] = b"BM"
    out[2:6] = u32le(total)
    out[10:14] = u32le(pixel_offset)
    out[14:18] = u32le(64)
    out[18:22] = s32le(width)
    out[22:26] = s32le(height)
    out[26:28] = u16le(1)
    out[28:30] = u16le(8)
    out[30:34] = u32le(0)
    out[34:38] = u32le(row_stride * height)
    out[38:42] = s32le(2835)
    out[42:46] = s32le(2835)
    out[46:50] = u32le(4)
    out[50:54] = u32le(4)
    for i, pe in enumerate(palette):
        base = 78 + i * 4
        out[base:base + 4] = bytes(pe)
    row = pixel_offset
    out[row:row + width] = bytes(bottom)
    row += row_stride
    out[row:row + width] = bytes(top)
    return bytes(out)


def write_manifest(root: pathlib.Path, cases: List[dict]) -> None:
    (root / "manifest.json").write_text(json.dumps({"cases": cases}, indent=2) + "\n")


def main() -> int:
    repo = pathlib.Path(__file__).resolve().parent.parent
    out_dir = repo / "tests" / "generated_corpus"
    out_dir.mkdir(parents=True, exist_ok=True)

    (out_dir / "generated_indexed2.bmp").write_bytes(build_indexed2_bmp())
    (out_dir / "generated_os2v2_64.bmp").write_bytes(build_os2v2_64_bmp())

    copied = []
    for name in ["invalid_topdown_rle8.bmp", "payload_png.bmp", "payload_jpeg_round3.bmp", "valid_rgb24.bmp"]:
        src = repo / "tests" / "fuzz_corpus" / name
        if src.exists():
            shutil.copy2(src, out_dir / name)
            copied.append(name)

    cases = [
        {"id": "generated-indexed2", "path": "generated_indexed2.bmp", "expected": {"parse_ok": True, "decode_ok": True}, "tags": ["generated", "2bpp"]},
        {"id": "generated-os2v2-64", "path": "generated_os2v2_64.bmp", "expected": {"parse_ok": True, "decode_ok": True}, "tags": ["generated", "os2-v2", "64-byte-dib"]},
        {"id": "generated-invalid-topdown-rle8", "path": "invalid_topdown_rle8.bmp", "expected": {"parse_ok": False}, "tags": ["generated", "bad", "top-down", "rle-invalid"]},
        {"id": "generated-payload-png", "path": "payload_png.bmp", "expected": {"parse_ok": True, "decode_ok": False}, "tags": ["generated", "embedded-png"]},
        {"id": "generated-payload-jpeg", "path": "payload_jpeg_round3.bmp", "expected": {"parse_ok": True, "decode_ok": False}, "tags": ["generated", "embedded-jpeg"]},
        {"id": "generated-valid-rgb24", "path": "valid_rgb24.bmp", "expected": {"parse_ok": True, "decode_ok": True}, "tags": ["generated", "baseline"]},
    ]
    write_manifest(out_dir, cases)
    print(f"generated corpus in {out_dir} ({len(cases)} manifest entries, {len(copied)} copied fixtures)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
