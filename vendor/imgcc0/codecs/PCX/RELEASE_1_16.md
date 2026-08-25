# Release 1.16.0

## Highlights

- Added GCC `access(...)` buffer annotations to key public APIs.
- Added MSVC SAL wrappers for byte-counted input/output buffers and output-pointer contracts.
- Added C++-only `lifetimebound` scaffolding in the export header for future pointer-view APIs.
- Tightened warning policy for GCC/Clang and added `/analyze` scaffolding for MSVC through CMake.
- Added negative compile tests that prove buffer overread/overflow diagnostics trigger when contracts are violated.
