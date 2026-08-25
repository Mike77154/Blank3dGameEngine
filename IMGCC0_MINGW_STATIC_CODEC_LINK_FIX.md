# imgcc0 MinGW static BMP/PCX linkage fix

## Symptom

MinGW/i686 failed while linking `blank3d.exe` with unresolved symbols such as:

- `_imp__bmp_limits_default`
- `_imp__bmp_parse_memory_with_limits`
- `_imp__bmp_decode_to_rgba32_with_limits`
- `_imp__bmp_error_string`
- `_imp__pcx_image_init`
- `_imp__pcx_decode_limits_default`
- `_imp__pcx_load_memory_ex`
- `_imp__pcx_image_release`
- `_imp__pcx_result_to_string`

## Cause

`vendor/imgcc0` is linked into Blank3D as the static archive
`vendor/imgcc0/build/libimgcc0.a`, but the vendored BMP and PCX public
headers default to `__declspec(dllimport)` on Windows unless their static
consumer definitions are active.

Therefore `src/imgcc0.c` was compiled as though BMP/PCX lived in DLLs while
the actual BMP/PCX implementations were embedded in the same static archive.
MinGW encoded the calls as import-address-table references (`_imp__...`),
which cannot be satisfied by normal static symbols inside `libimgcc0.a`.

## Fix

`vendor/imgcc0/Makefile` now compiles every imgcc0 object with:

```make
IMGCC0_STATIC_DEFS = -DBMP_STATIC_DEFINE -DPCX_STATIC_DEFINE
```

The object pattern rule applies these definitions explicitly. The object rule
also depends on the imgcc0 Makefile so changes to static ABI policy invalidate
old objects.

Blank3D's root Makefile now adds `bmp-pcx-static-v1` to the imgcc0 toolchain
stamp. Existing `vendor/imgcc0/build` directories made by an older Blank3D
build are therefore deleted and regenerated automatically on the next build.

## Validation

Performed on the integrated source tree:

- imgcc0 static archive build: PASS
- imgcc0 demo link against `libimgcc0.a`: PASS
- simulated `_WIN32` compile of `src/imgcc0.c`: no BMP/PCX `_imp__` references
- `test-image-stack`: PASS
- `test-muzzle-image-pipeline`: PASS
- `test-sprite-runtime89`: PASS

The fix changes only linkage/visibility policy. No BMP/PCX decoding algorithm,
asset routing behavior, sprite semantics, or image data path was changed.
