# Changelog

## 0.4.0

1. Full faux-3D and outlining path for radial pie and ring meters.
2. Radial interpretations of simple, double, bevel, brackets, rail and pixel frames.
3. Segmented/dashed arcs with configurable angular gaps.
4. Butt, round and square radial cap styles.
5. Polar grid, checker, scanline, diagonal and dither backgrounds.
6. Radial gloss, inner shadow, rings, spokes, ticks, pixel cells and sweep highlight.
7. Damage lag, independent middle band and numeric overlay rendered as radial ranges.
8. Radial markers, vector units and layered boss pips.
9. Procedural detail phase for animation.
10. Vertical partial-segment direction fix and orientation-aware gloss.
11. Larger configurable fixed command-buffer defaults for complex procedural meters.
12. New `tests/smoke_v04.c`, `examples/radial_vertical_preview_ppm.c` and `make radial_preview`.

## 0.3.0

Added an engine-agnostic procedural styling layer without changing the C89/no-heap/fixed-point contract:

1. Eight frame kinds: simple, double, outward/inward bevel, brackets, rail, pixel, none.
2. Composable 2D effects: outer outline, drop shadow, extrusion, inner shadow, gloss, fill grid, scanlines, pixel cells.
3. Six procedural backgrounds: solid, grid, checker, scanlines, diagonal, dither.
4. Pixel-quantized fill lengths with configurable cell size.
5. Independent animated middle/ghost value band with its own color and direction.
6. User-owned normalized vector glyphs repeated as filled/empty units.
7. C89 software-rendered showcase and 48-frame animation preview.
8. New `tests/smoke_v03.c` and `make preview` target.


## 0.2.0

Implemented the requested v0.2 roadmap items:

1. Fixed command buffer backend.
2. Real layered/boss bar value mapping.
3. Numeric overlay/shield bars.
4. Visual states with auto low/critical thresholds.
5. Accessible rect patterns.
6. Advanced mask fills: slant, hexagon, diamond, custom slices.
7. Fixed-point easing helpers and integrated value tweening.

Also added:

- `examples/v02_feature_demo.c`
- `tests/smoke_v02.c`
- `make test`

## 0.1.0

Initial C89 backend-agnostic meter renderer:

- linear bars
- damage lag
- smooth value stepping
- markers
- segmented bars
- sprite clip
- nine-slice
- radial pie/ring
- integer trig lookup
