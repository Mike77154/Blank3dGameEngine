# Contract annotations

This release adds compiler-visible API contracts to the installed public header.

- `BMP_WARN_UNUSED_RESULT` for status and computed-value APIs.
- `BMP_ATTR_NONNULL_N(...)` for mandatory pointer arguments.
- `BMP_ALLOCATOR` plus `BMP_ATTR_ALLOC_SIZE_1` / `BMP_ATTR_ALLOC_SIZE_2` for allocator-style return values.

New helpers: `bmp_malloc(size_t)` and `bmp_calloc(size_t, size_t)`, both paired with `bmp_free_memory()`.


## Round 1.14 additions

This round extends the surface with `returns_nonnull`, explicit allocator/deallocator pairing, and ownership metadata for custom allocation APIs.
