# Public API (1.16.0)

The supported installed header is:

```c
#include <bmp/bmp.h>
```

## Stable install surface
- `include/bmp/bmp.h`
- shared/static library artifacts
- CMake config package and pkg-config metadata

## Private repository headers
Files such as `bmp_types.h`, `bmp_metadata.h`, `bmp_parser.h`, and related implementation headers remain in the source tree for internal compilation and tests, but are no longer part of the default install tree.

## Contract-annotation helpers

- `bmp_malloc(size_t)` / `bmp_calloc(size_t, size_t)` allocator helpers with public contract annotations.
