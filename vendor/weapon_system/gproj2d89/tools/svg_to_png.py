#!/usr/bin/env python3
from pathlib import Path
import sys
import cairosvg

src = Path(sys.argv[1] if len(sys.argv) > 1 else "preview/gproj2d89_preview.svg")
dst = Path(sys.argv[2] if len(sys.argv) > 2 else "preview/gproj2d89_preview.png")
cairosvg.svg2png(url=str(src), write_to=str(dst), output_width=1600, output_height=1080)
print(f"wrote {dst}")
