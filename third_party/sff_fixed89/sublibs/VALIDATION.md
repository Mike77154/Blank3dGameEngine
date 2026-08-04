# Validation notes for added sublibs

## Build

```bash
make clean && make
```

This produces:
- `libsff.a`
- `libpcx89.a`
- `libpng89.a`
- `libzlib89.a`

## zlib89

A local random-vector check was run by compressing multiple random payloads with host zlib and inflating them with `zlib89`. The generated outputs matched the originals.

## Corpus status through embedded stack

Using `libsff.a` with no external PNG codec callback and caller-provided work buffers:

- `kfmv1wm.sff` -> indexed 281/281, rgba 281/281
- `kfmv2m10.sff` -> indexed 281/281, rgba 281/281
- `kfmv2m11.sff` -> indexed 281/281, rgba 281/281

That validates the integrated path: `sff -> png89 -> zlib89` and `sff -> pcx89`.
