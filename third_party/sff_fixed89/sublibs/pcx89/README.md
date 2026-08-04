# pcx89

Standalone PCX decoder for C89 projects.

## Goals
- no heap
- no floats
- no hidden I/O
- caller-owned output buffers
- integer-only decoding
- practical support for classic PCX variants

## API
- `pcx89_read_info`
- `pcx89_extract_vga_palette_rgb`
- `pcx89_decode_rgba8888`

## Scratch requirements
Provide one scanline buffer of at least:

`num_planes * bytes_per_line`

bytes.
