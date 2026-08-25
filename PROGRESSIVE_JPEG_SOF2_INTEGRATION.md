# Blank3D / imgcc0 Progressive JPEG SOF2 Integration

## Problem

The pistol muzzle source is a 512x512 progressive JPEG. The old c89jpeg decoder
accepted only baseline sequential SOF0, so imgcc0 failed during JPEG probe/decode.
The temporary TGA conversion proved the muzzle/render path but did not fix JPEG.

## Fix

c89jpeg v1.2.0 now decodes progressive Huffman SOF2 on its complete-memory paths.
Blank3D therefore points the pistol directly at:

`config/weapons/assets/pistol_muzzle_flash.jpg`

The compatibility TGA is no longer required.

## Decode architecture

```text
JPEG SOF2
   |
   v
probe frame / sampling / tables
   |
   v
caller-owned coefficient workspace
   |
   +--> DC first
   +--> AC first + EOBRUN
   +--> DC refine
   +--> AC refine
   +--> restart handling
   |
   v
all scans complete
   |
   v
fixed-point dequant + IDCT + color conversion
   |
   v
imgcc0 RGBA32 -> Blank3D image registry -> SpritePlane89
   |
   +--> core additive muzzle
   `--> enlarged additive glow pass
```

## Memory policy

The coefficient store is never allocated by c89jpeg. The caller asks
`c89jpeg_decoder_progressive_workspace_size()` for the required byte count. imgcc0
then takes that memory from its pre-existing fixed temporary arena.

The supplied 512x512 4:4:4 muzzle requires 1,572,864 bytes of coefficient workspace,
which fits in Blank3D's current imgcc0 temporary arena.

## Safety boundary

- maximum 64 progressive scans per image
- invalid Ss/Se/Ah/Al combinations are rejected
- AC scans must contain one component
- restart markers reset DC predictors and EOB run state
- truncated/invalid entropy data returns an error rather than synthesizing scans

## API boundary in this pass

Supported now:

- baseline SOF0 memory decode
- progressive SOF2 complete-memory decode
- progressive SOF2 memory-to-sink decode

Still baseline-only:

- resumable/source-input streaming decoder
- JPEG encoder

This boundary is deliberate. Progressive input suspension requires a second state
machine that preserves coefficient updates and entropy/refinement state transactionally.
It should not be faked by buffering internally because that would violate the no-heap
provider model.

## Regression gates

- c89jpeg strict C89 test suite
- matched baseline/progressive 4:2:0 fixture with byte-identical RGB output
- imgcc0 full protocol audit/build
- native pistol progressive-JPEG muzzle test
- image-stack regression
- generic muzzle-image regression
- SpriteAsset89/SpriteVerbs89/AssetRoute89 regression
- whole-engine syntax check

The pistol test requires `IMGCC0_FMT_JPEG`, 512x512 output, a dark decoded corner, a
white-hot decoded center, and the existing two-pass additive core/glow presentation.
