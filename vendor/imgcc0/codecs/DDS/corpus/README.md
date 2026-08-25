# Byte-parity corpus

`parity_expected/` contains 36 deterministic DDS/RGBA artifacts emitted by the untouched pre-sanitization codec.

The corpus covers all 14 public storage formats plus automatic mip generation, sRGB mip generation, BC5 normal-map encode paths, normal-map automatic mips, Z reconstruction, normalization, and Y inversion.

Run from the project root:

```sh
./tools/verify_parity.sh
```

The verifier compiles `tools/parity_harness.c` and the current codec as strict C89, emits a fresh corpus into temporary fixed filesystem storage, and compares every artifact byte-for-byte with `cmp`.
