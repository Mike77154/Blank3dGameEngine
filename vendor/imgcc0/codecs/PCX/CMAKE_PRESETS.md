# CMake presets for PCX

This project ships a shared `CMakePresets.json` for reproducible configure/build/test flows.

## Available configure presets

- `linux-gcc-release`
- `linux-clang-release`
- `macos-clang-release`
- `windows-msvc-release`

## Typical local workflow

```bash
cmake --preset linux-gcc-release
cmake --build --preset linux-gcc-release
ctest --preset linux-gcc-release
```

The same preset names are used by CI to reduce drift between local builds and runner builds.
