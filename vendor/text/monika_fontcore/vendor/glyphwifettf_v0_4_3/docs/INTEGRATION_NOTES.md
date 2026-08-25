# Integration notes

GlyphWifeTTF is split into three practical layers:

```txt
TTF bytes
  -> GWT_Font parser
  -> glyph outlines / metrics
  -> atlas + layout + quads
  -> your renderer
```

## For OpenGL-style backends

1. Allocate an 8-bit atlas buffer.
2. Call `gwt_atlas_init`.
3. Preload ASCII or exact text ranges with `gwt_atlas_add_ascii` / `gwt_atlas_add_utf8`.
4. Upload atlas pixels as an alpha/luminance texture.
5. Use `gwt_layout_utf8` or `gwt_layout_box_utf8`.
6. Convert layout glyphs into `GWT_TextQuad` with `gwt_build_text_quads_utf8`.
7. Render each quad using atlas UVs.

`GWT_TextQuad` gives destination pixel rectangle and atlas source rectangle. The engine owns final UV conversion.

## For software/debug backends

Use `gwt_draw_text_bitmap_utf8` directly. It draws into an 8-bit destination bitmap.

This is useful for:

- screenshots/debug windows,
- software renderer tests,
- headless text tests,
- editor previews.

## Memory ownership

The core never allocates memory. The caller owns:

- font byte buffer,
- outline points,
- contour buffer,
- atlas pixels,
- atlas glyph array,
- layout scratch,
- quad output buffer,
- final draw target.

## Recommended engine modules

```txt
lumi/gmenu/bvh/editor
       |
       v
GlyphWifeTTF atlas/layout/quads
       |
       v
rendimental/grx/OpenGL/software backend
```
