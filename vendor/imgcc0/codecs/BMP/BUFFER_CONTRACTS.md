# Buffer contracts

Round 1.16.0 adds buffer-direction and byte-count contracts to the installed BMP API.

## What is covered

- GCC-only `access(...)` annotations on byte-buffer entry points such as parse, format, encode, and in-place filters.
- MSVC SAL wrappers for byte-counted input/output buffers and output-pointer contracts.
- Clang C++ `lifetimebound` on APIs that return views into caller-owned storage (`bmp_stream_peek`).
- Stronger compiler warning policy to surface constant-size overreads and write overruns during compilation.
