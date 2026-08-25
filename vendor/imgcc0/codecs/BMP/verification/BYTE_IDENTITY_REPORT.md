# Byte-identity verification report

This report compares the original BMP codec with the sanitized C89/static-workspace codec.

## Result

- Encoder outputs: **15/15 byte-identical**.
- Parse/decode canonical outputs: **60/60 byte-identical**.
- Diagnostic text/JSON outputs: **27/27 byte-identical**.

The canonical parse/decode record includes parse status, parsed metadata, diagnostic fields, decoded-size status/size, decode status, and the raw RGBA bytes.

## Encoder cases

| Case | Bytes | SHA-256 |
|---|---:|---|
| `idx1_rgb` | 106 | `67cb0108c837abc7c11b2b9114c62533ca32b851d131126f450112fd30defe32` |
| `idx4_rgb` | 250 | `c87a1b8f594b39988338a83fde93991beb589af5b43ffd10d524ab7c8650599a` |
| `idx4_rle4` | 252 | `c130f430090416b0ccdbe93c358048669736325b8ed65838505e8a1939e572c7` |
| `idx8_core12` | 1014 | `11ad9a3736382e20ca0a3963c43a4376e4b280d78d68997099443d9eeff44b9f` |
| `idx8_rgb` | 1298 | `d4d26b8ce6fb8ee716c0c4c04561d07d5216cc0ea62c832d0ec58f6bc250b75d` |
| `idx8_rle8` | 1298 | `79ea6f85f40e88a0fc2a05d491f804825c1bc947cd273fc69af9eb2673dbf0db` |
| `rgba_bf30_1_1` | 818 | `71fa286a1e08a86d53011b73066950caa0c927f37c4f27e518a398dd05205723` |
| `rgba_bf4444` | 466 | `62b97f80aac6c19c1ec88d67fd13d8483d7fa7079cfe4fbe3efff656494fd4b8` |
| `rgba_bgr24` | 626 | `97d60c5e45511bf22ef845ddae3b857729e3a47d5920234e23397ee1f3b57877` |
| `rgba_bgr24_topdown` | 626 | `deb1b0e7c9b2a62d7bcec6c708601e1bb8c8e68015b8aa48f9bcbc821e479ff1` |
| `rgba_bgra32` | 886 | `036540aa096584d8da661d4d40d7b16c0865992ceb4b8a4a236db92e161cbe1c` |
| `rgba_bgrx32` | 802 | `9bc2468362483f4cdd69be185ac2e36a41daf2a193e1566b262fa53f0d47f4b8` |
| `rgba_rgb555` | 450 | `1194ad729ab04683495d289fbc47b7158a2943e7ea8eb8ecd75d6da11ed7318c` |
| `rgba_rgb565` | 462 | `9f36ccce57e020d915cff821fbeffbbb3644ff697efe00df2c6e44f4c2b24548` |
| `rgba_v5_icc` | 729 | `83cd360bca22ab14e59c8b8f238e48f76e2440d97b49e134e93715dd899cd61f` |

Coverage includes indexed 1/4/8-bit output, RGB, RLE4, RLE8, OS/2 CORE12, RGB555, RGB565, BGR24, top-down BGR24, BGRX32, BGRA32, 16-bit custom bitfields, a 30/1/1 custom bitfield case, and V5 with ICC payload.

## Strict-language gate

The eight codec translation units and a standalone public-header consumer compile with:

```text
-std=c89 -pedantic-errors -Wall -Wextra -Werror -Wdeclaration-after-statement
```

## Source sanitation gate

The delivered `.c` and `.h` files were scanned for the prohibited allocator, floating-point, native-wide-integer and platform-size type spellings requested for this sanitation. No matches remain.

## Scope note

This is strong differential regression coverage, not a proof over every possible byte sequence in the BMP format. Inputs outside the tested corpus can still reveal bugs. The sanitizer deliberately changes ownership-oriented convenience APIs: callers now provide output buffers/workspaces.
