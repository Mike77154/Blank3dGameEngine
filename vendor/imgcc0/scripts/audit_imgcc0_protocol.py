from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
FILES = [
    ROOT / "include" / "imgcc0.h",
    ROOT / "src" / "imgcc0.c",
    ROOT / "src" / "imgcc0_zragf_bridge.c",
    ROOT / "src" / "imgcc0_zragf_bridge.h",
    ROOT / "demos" / "demo_cli.c",
    ROOT / "codecs" / "JPEG" / "src" / "c89jpeg.c",
    ROOT / "codecs" / "TGA" / "gafnyf_tga.c",
]
for rel in (
    "codecs/GIF/src",
    "codecs/QOI/src",
    "codecs/WEBP/src/dec",
    "codecs/WEBP/src/demux",
    "codecs/WEBP/src/utils",
    "codecs/TIFF/src",
    "codecs/DDS/src",
    "codecs/BMP/bmp",
):
    FILES.extend(sorted((ROOT / rel).glob("*.c")))

for name in (
    "png_mem89.c", "png_fixed89.c", "png_crc.c", "png_filters.c",
    "png_chunks.c", "png_parser.c", "png_render.c", "png_stubs.c",
    "png_decoder.c", "png_apng.c", "png_apng_progressive.c",
    "png_encoder.c",
):
    FILES.append(ROOT / "codecs" / "PNG" / name)

for name in (
    "pcx_chunk.c", "pcx_parser.c", "pcx_render.c",
    "pcx_decoder.c", "pcx_encoder.c", "pcx_report.c",
):
    FILES.append(ROOT / "codecs" / "PCX" / name)

for rel in (
    "codecs/ZRAGF",
    "codecs/ZRAGF/zragf_deflate",
    "codecs/ZRAGF/zragf_inflate",
):
    FILES.extend(sorted((ROOT / rel).glob("*.c")))
    FILES.extend(sorted((ROOT / rel).glob("*.h")))

for rel in (
    "codecs/PSD/src",
    "codecs/PSD/include/psd89",
):
    FILES.extend(sorted((ROOT / rel).glob("*.c")))
    FILES.extend(sorted((ROOT / rel).glob("*.h")))

for path in (
    ROOT / "codecs" / "PNG" / "png_decoder.h",
    ROOT / "codecs" / "PNG" / "png_fixed89.h",
    ROOT / "codecs" / "PNG" / "png_mem89.h",
    ROOT / "codecs" / "DDS" / "include" / "giffany_dds" / "gdds.h",
    ROOT / "codecs" / "BMP" / "include" / "bmp" / "bmp.h",
    ROOT / "codecs" / "PCX" / "include" / "pcx" / "pcx.h",
):
    FILES.append(path)

RULES = {
    "dynamic-allocation call": re.compile(r"\b(?:malloc|calloc|realloc|free)\s*\("),
    "floating type": re.compile(r"\b(?:float|double)\b"),
    "explicit 64-bit integer": re.compile(r"\b(?:long\s+long|int64_t|uint64_t|u64|i64)\b"),
    "C99 line comment": re.compile(r"//"),
}

bad = []
for path in FILES:
    text = path.read_text(encoding="utf-8", errors="replace")
    for label, rx in RULES.items():
        for match in rx.finditer(text):
            line = text.count("\n", 0, match.start()) + 1
            bad.append((path.relative_to(ROOT), line, label, match.group(0)))

if bad:
    for item in bad:
        print("FAIL %s:%d: %s: %s" % item)
    sys.exit(1)

print("imgcc0 strict-build protocol audit: PASS")
print("  scanned %d active C/header files" % len(FILES))
print("  no dynamic-allocation calls")
print("  no floating types")
print("  no explicit 64-bit integer types")
print("  no C99 line comments")
