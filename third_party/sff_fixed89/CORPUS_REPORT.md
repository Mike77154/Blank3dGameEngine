# Corpus check report

Corpus supplied by the user:

- `kfmv1wm.sff`
- `kfmv2m10.sff`
- `kfmv2m11.sff`

Test harness used: `tests/corpus_check.c`

## Results with embedded sublibs

### `kfmv1wm.sff`
- kind: v1
- sprites parsed: 281
- indexed decode: 281 / 281
- RGBA decode: 281 / 281
- failures: 0

### `kfmv2m10.sff`
- kind: v2
- sprites parsed: 281
- indexed decode: 281 / 281
- RGBA decode: 281 / 281
- failures: 0

### `kfmv2m11.sff`
- kind: v2
- sprites parsed: 281
- indexed decode: 281 / 281
- RGBA decode: 281 / 281
- failures: 0

## Notes

The previous MUGEN 1.1 gap is closed by the new embedded path:

```text
sff -> png89 -> zlib89
```

The PCX route is also tied into the sublib layer:

```text
sff -> pcx89
```
