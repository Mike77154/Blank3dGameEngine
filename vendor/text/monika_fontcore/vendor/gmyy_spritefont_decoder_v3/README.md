# gmyy_spritefont_decoder_v3

C89 decoder for **GameMaker-style sprite based `.yy` fonts**.

This module intentionally does **not** decode TTF/OTF. It follows the practical GameMaker route for sprite fonts:

```gml
global.fnt = font_add_sprite_ext(spr_font, "012345", true, 1);
// or
global.fnt = font_add_sprite(spr_font, ord("!"), true, 1);
```

GameMaker's sprite-font functions use a sprite strip/subimages where each subimage is a glyph. `font_add_sprite_ext` maps subimages through a string map; `font_add_sprite` starts from a first codepoint.

## Rules kept

- C89
- no `malloc`
- no `free`
- no `realloc`
- no heap ownership
- no `float`
- no `double`
- fixed point 16.16 for glyph advances
- caller-owned buffers only

## New in v3

Alpha / visible-pixel scanner for proportional fonts:

- `gmyy_scan_rgba_bounds()` for decoded RGBA buffers
- `gmyy_scan_bmp_bounds()` for uncompressed BMP 24/32-bit
- `gmyy_scan_tga_bounds()` for uncompressed TGA 8/24/32-bit
- `gmyy_scan_image_bounds_by_ext()` autodetects BMP/TGA by extension/magic
- `gmyy_font_apply_proportional_rgba_provider()` applies bounds to every glyph via a no-heap RGBA provider callback

## PNG note

PNG pixels are normally compressed with zlib/DEFLATE. A full PNG decoder would either be large or need an inflate implementation. To preserve the no-heap C89 contract, v3 accepts PNG through `GMYY_RGBAProvider`: your engine's existing no-heap image loader decodes PNG into a caller-provided static RGBA scratch buffer, then this module scans alpha.

That keeps this module focused: it measures visible glyph bounds like `prop=true`, but does not own image decompression.

## Build

```sh
gcc -std=c89 -Wall -Wextra -pedantic demo.c gmyy_spritefont.c -o demo
./demo
```

Expected output includes smaller advances after alpha scanning:

```text
before scan:
 glyph 0 cp=48 advance=9
 glyph 1 cp=49 advance=9
 glyph 2 cp=50 advance=9
 glyph 3 cp=51 advance=9

after scan: applied proportional alpha scan warnings=0
 glyph 0 ... w=8 adv=9
 glyph 1 ... w=6 adv=7
 glyph 2 ... w=4 adv=5
 glyph 3 ... w=2 adv=3
```

## Typical pipeline

```text
GMSprite .yy
   +
GML font_add_sprite_ext(...)
   ↓
gmyy_decode_sprite_yy
gmyy_decode_font_call_gml
gmyy_build_spritefont
   ↓
optional prop=true scan:
  gmyy_font_apply_proportional_rgba_provider
   ↓
codepoint -> frame -> trim bounds -> xadvance fixed 16.16
```

## Limits

This is a source-format parser / builder. It does not parse GameMaker's compiled runner output or texture pages. It intentionally avoids vector font assets and compiled TTF/OTF data.
