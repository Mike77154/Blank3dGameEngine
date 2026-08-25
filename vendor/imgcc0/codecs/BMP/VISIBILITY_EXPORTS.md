# Visibility and export policy

This release adds an explicit installed export header (`<bmp/bmp_export.h>`) and annotates the stable public API with `BMP_EXPORT`. Shared-library builds now default to hidden visibility on GCC/Clang and rely on explicit exports for the public ABI.

## Macros

- `BMP_EXPORT`: public API symbol
- `BMP_NO_EXPORT`: internal symbol marker
- `BMP_DEPRECATED`: compiler deprecation attribute
- `BMP_DEPRECATED_EXPORT`: exported + deprecated symbol
- `BMP_DEPRECATED_NO_EXPORT`: internal + deprecated symbol
- `BMP_STATIC_DEFINE`: disable import/export decorations for static linkage

## Notes

- Installed consumers should continue including `<bmp/bmp.h>`.
- Windows static consumers may define `BMP_STATIC_DEFINE` when linking the static archive directly outside of CMake package metadata.
