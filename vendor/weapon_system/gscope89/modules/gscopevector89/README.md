# gscopevector89

Pure vector reticle authoring layer over `gscopepaint89`.

Supported geometry includes lines, horizontal/vertical lines, rectangles, squares,
triangles, circles, ellipses, arcs, dots, crosses, diagonal crosses, chevrons,
diamonds, parentheses, brackets, horseshoes, regular polygons, arbitrary paths,
grids, tick strips, quadratic curves and built-in stroke glyphs (digits, signs and common reticle labels).

Every shape chooses a semantic part. Each part has independently configurable:

- RGBA color
- outline RGBA
- line thickness
- outline thickness
- layer
- blend mode
- visibility

Custom reticles use caller-owned fixed arrays through `gsv89_builder`; no allocation occurs.
Filled arbitrary paths are triangulated as a fan and should therefore be convex.

Preset labels are decomposed into line/circle commands; the vector path never requires a raster font.

## Build

`make` builds `libgscopevector89.a` using the vendored painter header. The runnable demo is available as `make demo` when this directory is inside the complete five-library bundle.
