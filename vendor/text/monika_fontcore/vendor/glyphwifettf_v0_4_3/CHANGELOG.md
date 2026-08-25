# Changelog

## v0.4.3

- Added callback-based SVG export helpers.
- Added `GWT_SvgOptions` and `GWT_SvgWriteFn`.
- Added `gwt_svg_write_path_d` for exporting existing outlines as SVG path `d` data.
- Added `gwt_svg_write_glyph` for full standalone glyph SVG export.
- Added `examples/ttf_svg_dump_plus.c`.
- Added optional metrics overlays and control-point overlays for debugging TTF quadratic outlines.


## v0.4.0

- Added atlas reset helper.
- Added UTF-8 driven atlas preloading.
- Added text quad generation from layout glyphs.
- Added boxed text quad generation helper.
- Added text metrics struct and helper.
- Added alpha blitter for 8-bit bitmap targets.
- Added bitmap text draw helper using atlas + layout.
- Added `ttf_text_bitmap.c` example.
- Updated Makefile to build the new text bitmap demo.
- Updated docs for engine integration.

## v0.3.0

- Added error strings.
- Added basic `name` table extraction.
- Added anti-aliased raster path.
- Added atlas ASCII/range helpers.
- Added UTF-8 measure and box layout helpers.

## v0.2.0

- Added compound glyph support.
- Added fixed-point transforms.
- Added UTF-8 scanner and basic layout.
- Added static atlas builder.

## v0.1.0

- Initial SFNT/TTF parser.
- Basic table decode.
- Simple glyph outlines.
- SVG/PGM examples.
