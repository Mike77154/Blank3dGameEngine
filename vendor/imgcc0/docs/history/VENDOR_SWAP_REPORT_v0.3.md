# imgcc0 0.3.0 vendor swap

## Requested replacement set

The old PNG, DDS, BMP and PSD vendors were requested for removal. The attached replacement archives actually contained PNG, DDS, BMP and PCX.

## Result

- Old `codecs/PNG` removed; supplied PNG package vendored intact and integrated.
- Old `codecs/DDS` removed; supplied DDS package vendored intact and integrated.
- Old `codecs/BMP` removed; supplied BMP package vendored intact and integrated.
- Old `codecs/PCX` removed; supplied PCX89 package vendored intact and integrated.
- Old `codecs/PSD` removed. No replacement PSD package was supplied, so PSD decode remains protocol-quarantined.

PCX was not relabeled or treated as PSD.

## Adapter policy

Replacement vendor source trees are not rewritten. Integration lives in the imgcc0 facade/build layer. The caller still owns imgcc0 output/scratch/file storage. PNG/APNG, DDS, BMP and PCX outputs are copied or decoded into imgcc0 caller-owned RGBA output without exposing vendor-owned storage.

## Verification

- ISO C89 pedantic build: PASS.
- Protocol audit: PASS across 92 active C/header files.
- Dynamic allocation calls in active strict set: 0.
- `float` / `double` in active strict set: 0.
- Explicit 64-bit integer types in active strict set: 0.
- Unresolved malloc/calloc/realloc/free symbols in `libimgcc0.a`: 0.
- Existing golden regression: 13/13 byte-identical frames.
- New direct-vendor adapter parity: 9/9 byte-identical canonical outputs.
- Vendor integrity: PNG 230/230, DDS 80/80, BMP 250/250, PCX 234/234 files byte-identical to supplied packages.
