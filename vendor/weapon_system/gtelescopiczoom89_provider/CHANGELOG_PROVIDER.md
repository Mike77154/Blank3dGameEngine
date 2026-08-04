# Provider edition changes

- Added `GTZ89_ABI_VERSION 2`.
- Added internal, external-camera, external-math and combined modes.
- Added zero-copy binding to an externally owned `g89_camera`.
- Added callback adapters for camera systems with a different native type.
- Added optional external FOV solver and integer interpolation callbacks.
- Added per-operation fallback to the original internal fixed-point math.
- Camera transform vectors remain externally owned and are never modified.
- Added provider example and smoke tests covering all modes.

Compatibility: existing function calls remain source-compatible, but `gtz89_ctx`
was extended, so binaries compiled against the previous structure must be rebuilt.
