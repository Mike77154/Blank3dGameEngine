# Architecture

GlyphWifeTTF remains a small static-buffer TrueType core.

## Decode path

```txt
GWT_Font
  tables
    head/maxp/hhea/hmtx/cmap/loca/glyf/name/kern
  -> codepoint_to_glyph
  -> glyph metrics
  -> decode glyph outline
  -> raster or convert to segments
```

## Render path

```txt
GWT_Outline
  -> gwt_rasterize_outline_aa
  -> GWT_Atlas
  -> GWT_LayoutGlyph[]
  -> GWT_TextQuad[]
  -> renderer
```

## Fixed point

Coordinates and transforms use 16.16 fixed point.

The code avoids `float`/`double`. Multiplication and division use integer math. Some internal expressions use `long long` to keep fixed-point multiplication stable on common C89-capable compilers.

## v0.4.0 focus

v0.4.0 focuses on game-engine consumption rather than deeper font-format coverage.

The big additions are:

- text quads,
- bitmap drawing,
- atlas reset,
- UTF-8 preloading,
- text metrics.
