# Fuzzing and malformed corpus

GIFF v1.2 ships two layers of hardening material.

## 1. Fixture corpus

`tests/corpus/` contains tiny valid and malformed GIF samples that exercise:

- valid comment extension parsing
- bad signature rejection
- truncated local color table rejection
- invalid LZW minimum code size rejection
- missing raster terminator handling
- truncated raster sub-block handling
- truncated application extension handling
- short local-table payload handling
- loader-only streams with just a global color table and trailer
- duplicate Graphic Control Extensions before one image
- application/comment/control ordering oddities
- zero-delay looped multi-frame animations
- empty comment-extension samples
- interlaced local-palette streams
- application + empty-comment ordering oddities
- Graphic Control Extension scope surviving an intervening comment block
- repeated application-extension streams that keep the last loop payload

## 2. Deterministic fuzz-like harness

Build and run with:

```sh
make fuzz
```

The harness mutates multiple seed GIFs across deterministic bit flips, truncations, and short appends. Each mutation is then driven through:

- one-shot memory decode
- push-style `feed()` decode
- callback-streaming decode

The hard requirement is that GIFF never returns `GIFF_E_INTERNAL` and never crashes while doing so.

## Hardening focus in v1.2

- preflight completeness checks for extensions and image payloads in feed mode
- bounded median-cut workspace
- bounded octree node pool
- direct octree-map encode path with nearest-color fallback
- bounded full-frame staging for per-frame compression tuning
- bounded feed-buffer compaction exercised by large animated push streams
- bounded error-diffusion rows
- bounded LZW phrase-length side table for reset heuristics
- regression tests for auto-local palette remap and aggressive multi-clear image encoding
- regression tests for explicit keep/local/temporal-remap palette decisions
- regression tests for multi-frame temporal-window cost-model behavior
- malformed stream tests for signature corruption, truncated color tables, invalid LZW code-size bytes, truncated sub-blocks, cut application extensions, GCE/comment scope oddities, and repeated app-extension payloads
