# ABI policy (1.x)

The shared library now ships with three layers of ABI discipline:

- ELF `DT_SONAME` is pinned to `libbmp.so.1`.
- exported dynamic symbols are controlled by `exports/bmp.map`.
- `scripts/check_exports.py` verifies the actual export set against `exports/bmp.exports.txt`.

## Compatibility rule

Within the 1.x line:

- adding new exported functions is allowed;
- changing or removing an exported symbol is a breaking ABI change;
- a breaking change requires bumping the shared-library major and the public package major.

## Validation

Use:

```sh
make abi-check
```

This checks both the exported symbol set and the SONAME recorded in the shared object.
