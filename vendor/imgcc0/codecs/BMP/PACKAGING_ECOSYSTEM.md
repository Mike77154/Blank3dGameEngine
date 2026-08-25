# BMP packaging ecosystem (1.16.0)


## Portability and preset-driven QA in 1.16.0

- Added shared `CMakePresets.json` entries for Linux/GCC, Linux/Clang, macOS/Clang, and Windows/MSVC.
- Added preset-driven smoke validation and a portability audit target.
- Hardened CMake for Windows shared-library exports and compiler-specific warning levels.
