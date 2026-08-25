# PCX 1.7.0 industrial upgrades

Highlights in this build:

- security limits with defaults for input size, dimensions, pixels, decoded bytes and scanline working set
- new runtime version number API: `pcx_version_number()`
- new human-readable error descriptions: `pcx_result_description()`
- install / uninstall / pkg-config support
- `SECURITY.md` with deployment guidance

Example:

```c
PCXDecodeLimits limits;
pcx_decode_limits_default(&limits);
limits.maxPixels = 32U * 1024U * 1024U;
limits.maxDecodedBytes = 256U * 1024U * 1024U;
pcx_set_global_decode_limits(&limits);
```

Reset to defaults:

```c
pcx_set_global_decode_limits(NULL);
```

## New in 1.2.0

- per-call limit-aware load/inspect entry points for thread-friendlier embedding
- benchmark tool (`make benchmark`)
- CI workflow for GCC/Clang
- indexed path now enforces parsed limits consistently


## New in 1.7.0

- versioned shared libraries with a stable SONAME
- linker-managed export allowlist for the shared ABI
- `make abi-check` to verify exports and SONAME
- first-class CMake package export (`find_package`)
- sanitizer convenience targets and `make cmake-smoke`
