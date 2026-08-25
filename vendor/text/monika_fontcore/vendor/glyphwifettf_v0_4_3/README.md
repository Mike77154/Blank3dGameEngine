# GlyphWifeTTF v0.4.3

Tiny TrueType decoder/raster helper for game engines.

Design rules:

- C89 style API.
- Core owns no heap memory.
- No `malloc`, `realloc`, or `free` in the library.
- No `float` or `double`.
- Caller supplies font data, outline buffers, atlas pixels, glyph tables, layout buffers, and draw buffers.
- Fixed-point math uses 16.16 values.

## Current feature set

- SFNT/TTF table directory parser.
- Required tables: `head`, `maxp`, `hhea`, `hmtx`, `cmap`, `loca`, `glyf`.
- Optional tables: `name`, `kern`.
- `cmap` format 4 and format 12.
- Simple glyph decode.
- Compound glyph decode with fixed-point transforms.
- Glyph metrics and kerning.
- TrueType quadratic outline traversal.
- Anti-aliased rasterization by supersampling.
- Static atlas builder.
- UTF-8 scanner.
- Basic text layout.
- Box layout with wrapping.
- Text metrics helper.
- Text quad generation for renderer backends.
- 8-bit bitmap text draw path for software/debug renderers.
- Callback-based SVG glyph export for outline debugging.
- Optional SVG metadata, baseline/bbox guides, and control-point overlays.

## New in v0.4.3

v0.4.3 keeps the v0.4.0 engine-facing layer and adds SVG export helpers over the lower TTF decoder:

```c
void gwt_atlas_reset(GWT_Atlas *atlas, unsigned char clear_value);
int gwt_atlas_add_utf8(...);
int gwt_build_text_quads_utf8(...);
int gwt_build_text_quads_box_utf8(...);
int gwt_get_text_metrics(...);
int gwt_bitmap_blit_alpha(...);
int gwt_draw_text_bitmap_utf8(...);
int gwt_svg_write_path_d(...);
int gwt_svg_write_glyph(...);
```

This makes the library easier to plug into:

- `lumi` menu widgets.
- `bvh` HUD labels.
- `gmenu` option text.
- debug overlays.
- object/property editor inspectors.
- software renderer experiments.
- OpenGL texture-atlas quads.

## Build

```sh
make
```

The Makefile builds the example tools with:

```sh
gcc -std=c89 -O2 -Wall -Wextra -Iinclude
```

## Example

```sh
./ttf_text_bitmap /path/to/font.ttf "Nueva partida - Opciones - Salir"
```

Outputs:

- `text_bitmap.pgm`
- `text_atlas.pgm`

## Important limits

Still intentionally not a full FreeType/HarfBuzz replacement.

Not implemented yet:

- TrueType hinting VM.
- GPOS/GSUB shaping.
- complex script shaping.
- color fonts.
- CFF outlines.
- variable fonts.
- subpixel LCD rendering.

For the engines, this is meant as a tiny portable text core, not a desktop publishing stack.

## SVG debug example

```sh
make ttf_svg_dump_plus
./ttf_svg_dump_plus /path/to/font.ttf 65 points > glyph_A.svg
```

The optional `points` argument enables on-curve/off-curve debug markers.
