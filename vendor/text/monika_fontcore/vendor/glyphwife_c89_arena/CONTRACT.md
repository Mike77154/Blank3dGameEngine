# GlyphWife runtime contract

GlyphWife is designed for environments where deterministic memory matters more than full desktop typography coverage.

The runtime does not allocate heap memory. Scratch memory is provided through `GF_Arena`, which is a fixed-size struct containing a static byte buffer. The code avoids floating point and uses 26.6 fixed point for coordinates and transforms.

The project deliberately separates:

1. Fontmaking core
2. Internal glyph representation
3. Rasterization
4. Static shaping
5. Compatibility facades
6. Format decoding

This package implements 1-5. Format decoding is intentionally excluded.
