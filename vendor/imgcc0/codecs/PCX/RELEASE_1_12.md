# Release 1.12.0

## Focus

- explicit export/deprecation header for the installed API
- hidden-by-default shared-library visibility on GCC/Clang
- removal of implicit Windows export-all behavior in favor of declared API symbols
- visibility audit tooling and install-surface validation

## Key changes

- added `include/pcx/pcx_export.h`
- annotated public PCX API declarations with `PCX_EXPORT`
- CMake now defines `PCX_BUILDING_LIBRARY` for shared builds and `PCX_STATIC_DEFINE` for static builds
- Makefile shared builds now use `-fvisibility=hidden` and explicit build/static defines
- install tree now includes `pcx_export.h`
- new audit target: `make visibility-audit`
