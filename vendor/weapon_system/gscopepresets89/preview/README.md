# Code-generated preview

This preview does **not** use a generative image renderer. The executable calls
`gsvp89_emit()` for every preset, consumes the resulting vector commands through
a small fixed-point C89 raster backend, and writes one binary PPM image per
preset.

Properties of the preview backend:

- C89 (`-std=c89 -pedantic`)
- no `malloc`, `realloc`, `free`, or heap storage
- no `float` or `double`
- fixed-size static RGB framebuffer
- integer/fixed-point geometry and integer trigonometry table

Build and generate the 59 PPM files:

```sh
make preview_ppm
```

Optional PNG conversion with ImageMagick:

```sh
magick generated_preview/38_svd_pso1_dragunov.ppm dragunov.png
```

The included `gscopepresets89_catalog_59.png` and
`gscopepresets89_highlights.png` were produced from those generated PPM files.
