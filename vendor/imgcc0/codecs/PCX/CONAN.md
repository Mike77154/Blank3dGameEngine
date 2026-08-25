# Conan 2 integration for PCX

This repository now ships a Conan 2 recipe at `conanfile.py` plus a validating consumer in `test_package/`.

## Typical flow

```bash
conan create . --build=missing
```

## What the recipe does

- reads the version from `VERSION`
- configures the upstream CMake project
- disables project-local tests/tools for package builds
- installs the library, headers, CMake config files and pkg-config metadata
- exposes `pcx::pcx` to CMakeDeps consumers

## Validation

The `test_package/` folder is a small external consumer. It uses `find_package(pcx CONFIG REQUIRED)` and links against `pcx::pcx`.
