# GlyphWife C89 Arena FontCore v0.2

A small no-heap C89 fontmaking/rendering core for embedded/retro/game-engine use.

## Hard contract

- C89
- no `malloc`
- no `free`
- no `realloc`
- no direct heap allocation by the library
- arena-only scratch memory
- no `float`
- no `double`
- fixed 26.6 coordinates
- manual glyph builder API
- closed contours
- lines + quadratic Bezier outlines
- 1-bit static bitmap rasterizer
- static atlas packer
- SVG glyph writer
- minimal SVG path subset importer/drawer: `M L Q Z`, integer coordinates
- internal `.gff` binary export/import
- mini hinting VM
- OpenType-like static shaping: ligature + kerning table
- FreeType-like facade: face/load/render slot API
- HarfBuzz-like facade: buffer + shape API over the static shaping plan
- text layout/run renderer using shaped glyph items
- glyph transform helpers: copy, translate, scale, embolden

## Explicit non-goals in this package

This is **not** a full HarfBuzz clone, full FreeType clone, full OpenType engine, or a real TrueType instruction interpreter.

It intentionally has:

- no TTF decoding
- no OTF decoding
- no WOFF decoding
- no CFF/CFF2 decoder
- no real HarfBuzz Unicode/script machine
- no real FreeType autohinter
- no full TrueType hint bytecode VM

The compatibility layers are stable architectural facades so the engine can grow toward those APIs without tying the runtime to external libraries.

## v0.2 upgrade over v0.1

- Added `gf_transform.*`
  - copy glyph
  - translate glyph
  - scale XY
  - simple embolden helper
- Added `gf_layout.*`
  - shaped text run
  - pen positions
  - measure width
  - render shaped text to static 1-bit bitmap
- Added `gf_svg_path.*`
  - no-heap SVG path subset importer
  - supports integer `M/m`, `L/l`, `Q/q`, `Z/z`
- Extended arena API
  - mark/pop
  - push zero
  - remaining bytes
- Added bitmap OR blitter
- Demo now exercises:
  - manual glyphs
  - SVG path to glyph
  - shaping ligature `fi`
  - kerning `AV`
  - FreeType-like load/render
  - HarfBuzz-like buffer/shape
  - hint VM rounding
  - text rendering
  - GFF write/read roundtrip

## Build

```sh
make
make run
```

On MSYS2 MinGW:

```sh
gcc -std=c89 -Wall -Wextra -pedantic -O2 -o demo_glyphwife.exe src/*.c demo/main.c
./demo_glyphwife.exe
```

Windows `.bat` helper:

```bat
build_mingw.bat
```

## Generated demo outputs

After `make run`:

- `out_A.pbm`
- `out_O.pbm`
- `out_O.svg`
- `out_V_hinted.svg`
- `out_A_embolden.svg`
- `out_S_from_svg_path.svg`
- `out_text_AVfiS.pbm`
- `out_atlas.pbm`
- `demo_font.gff`

## Module map

```txt
src/gf_fixed.h             fixed 26.6 helpers
src/gf_arena.*             static arena scratch memory
src/gf_outline.*           glyph/font structures + builder API
src/gf_transform.*         copy/translate/scale/embolden helpers
src/gf_raster.*            1-bit rasterizer
src/gf_bitmap.*            bitmap + atlas + OR blit
src/gf_svg.*               SVG glyph path writer/exporter
src/gf_svg_path.*          minimal SVG path subset importer
src/gf_gff.*               internal binary font export/import
src/gf_shape.*             static GSUB/GPOS-like shaping plan
src/gf_layout.*            shaped text run + text bitmap render
src/gf_hb_compat.*         HarfBuzz-like facade
src/gf_freetype_compat.*   FreeType-like facade
src/gf_hintvm.*            tiny hint VM
src/gf_fontmake.*          demo glyphs
```

## Suggested next upgrades

1. 4x supersampling grayscale coverage using static buffers.
2. Cubic Bezier support for CFF-like authoring.
3. Better winding/fill modes: even-odd and non-zero selectable.
4. Textual `.gfw` fontmaking DSL compiler to `.gff`.
5. Script/lang feature tables for the static shaper.
6. Optional TTF subset writer, still no decoder.
