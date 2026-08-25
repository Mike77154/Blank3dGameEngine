# Contract annotations — caller-owned storage

The public export header keeps compiler-aware contracts that remain truthful under the strict C89 model:

- `BMP_WARN_UNUSED_RESULT`
- `BMP_RETURNS_NONNULL`
- `BMP_ATTR_NONNULL_*`
- SAL input/output buffer annotations on MSVC
- GCC access read/write annotations where supported
- visibility, analyzer, pure/const, cold, noinline and deprecation helpers

Ownership-return annotations are intentionally absent because the codec does not create caller-owned dynamic blocks. Encoder, decoder and payload-copy operations instead describe caller-provided output/workspace buffers and capacities.

Run `make contract-audit`, `make contract-smoke`, and `make buffer-contract-smoke` to verify the annotation layer.
