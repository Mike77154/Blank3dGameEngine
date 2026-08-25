#!/usr/bin/env python3
"""Optional helper for demo_asset_preview_ppm.

The C89 core intentionally does not include a PNG decoder. This helper converts
six PNG cubemap faces into PPM P6 so the tiny C preview demo can consume them.
It is not part of the runtime library.
"""
import argparse
from pathlib import Path
from PIL import Image

FACE_NAMES = ["px", "nx", "py", "ny", "pz", "nz"]

def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("src_dir", help="directory containing px/nx/py/ny/pz/nz PNG files")
    ap.add_argument("out_dir", help="directory to write PPM P6 faces")
    args = ap.parse_args()
    src = Path(args.src_dir)
    out = Path(args.out_dir)
    out.mkdir(parents=True, exist_ok=True)
    for face in FACE_NAMES:
        in_path = src / f"{face}.png"
        out_path = out / f"{face}.ppm"
        im = Image.open(in_path).convert("RGB")
        im.save(out_path)
        print(f"{in_path} -> {out_path} {im.size[0]}x{im.size[1]}")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
