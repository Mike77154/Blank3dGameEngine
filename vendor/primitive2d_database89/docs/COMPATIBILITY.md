# Compatibility — v0.3.0 to v0.4.0

## Preserved

- Shape IDs `0..335` retain their original ordering and names.
- The provider command ABI remains `POINT / MOVE / LINE / QUAD / CUBIC / CLOSE`.
- `P2D89_Command`, provider callback signatures and the existing metadata layout are unchanged.
- The emitted geometry streams for all 336 v0.3 shapes are byte-identical in v0.4.

Preservation stream:

```text
84,461 bytes
SHA-256 7a6160719bade9014d2122f460df36e8ffeab4d1deef7b89e9804a320d9bb119
336 / 336 identical
```

## Additive changes

v0.4 appends 384 shapes (`336..719`), taking the catalogue to 720 shapes and 57 physical registry submodules.

New module IDs are additive:

```text
DIAGRAMS
CONNECTORS
PATTERNS
GRAPH_SYMBOLS
ANNOTATIONS
WEATHER_SYMBOLS
ARCHITECTURE
```

New semantic flags are additive within the existing unsigned 16-bit flags field:

```text
REPEATABLE_TILE = 0x1000
MARKER          = 0x2000
DIAGRAM_NODE    = 0x4000
EDITOR_HELPER   = 0x8000
```

## Integration note

Code that iterates `P2D89_SHAPE_COUNT` will now see 720 entries instead of 336. Code that hard-coded the former count should be updated to query `p2d89_shape_count()` or use the current enum constant.
