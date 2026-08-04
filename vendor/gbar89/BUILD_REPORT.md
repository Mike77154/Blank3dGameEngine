# GBAR89 v0.4 build report

Validated with:

```sh
cc -std=c89 -Wall -Wextra -pedantic
```

Targets compiled:

- `examples/ascii_demo.c`
- `examples/v02_feature_demo.c`
- `examples/super_preview_ppm.c`
- `examples/radial_vertical_preview_ppm.c`
- `tests/smoke_v02.c`
- `tests/smoke_v03.c`
- `tests/smoke_v04.c`

All smoke tests completed successfully.

The implementation under `include/` and `src/` contains no calls or declarations using
`malloc`, `calloc`, `realloc`, `free`, `float`, or `double`. Those words appear only in the
constraint comment in the public header.

Preview files were rendered by `examples/radial_vertical_preview_ppm.c` through GBAR89's
actual callback interface and converted from its generated PPM frames to PNG/GIF for viewing.
