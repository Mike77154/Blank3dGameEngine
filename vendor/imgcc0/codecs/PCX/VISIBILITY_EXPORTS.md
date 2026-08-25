# Visibility and export policy

This release adds an explicit installed export header (`<pcx/pcx_export.h>`) and annotates the stable public API with `PCX_EXPORT`. Shared-library builds now default to hidden visibility on GCC/Clang and rely on explicit exports for the public ABI.

## Macros

- `PCX_EXPORT`: public API symbol
- `PCX_NO_EXPORT`: internal symbol marker
- `PCX_DEPRECATED`: compiler deprecation attribute
- `PCX_DEPRECATED_EXPORT`: exported + deprecated symbol
- `PCX_DEPRECATED_NO_EXPORT`: internal + deprecated symbol
- `PCX_STATIC_DEFINE`: disable import/export decorations for static linkage

## Notes

- Installed consumers should continue including `<pcx/pcx.h>`.
- Windows static consumers may define `PCX_STATIC_DEFINE` when linking the static archive directly outside of CMake package metadata.
