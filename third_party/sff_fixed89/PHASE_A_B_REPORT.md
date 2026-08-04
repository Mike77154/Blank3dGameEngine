# Phase A / B report

## Scope

Investigate the visual corruption seen in the KFM corpus without changing the SFF container format itself.

## Corpus status after fixes

- `kfmv1wm.sff`: 281 / 281 indexed, 281 / 281 RGBA
- `kfmv2m10.sff`: 281 / 281 indexed, 281 / 281 RGBA
- `kfmv2m11.sff`: 281 / 281 indexed, 281 / 281 RGBA

## Phase A findings

### 1) The main v2 corruption was **not** PNG and **not** zlib/DEFLATE

The widespread vertical/"liquified" distortion in `kfmv2m10.sff` and most of `kfmv2m11.sff`
comes from the SFFv2 `LZ5` sprite decompressor.

Using sprite `(group=0,item=0)` / sprite index `2` as the oracle case:

- expected dimensions: `47 x 106`
- expected raw pixel count: `4982`
- compressed size in `kfmv2m10.sff`: `1018` bytes

A direct comparison against the same sprite from the v1 KFM corpus showed that the old
LZ5 decoder produced the wrong indexed pixel stream even though the output size looked plausible.

### 2) Root cause in LZ5

The old implementation treated **literal runs** as `size + 1`.
For the KFM corpus, the correct behavior is:

- **literal run**: `run = size`
- **back-reference run**: `run = size + 1`

After changing only that rule, the decoded indexed stream for the KFM `(0,0)` sprite matches
the v1 oracle exactly.

### 3) The v2.11 black portrait was a PNG path bug

The first sprite in `kfmv2m11.sff` is PNG-backed and has the PNG signature at **offset 4** in the blob.
The integrated PNG path was decoding indexed PNG directly to RGBA and therefore bypassed the external
SFF palette bank for that sprite. In this corpus, the practical color mapping comes from the SFF palette,
not the embedded PNG palette.

### 4) The v1 black silhouettes were a palette-owner cache bug

The v1 palette cache logic was not reliably updating the current palette owner and could also treat
`same_palette` sprites as palette-bearing sprites. That caused incorrect palette inheritance and black
silhouette output.

## Phase B fixes applied

### Fix 1: LZ5 literal run sizing

File: `sff/sff_decomp.c`

Changed the literal path in `sff_decomp_lz5()` from `run = size + 1` to `run = size`.
Back-references remain `run = size + 1`.

### Fix 2: v1 palette-owner cache

File: `sff/sff_api.c`

- update `current_root` every time a sprite truly owns a palette
- do **not** probe for an embedded palette when `same_palette != 0`

### Fix 3: indexed PNG should use the SFF indexed path

File: `sff/sff_api.c`

The direct PNG-to-RGBA fast path now runs only for non-indexed PNG color types.
Indexed PNG (`color_type == 3`) falls back to the indexed decode path so the external SFF palette
can be applied.

## Files visibly re-rendered after fixes

- `kfmv1wm_fixed.png`
- `kfmv2m10_fixed.png`
- `kfmv2m11_fixed.png`

All three now render correctly from the corpus using the integrated libraries.
