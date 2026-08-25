# Buffer contracts

Round 1.16.0 adds buffer-direction and size contracts to the installed API.

## What is covered

- GCC-only `access(read_only|write_only|read_write, ...)` annotations for public APIs with direct buffer parameters.
- MSVC SAL wrappers for `_In_reads_bytes_`, `_Out_writes_bytes_`, `_Inout_updates_bytes_`, `_In_z_`, and output-pointer contracts.
- Clang C++ `lifetimebound` scaffolding for APIs that return views tied to input storage.
- Additional warning-policy hooks so GCC can diagnose overreads/overflows at compile time when sizes are constant.

## Current scope

The PCX public surface does not have many view-returning APIs, so most of the value in this round comes from read/write size contracts on memory-loading, formatting, and encode/decode entry points.
