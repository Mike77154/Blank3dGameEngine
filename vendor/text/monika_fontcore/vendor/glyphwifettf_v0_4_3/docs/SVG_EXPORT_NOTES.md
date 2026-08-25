# GlyphWifeTTF SVG export notes

v0.4.3 adds callback-based SVG helpers for inspecting and debugging TrueType outlines without heap ownership.

## Why SVG `Q` commands?

TrueType glyf outlines use on-curve and off-curve points that describe quadratic Bezier segments. SVG path data supports quadratic Bezier curves directly with the `Q cx cy x y` command, so GlyphWifeTTF can export contours without converting to cubic curves.

## Coordinate system

TrueType font units use Y-up coordinates. SVG uses a Y-down viewport in normal screen display. The helper option `flip_y=1` exports glyphs in a screen-friendly orientation by negating Y values.

## Debug layers

`include_metrics=1` writes:

- a baseline guide
- a glyph bounding box guide
- metadata with advance, left side bearing, bbox and unitsPerEm

`include_control_points=1` writes control point circles:

- green = on-curve point
- red = off-curve quadratic control point

## Minimal usage

```c
GWT_SvgOptions opt;
opt.padding_units = 80;
opt.flip_y = 1;
opt.include_metrics = 1;
opt.include_control_points = 1;
opt.fill = "black";
opt.stroke = "none";

gwt_svg_write_glyph(&font, 'A', &outline, &segments, my_writer, my_user, &opt);
```

The writer callback receives small chunks of text and returns 0 on success.
