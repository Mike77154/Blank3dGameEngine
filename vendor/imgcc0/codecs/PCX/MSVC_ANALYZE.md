# MSVC /analyze integration

The CMake build now exposes `PCX_ENABLE_MSVC_ANALYZE`. When enabled on MSVC builds, `/analyze` is added to targets built by CMake.

The installed public header also provides SAL wrappers that expand to native annotations on MSVC and to no-ops elsewhere. This keeps the Unix builds clean while giving Visual Studio static analysis richer buffer and string contracts.
