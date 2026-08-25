# gafnyf_tga v3.0

Tiny **TGA encoder/decoder** for **C89**.

## Constraints

- no `malloc`
- no floating point
- caller-owned buffers only
- intended for **CC0-1.0** distribution

## New in v3.0

### Retro-friendly 16-bit truecolor helpers

New packed 16-bit memory formats:

```c
TGA_PIXFMT_RGB565
TGA_PIXFMT_ARGB1555
```

New helpers:

```c
tga_u16 tga_pack_rgb565(tga_u8 r, tga_u8 g, tga_u8 b);
tga_u16 tga_pack_argb1555(tga_u8 a, tga_u8 r, tga_u8 g, tga_u8 b);
void tga_unpack_rgb565(tga_u16 pixel, tga_u8 *out_r, tga_u8 *out_g, tga_u8 *out_b);
void tga_unpack_argb1555(tga_u16 pixel,
                         tga_u8 *out_a,
                         tga_u8 *out_r,
                         tga_u8 *out_g,
                         tga_u8 *out_b);
```

These formats work through the regular encode/decode APIs too, so you can now:

- encode from `RGB565` / `ARGB1555`
- decode directly into `RGB565` / `ARGB1555`
- use 16-bit packed formats as indexed-palette source data

Packed 16-bit memory pixels are stored as **little-endian byte pairs**.

### Repair / canonicalize helpers

New helpers to rebuild **TGA 2.0** metadata blocks without re-encoding image pixels:

```c
int tga_canonicalize_size_memory(const void *src,
                                 size_t src_size,
                                 const tga_canonicalize_options *options,
                                 size_t *out_size);
int tga_canonicalize_memory(void *dst,
                            size_t dst_capacity,
                            size_t *dst_size,
                            const void *src,
                            size_t src_size,
                            const tga_canonicalize_options *options);
int tga_repair_tga2_size_memory(const void *src, size_t src_size, size_t *out_size);
int tga_repair_tga2_memory(void *dst,
                           size_t dst_capacity,
                           size_t *dst_size,
                           const void *src,
                           size_t src_size);
```

And `FILE*` variants:

```c
int tga_canonicalize_file(FILE *dst,
                          FILE *src,
                          const tga_canonicalize_options *options);
int tga_repair_tga2_file(FILE *dst, FILE *src);
```

Default repair/canonicalize behavior:

- keeps valid developer payloads and rebuilds the directory
- keeps valid scan-line table / postage stamp / color-correction table
- rewrites a canonical footer
- synthesizes a sane Extension Area when needed
- normalizes `attributes_type` semantically from the image descriptor when appropriate
- preserves image bytes as-is instead of re-encoding them

Useful flags:

```c
TGA_CANONICALIZE_KEEP_EXTENSION
TGA_CANONICALIZE_KEEP_DEVELOPER_AREA
TGA_CANONICALIZE_KEEP_COLOR_CORRECTION
TGA_CANONICALIZE_KEEP_POSTAGE_STAMP
TGA_CANONICALIZE_KEEP_SCAN_LINE_TABLE
TGA_CANONICALIZE_SYNTHESIZE_EXTENSION
TGA_CANONICALIZE_FORCE_FOOTER
TGA_CANONICALIZE_NORMALIZE_EXTENSION
TGA_CANONICALIZE_DEFAULT
```

## Previous features kept

- decode image types `1, 2, 3, 9, 10, 11`
- encode grayscale, indexed, truecolor
- raw and RLE image data
- origin bit handling
- TGA 2.0 footer + extension support
- developer area reader/writer
- scan-line table reader/writer
- postage stamp reader/writer
- color correction table reader/writer
- exact encode-size helpers
- row-range / rectangle decode
- swizzle / channel-mask decode
- indexed colormap helpers
- section listing and general forensics
- extension semantics + patch helpers
- `FILE*` helpers

## Build

```sh
make
./selftest
./example_palette16
./example_forensics
./example_extension
./example_truecolor16
./example_canonicalize
```

## Files in this package

- `gafnyf_tga.h`
- `gafnyf_tga.c`
- `selftest.c`
- `example_palette16.c`
- `example_forensics.c`
- `example_extension.c`
- `example_truecolor16.c`
- `example_canonicalize.c`
- `Makefile`
- `LICENSE.txt`

## License

This package is intended to be released under **CC0-1.0**.
See `LICENSE.txt`.
