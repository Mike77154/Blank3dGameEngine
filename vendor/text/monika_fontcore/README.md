# Monika FontCore Grand Decoder v0.1.0

C89 facade that vendors the uploaded font decoders **complete and untrimmed** under `vendor/`. The original uploaded ZIPs are also kept under `original_zips/` for byte-level reference.

## Core rules

The new facade in `src/monika_fontcore.c` follows:

- no `malloc`
- no `free`
- no `realloc`
- no `calloc`
- no `float`
- no `double`
- no file I/O inside the core facade
- all big buffers are caller-owned through `MFC_Scratch`

The vendored packages are preserved as received. Some vendor demo/tools may use `stdio` because they are command-line utilities, but the facade API itself is memory-buffer based.

## Vendored decoders

```text
vendor/sfnt_decoder_c89/                 SFNT, TTF tables, cmap, glyf, hmtx, kern
vendor/glyphwifettf_v0_4_3/              TTF glyph decode, layout, raster, atlas
vendor/monika_otf_decoder_c89_interp/    OTF/CFF/CFF2, HVAR, VVAR, MVAR
vendor/monika_woff1_decoder_c89/         WOFF1 -> SFNT
vendor/monika_woff2_c89_decoder/         WOFF2 -> SFNT, hmtx/glyf/loca transforms
vendor/eotdec_c89/                       EOT/MTX reference decoder tools
vendor/gmyy_spritefont_decoder_v3/       GameMaker sprite-font .yy + GML call
vendor/mugen_fnt_codec/                  M.U.G.E.N FNT codec
vendor/giffy_hb_c89_v0_20/               static shaping/layout mini-HarfBuzz style
vendor/glyphwife_c89_arena/              original arena fontcore package, preserved
```

## Supported path through the facade

```text
TTF/SFNT  -> sfnt + glyphwifettf
OTF/CFF   -> sfnt + monika_otf
WOFF1     -> woff1 -> sfnt -> TTF/OTF
WOFF2     -> woff2 -> sfnt -> TTF/OTF
EOT       -> raw SFNT scan bridge, plus full vendor reference tools
GMYY      -> sprite .yy + font_add_sprite_ext bridge
MUGEN     -> text FNT + binary header bridge
GIFFY_HB  -> static-font shaping bridge
```

## Build

```sh
make clean
make
make proof          # no external Brotli link; WOFF2 uses stub/copy callback
make proof-brotli   # links -lbrotlidec -lbrotlicommon and tests real WOFF2 Brotli
```

`make proof` or `make proof-brotli` generates:

```text
proof_fontcore.png
proof_output.txt
```

The PNG is written by the demo using a tiny no-compression PNG writer and a real glyph rasterized from `vendor/monika_woff2_c89_decoder/tests/tiny.ttf`.

## Minimal API

```c
#include "monika_fontcore.h"

static unsigned char sfnt_buf[2 * 1024 * 1024];
static unsigned char work_buf[2 * 1024 * 1024];

MFC_Font font;
MFC_Scratch scratch;

mfc_scratch_init(&scratch, sfnt_buf, sizeof(sfnt_buf), work_buf, sizeof(work_buf));
r = mfc_open_memory(&font, bytes, size, MFC_KIND_UNKNOWN, &scratch);
```

## Notes

- WOFF2 uses a callback. The default target uses the vendored stub/copy callback. The `proof-brotli` target uses the vendored fixed-arena Google Brotli adapter and links against system Brotli libraries.
- EOT has a memory-safe raw SFNT scan bridge in the facade; the full EOT/MTX tool package is kept under `vendor/eotdec_c89/`.
- Complex universal shaping is not claimed as complete HarfBuzz. `giffy_hb` is vendored and bridged for static packed fonts.
