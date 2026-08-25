# PCX 1.16.0

This release adds semantic contract metadata on top of the previous contract-annotation pass.

## Highlights

- Added `PCX_RETURNS_NONNULL` for string-returning APIs that always yield stable literals.
- Added allocator/deallocator pairing metadata with `PCX_ATTR_MALLOC_DEALLOCATOR(...)`.
- Added Clang Static Analyzer ownership annotations for allocator, deallocator, and ownership-transfer APIs.
- Tightened compiler-specific warning policy in both Make and CMake builds.
- Added negative compile tests for ignored results, nullability, and GCC mismatched deallocation.
