# Public API (PCX89 / 1.16.0 compatibility line)

The supported installed header is:

```c
#include <pcx/pcx.h>
```

## Stable install surface
- `include/pcx/pcx.h`
- `include/pcx/pcx_export.h`
- `include/pcx/pcx_config89.h`
- shared/static library artifacts
- CMake config package and pkg-config metadata

## Static-storage ownership model
PCX89 does not expose runtime allocation helpers. Image storage is selected with:

- `pcx_image_use_static()` / `pcx_image_use_buffer()` / `pcx_image_release()`
- `pcx_indexed_image_use_static()` / `pcx_indexed_image_use_buffer()` / `pcx_indexed_image_release()`

Encoded output is returned from a compile-time bounded internal store. Its lifetime is controlled by the library and a later encode may reuse that store.

All public byte-count parameters use `pcx_size`, required by the build to be an unsigned 32-bit type.

## Private repository headers
Files such as `pcx_chunk.h`, `pcx_parser.h`, `pcx_decoder.h`, and related implementation headers remain in the source tree for internal compilation and tests, but are not part of the default install tree.
