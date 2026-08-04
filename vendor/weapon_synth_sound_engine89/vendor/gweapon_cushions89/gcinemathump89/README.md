# gcinemathump89 1.0.0

Strict C89 procedural audio layer. Fixed-point signal path, caller-owned state, no malloc/calloc/realloc/free, no float/double, and no math.h in the core.

## Build

```sh
make
make test
```

The `include/` header is the public ABI. Copy or vendor the static library, or compile the single source file directly into an engine. Built-in presets are optional: fetch a preset, modify its integer fields, then initialize the caller-owned context.

License: CC0-1.0.
