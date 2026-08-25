# BMP 1.7.0 industrial upgrades

Highlights in this build:

- security limits with defaults for input size, dimensions, pixels, decoded RGBA size, palette size, embedded payload size and ICC profile size
- new runtime version number API: `bmp_version_number()`
- new human-readable error descriptions: `bmp_error_description()`
- new RGBA size helper: `bmp_calc_rgba32_buffer_size()`
- install / uninstall / pkg-config support
- `SECURITY.md` with deployment guidance

Example:

```c
bmp_limits limits;
bmp_limits_default(&limits);
limits.max_pixels = 32ULL * 1024ULL * 1024ULL;
limits.max_decoded_bytes = 256ULL * 1024ULL * 1024ULL;
bmp_set_global_limits(&limits);
```

Reset to defaults:

```c
bmp_set_global_limits(NULL);
```

## New in 1.2.0

- per-call limit-aware parse/decode entry points for thread-friendlier embedding
- decode path checks decoded-size limits before writing into caller-owned RGBA storage
- benchmark tool (`make benchmark`)
- CI workflow for GCC/Clang


## New in 1.7.0

- versioned shared libraries with a stable SONAME
- linker-managed export allowlist for the shared ABI
- `make abi-check` to verify exports and SONAME
- first-class CMake package export (`find_package`)
- sanitizer convenience targets and `make cmake-smoke`
