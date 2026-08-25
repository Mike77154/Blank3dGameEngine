# Warning policy

The strict build treats warnings as errors and compiles the C library as ISO C89.

- GCC/Clang baseline: `-Wall -Wextra -Wpedantic -Werror -pedantic-errors` plus strict prototypes, missing prototypes, shadow, pointer arithmetic, format, undef, bounds, null-dereference and string diagnostics when available.
- MSVC baseline: `/W4 /WX /permissive-` with optional `/analyze`.

Warnings aimed specifically at dynamic allocator/deallocator pairing are no longer part of the required policy because the public/core storage model is caller-owned and static-workspace friendly.
