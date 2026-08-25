# gscoperaster89

Raster layer description library for user-provided images.

The library never opens PNG/BMP files and never owns textures. The engine maps a file name to an integer `asset_id`; this library only emits sprite commands with normalized placement, UVs, tint, alpha, outline, blend mode, anchoring and optional circular clipping.

This keeps image loading and graphics APIs outside the portable C89 core.

## Build

`make` builds `libgscoperaster89.a` using the vendored painter header. The runnable demo is available as `make demo` inside the complete bundle.
