# Release 1.12.0

## Focus

- explicit export/deprecation header for the installed API
- hidden-by-default shared-library visibility on GCC/Clang
- removal of implicit Windows export-all behavior in favor of declared API symbols
- visibility audit tooling and install-surface validation

## Key changes

- added `include/bmp/bmp_export.h`
- annotated public BMP API declarations with `BMP_EXPORT`
- CMake now defines `BMP_BUILDING_LIBRARY` for shared builds and `BMP_STATIC_DEFINE` for static builds
- Makefile shared builds now use `-fvisibility=hidden` and explicit build/static defines
- install tree now includes `bmp_export.h`
- new audit target: `make visibility-audit`
