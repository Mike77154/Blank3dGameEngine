# BMP portability and consumer QA (1.10.0)

This round focuses on **consumer adoption** and **cross-platform build portability**.

## What was added

- `CMakePresets.json` with shared presets for Linux/GCC, Linux/Clang, macOS/Clang, and Windows/MSVC.
- Compiler-aware warning policy in CMake:
  - GCC/Clang: `-Wall -Wextra -Wpedantic` plus errors.
  - MSVC: `/W4 /WX`.
- Windows shared-library support in CMake through `WINDOWS_EXPORT_ALL_SYMBOLS`.
- `BUILD_TESTING` + CTest labels for preset-driven smoke validation.
- `make preset-smoke` and `make portability-audit`.
- CI matrix job for multi-OS smoke coverage.

## Local smoke commands

```bash
make preset-smoke
make portability-audit
```

## Notes

- The Linux presets were validated in this release sandbox.
- The macOS and Windows presets are modeled for CI/runners but were not executed in this Linux-only sandbox.
- Shared-library ABI/export governance from earlier rounds remains in place.
