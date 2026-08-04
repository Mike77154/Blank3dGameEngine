# png89

Standalone PNG decoder frontend for C89 projects.

## Goals
- no heap
- no floats
- caller-owned buffers
- parser and pixel reconstruction kept separate from compression backend
- suitable for embedding into retro/strict codebases

## What is inside
- PNG signature validation
- chunk walker
- CRC32 validation
- IHDR / PLTE / tRNS / IDAT / IEND parsing
- scanline unfiltering
- Adam7 reconstruction
- RGBA8888 output

## What is intentionally external
The zlib/deflate step is supplied by the host through `Png89InflateHooks`.

## Scratch requirements
Caller must provide:
- `idat_data` buffer large enough for concatenated IDAT payload
- `inflate_out` buffer large enough for decompressed filtered scanlines
- `prev_row`, `cur_row`, `pass_row` line buffers sized for the image/pass
