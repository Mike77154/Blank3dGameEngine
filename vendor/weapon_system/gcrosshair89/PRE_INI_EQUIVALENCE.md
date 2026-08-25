# Pre-INI equivalence validation

Baseline: `gcrosshair_base89_preset_pack192_vector_only_abi3_outline.zip`, the hardcoded 192-preset implementation before the INI refactor.

## Canonical preset bytes

The old and new implementations were compiled separately. Each exported all 192 preset IDs, names, categories, complete normal/aim/fire/hit variants, style flags/spread/offsets, and the four legacy animation-range fields into a deterministic little-endian canonical stream.

- bytes per stream: 88,884
- old SHA-256: `7024ddf62fc175a8d63055365637bb5fbc3518173d456bd5d1f1b231a372c088`
- new SHA-256: `7024ddf62fc175a8d63055365637bb5fbc3518173d456bd5d1f1b231a372c088`
- `cmp`: identical

A copy of that pre-INI golden stream is shipped as `gcrosshair_base89/tests/golden/pre_ini_hardcoded_192.bin`; `test_pre_ini_equivalence` checks the current INI loader against it on every test run.

## Raster stream

Both implementations were then rendered through the same params/core/software primitive path at 96x96 RGBA8888 for every preset, four states (normal/aim/fire/hit), and two spread values (0 px and 3 px): 1,536 frames total.

- bytes per stream: 56,623,104
- old SHA-256: `0fedb5522740438d63c6c02e90c328852ca641604ade50130c31bd50a7ede320`
- new SHA-256: `0fedb5522740438d63c6c02e90c328852ca641604ade50130c31bd50a7ede320`
- `cmp`: identical

The new event animations are additive and were not triggered during this regression test. Therefore the original static/state appearance remains the pre-INI appearance byte-for-byte.
