# MSVC /analyze integration

The CMake build now exposes `BMP_ENABLE_MSVC_ANALYZE`. When enabled on MSVC builds, `/analyze` is added to targets built by CMake.

The installed public header also maps SAL wrappers to native annotations on MSVC while keeping them as no-ops on non-MSVC toolchains.
