# MERGE_NOTES

This tree was reconstructed from the **full v1.5 zip tree** and then overlaid with the **v1.6 patch drop**.

## Base used
- `psd89_v1_5_vector_masks_zip.zip` (full tree with Makefile, tests, corpus, zip backend, vector-mask base)

## Overlaid from v1.6
- `include/psd89/psd89.h`
- `src/psd89_vector_bezier.c`
- `src/psd89_mask_global_tags.c`
- `src/psd89_zip_diag.c`
- `tests/test_v16.c`
- `INTEGRATION_NOTES.md`
- `patches/v1_6_header.diff`

## Additional merge work performed here
- Updated `Makefile` to build/link v1.6 objects and `tests/test_v16`
- Wired `lmgm` / `vmgm` parse and write support into `src/psd89_read.c` and `src/psd89_write.c`
- Preserved the existing v1.5 compose pipeline; adaptive Bezier flattening remains available as public/library code and standalone test coverage, but is **not yet fully wired into `src/psd89_compose.c`** in this merged tree.

## Verification
- `make clean && make test`

## Known limitation after merge
- `lmgm` / `vmgm` fields are parsed/written, but the full documented final-crossfade semantics are still described in `INTEGRATION_NOTES.md` and are not yet fully applied in the v1.5 composer path.
