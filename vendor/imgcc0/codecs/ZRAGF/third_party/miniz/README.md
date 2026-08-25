# Optional miniz vendor drop-in

Phase 68 can optionally add **miniz** to the public benchmark suite.

To enable it with vendored sources, place these files here:

- `third_party/miniz/miniz.c`
- `third_party/miniz/miniz.h`

Then configure CMake with the default option:

```sh
cmake -S . -B build -DZRAGF_BUILD_PUBLIC_BENCH=ON
```

Alternatively set `-DZRAGF_MINIZ_ROOT=/path/to/miniz` if you already have a
miniz install tree or source drop somewhere else.
