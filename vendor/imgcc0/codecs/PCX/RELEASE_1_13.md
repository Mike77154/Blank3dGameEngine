# Release 1.16.0

- Added public contract-annotation macros to `include/pcx/pcx_export.h`.
- Annotated high-value public APIs with `warn_unused_result` and `nonnull` style contracts.
- Added allocator helpers `pcx_malloc()` and `pcx_calloc()` with `malloc` / `alloc_size` semantics.
- Added `scripts/audit_contract_annotations.py` and `make contract-audit`.
