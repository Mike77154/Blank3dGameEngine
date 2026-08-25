# Monika WOFF2 C89 Decoder — v3 hmtx transform patch

This package is a tiny WOFF2-to-SFNT/TTF/OTF decoder core written in C89 style for fixed-buffer engine/toolchain experiments. It keeps the core free of heap calls: no malloc, no free, no realloc, no calloc, no float, and no double in the project sources.

## v3 additions

- Added real WOFF2 `hmtx` transform version 1 reverse transform.
- Reconstructs missing `lsb[]` values from `glyf` glyph xMin.
- Reconstructs missing monospaced `leftSideBearing[]` values from `glyf` glyph xMin.
- Handles empty glyphs as zero left side bearing.
- Validates hmtx transform flags: bits 0/1 required, bits 2-7 rejected.
- Validates transformed and original `hmtx` lengths from `maxp.numGlyphs` and `hhea.numberOfHMetrics`.
- Added synthetic C89 test for hmtx transform v1.

## Already present from v2

- WOFF2 header and table directory parser.
- UIntBase128 and 255UInt16 readers.
- Brotli callback API plus optional Google Brotli static-arena adapter.
- `glyf` transform version 0 reverse transform for simple/composite glyphs.
- `loca` transform version 0 reconstruction.
- SFNT writer with sorted table records, checksums, and `head.checkSumAdjustment`.

## Build

```sh
make clean
make check
```

With system Brotli development libraries installed:

```sh
make check-brotli
```

## CLI

```sh
make w2f_cli_static_brotli
./w2f_cli_static_brotli input.woff2 output.ttf
```

## Limits

Edit `include/w2f_config.h` for the fixed caps used by the examples and adapters. The library expects caller-owned buffers.

## Transform coverage

| Table | Transform | Status |
|---|---:|---|
| `glyf` | 0 | implemented |
| `glyf` | 3/null | implemented |
| `loca` | 0 | implemented |
| `loca` | 3/null | implemented |
| `hmtx` | 0/null | implemented |
| `hmtx` | 1 | implemented in v3 |

## Important notes

WOFF2 fonts use Brotli for the compressed font data block. The core takes a Brotli callback so the decoder can remain allocator-policy-neutral. The optional `src/w2f_brotli_google_static.c` adapter routes Brotli allocations through one fixed arena.

