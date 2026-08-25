# PCX packaging ecosystem (1.16.0)

This round focuses on making `pcx` easier to consume through the usual C/C++ distribution paths.

## What is included

- `pkg-config` consumer smoke example in `examples/pkgconfig_consumer/`
- root `conanfile.py` and `test_package/` for Conan 2
- local vcpkg overlay port in `packaging/vcpkg/ports/pcx/`
- CPack binary and source archive generation
- `scripts/audit_packaging.py` to validate version sync and package contents

## Local validation targets

```bash
make ecosystem-audit
make pkgconfig-smoke
make cpack-smoke
```

## Notes

The vcpkg port is intentionally a **local overlay port**. It packages the checked-out repository itself instead of downloading from a public registry URL, which makes it useful for internal CI and release-candidate validation before publication.

The Conan recipe is rooted at the repository top level so `conan create .` has direct access to the canonical CMake build and install rules.


## Portability and preset-driven QA in 1.16.0

- Added shared `CMakePresets.json` entries for Linux/GCC, Linux/Clang, macOS/Clang, and Windows/MSVC.
- Added preset-driven smoke validation and a portability audit target.
- Hardened CMake for Windows shared-library exports and compiler-specific warning levels.
