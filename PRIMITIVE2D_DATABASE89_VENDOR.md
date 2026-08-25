# Primitive2DDatabase89 vendorization

`primitive2d_database89 v0.4.0 encyclopedic` is vendored at:

`vendor/primitive2d_database89/`

The upstream package is preserved byte-for-byte, including its tests,
documentation, examples, manifest, SHA256SUMS and the upstream prebuilt
`test_modular_clang` artifact. Blank3D does not execute or link that prebuilt
artifact; QA rebuilds `tests/test_modular.c` from source and then cleans the
newly generated test executable.

Verified catalog:

- 720 shapes
- 57 submodules
- 16,321 emitted commands in the full traversal test
- Q14 fixed-point geometry
- POINT/MOVE/LINE/QUAD/CUBIC/CLOSE provider IR
- no renderer dependency
- no heap/floating-point/explicit 64-bit types in include/src

The root Makefile now exposes `PRIMITIVE2D89_ROOT` and the vendor include path,
and provides `make test-primitive2d89-vendor`.

No HUD/crosshair/editor consumer has been rewritten in this step. The vendor
is deliberately landed first as a stable geometric source. Blank3D's future
bridge should convert Q14 to the engine's common Q16 with an exact left shift
of two bits and route the command IR into a draw provider.
