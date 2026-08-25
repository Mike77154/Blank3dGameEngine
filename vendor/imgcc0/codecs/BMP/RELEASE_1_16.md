# Release 1.16.0

## Highlights

- Added GCC `access(...)` buffer annotations to the installed public API.
- Added MSVC SAL buffer annotations and output-pointer contracts.
- Added Clang C++ `lifetimebound` on `bmp_stream_peek()`.
- Added negative compile tests proving that GCC diagnoses compile-time buffer contract violations.
- Added `/analyze` scaffolding for MSVC through CMake.
