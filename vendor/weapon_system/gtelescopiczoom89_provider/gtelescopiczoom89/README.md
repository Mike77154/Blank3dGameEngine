# gtelescopiczoom89 — provider edition

Optical zoom/FOV transitions and scoped sensitivity in strict C89.

## Modes

- `GTZ89_PROVIDER_INTERNAL`: original self-contained behavior.
- `GTZ89_PROVIDER_CAMERA`: an external camera owns position/orientation and receives FOV output.
- `GTZ89_PROVIDER_MATH`: an external system may replace FOV solving and/or interpolation.
- `GTZ89_PROVIDER_CAMERA_AND_MATH`: both providers at once.

The provider is copied into `gtz89_ctx`; pointers inside it are borrowed. No heap ownership is introduced.

## External camera binding

```c
gtz89_provider provider;
gtz89_provider_init(&provider);
provider.mode = GTZ89_PROVIDER_CAMERA;
provider.camera = &engine_camera;
gtz89_set_provider(&zoom, &provider);
```

Only `base_fov_deg_x100` and `current_fov_deg_x100` are read/written. Camera transform fields (`pos`, `forward`, `right`, `up`) remain entirely owned by the external engine/math system.

Callbacks are also available for engines whose camera is not represented by `g89_camera`.

## External math binding

Set `provider.solve_fov` and/or `provider.lerp_short`, then enable `GTZ89_PROVIDER_MATH`. A callback returns non-zero when it handled the operation. Returning zero invokes the original internal fixed-point fallback.

## Build

```sh
make
```

## Example

```sh
gcc -std=c89 -pedantic -Wall -Wextra \
  -Iinclude -Igtelescopiczoom89/include \
  examples/provider_example.c gtelescopiczoom89/src/gtelescopiczoom89.c \
  -o provider_example
./provider_example
```

Restrictions: strict C89, Q16.16/integer runtime, no malloc/realloc/free, no float/double, no heap ownership.

## Compatibility note

The old API remains source-compatible, but `gtz89_ctx` now contains provider state (`GTZ89_ABI_VERSION 2`). Recompile code that embedded the previous context structure; this is not binary ABI-compatible with the original archive.
