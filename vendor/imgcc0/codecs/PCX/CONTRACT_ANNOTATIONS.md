# Contract annotations — PCX89 static-storage edition

The installed API retains compiler-visible contracts that do not imply dynamic ownership:

- `PCX_EXPORT` / `PCX_NO_EXPORT` for DLL and shared-library visibility.
- `PCX_WARN_UNUSED_RESULT` for status and predicate APIs.
- `PCX_RETURNS_NONNULL` for guaranteed string-return APIs.
- `PCX_ATTR_NONNULL_N(...)` for mandatory pointer arguments.
- SAL read/write byte-range annotations on MSVC.
- GCC/Clang access, pure, const, cold, noinline and noreturn attributes where available.

Legacy allocation-annotation macro names remain defined as inert compatibility macros so old annotation-aware consumers do not fail preprocessing, but PCX89 has no public runtime allocation API.

The user-buffer ownership path is explicit: callers can provide storage with `pcx_image_use_buffer()` and `pcx_indexed_image_use_buffer()`, or select the configured static stores with the corresponding `*_use_static()` calls. `*_release()` only resets the view; it does not release runtime memory.

Historical `RELEASE_1_13.md` and `RELEASE_1_14.md` files are preserved as release history for the pre-PCX89 allocation model and should not be read as the current ownership contract.
