# GIFF v1.2

`giff` is a **C89**, **zero-allocation**, **mixed-memory** GIF codec with:

- indexed encode/decode
- RGBA -> indexed quantization
- fixed-point dithering
- global + local palette encode
- memory-backed decode
- callback-fed streaming decode
- incremental `feed()` decode without mandatory callbacks
- fixed-workspace octree + median-cut quantizers
- interlaced GIF encode
- feed-buffer compaction for push decode
- per-frame compression tuning before LZW emission
- heuristic LZW dictionary reset planning
- automatic sparse global-index remap into frame-local palettes
- cross-frame palette clustering and temporal remap for animated encodes
- explicit multi-frame temporal-window palette model
- explicit cost model for keep/local/temporal-remap decisions
- cross-frame cost lookahead that also scores window/segment reuse
- animation-segment palette sharing with bounded segment state
- post-remap frame reanalysis before final LZW strategy selection
- per-band LZW strategy tuning with safe soft-break clears
- expanded valid-but-cursed corpus + wider fuzz seeds

This cut keeps the same hard rules:

- no hidden `malloc`
- caller-owned workspace
- fixed-size LZW tables
- bounded packetizer state
- decoder/encoder state separated cleanly
- mixed model: small inline state + heavy caller workspace

## What landed in v1.2

- **cross-frame palette clustering** via bounded temporal palette weights and previous-palette matching
- **temporal remap for animated frames** when a sparse or local palette can be reordered without changing colors
- **band-aware LZW tuning** that can switch strategies across row bands and insert safe soft-break clears
- expanded **valid-but-cursed** regression corpus covering loader streams, duplicated GCEs, weird extension ordering, and zero-delay looped animations


- **deferred-clear capable LZW encoder** with per-frame strategy selection
- **entropy-guided reset heuristics** choosing between compact / balanced / aggressive LZW behavior
- **auto-local palette synthesis** for sparse indexed frames when the local-table overhead is smaller than the expected compression win
- **local palette compaction + frequency ordering** for frames that already carry a local table but only touch a sparse subset of indices
- new encoder stats in `giff_encoder` for:
  - `frame_used_colors`
  - `frame_encoded_palette_entries`
  - `frame_entropy_q8`
  - `frame_reset_threshold`
  - `frame_periodic_clear`
  - `frame_stall_threshold`
  - `lzw_clear_count`
  - `lzw_strategy`
- new bounded workspace slice for **per-code phrase lengths** in the LZW encoder
- new tests for **auto-local remap** and **aggressive multi-clear LZW encoding**

- **bounded multi-frame temporal window** with a caller-configurable history length (`temporal_window_frames`, capped by `GIFF_TEMPORAL_WINDOW_MAX`)
- explicit **palette decision cost model** comparing keep/current, local-frequency remap, and temporal-remap candidates before emission
- new encoder decision stats in `giff_encoder`:
  - `frame_cost_keep`
  - `frame_cost_local`
  - `frame_cost_temporal`
  - `frame_cost_chosen`
  - `frame_cost_mode`
  - `frame_window_used`
  - `temporal_window_similarity_q8`
- new regression coverage for:
  - cost model preferring keep-mode when dense low indices already compress well
  - multi-frame temporal-window remap selection across animated sparse-index frames
  - extra valid-but-maldito fixtures (`empty-comment`, `interlaced-local`, `app-comment-empty`)

- **cross-frame cost lookahead** that now scores candidate palettes against the bounded temporal window and against an active animation segment
- **segment palette sharing** that can reuse a stable local palette across several related animation frames when remapping stays cheap
- **segment break / reuse stats** in `giff_encoder` (`frame_cost_segment`, `segment_frame_count`, `frame_segment_shared`, `frame_segment_break`, `segment_similarity_q8`)
- **post-remap reanalysis** so min-code-size, entropy, and temporal history are computed from the palette ordering actually written to the frame
- extra valid-but-weird fixtures for **GCE scope across comments** and **repeated application extensions**

## Compression tuning in v1.2

GIFF now does more than just choose the minimum legal LZW code size.

At `giff_encoder_end_frame()` time it can:

1. inspect the actual indexed raster staged for the frame
2. estimate **frame entropy** from the palette histogram using integer-only math
3. compact a local palette to the indices that are actually used
4. synthesize a temporary local palette for sparse global-index frames when that is a net win
5. choose an LZW reset strategy before emitting image data

### LZW strategy buckets

```c
GIFF_LZW_STRATEGY_COMPACT
GIFF_LZW_STRATEGY_BALANCED
GIFF_LZW_STRATEGY_AGGRESSIVE
```

The current heuristics use:

- used-color count
- repeated-adjacent-pixel ratio
- dominant-color share
- fixed-point entropy estimate

to decide whether to preserve a fuller dictionary longer, reset on stall, or clear periodically for noisy frames.

## Sparse local remap

For indexed frames, GIFF can now tighten the active index space before compression.

### Existing local palette

If a frame already carries a local palette, GIFF can:

- drop unused entries
- move the transparency index to slot 0 when it is actually used
- sort the remaining entries by descending frequency
- rewrite the staged indexed pixels through the dense remap

### Sparse global palette usage

If a frame uses a very sparse subset of a large global palette, GIFF can build a **temporary local palette** for that frame and remap the pixel indices into a denser range. This improves both:

- the minimum legal GIF LZW code size for that frame
- the practical usefulness of the dictionary for short, noisy indexed streams

## Quantizer / dithering

### Quantizer

GIFF still exposes two heap-free palette builders:

1. **bounded 5:5:5 histogram + median cut**
2. **fixed-node octree reduction**

When the octree path is selected and the palette was built from that octree, GIFF can map pixels back through the reduced tree directly before falling back to weighted nearest-color search.

### Dither modes

```c
GIFF_DITHER_NONE
GIFF_DITHER_ORDERED4X4
GIFF_DITHER_FLOYD_STEINBERG
```

The Floyd–Steinberg path uses scaled integer error buffers in caller workspace; there are no floating-point dependencies.

## Mixed-memory model in v1.2

Caller workspace owns the heavy bounded pieces for **both** the codec core and the quantizer path:

- decoder LZW prefix / suffix / stack
- decoder sub-block scratch + scanline scratch
- decoder canvas / previous buffers when caller does not provide them
- decoder streaming buffer (small for callback mode, compacted/bounded for feed mode)
- encoder LZW hash tables
- encoder LZW code-length table
- encoder sub-block packet buffer
- encoder scanline index buffer
- encoder full indexed frame staging buffer
- encoder histogram workspace
- encoder median-cut bin lists
- encoder octree node pool
- encoder error-diffusion rows

## Incremental feed mode

The callback path is still the best fit for truly unbounded streams.

The `feed()` path is for callers that want **push-style** decoding without providing `giff_io.read` callbacks. It works like this:

```c
giff_decoder_feed(&dec, chunk0, size0, &consumed);
giff_decoder_next_event(&dec, &ev); /* may return GIFF_E_NEED_MORE_INPUT */
...
giff_decoder_finish_input(&dec);
```

Notes:

- feed mode keeps a **contiguous accumulation buffer** inside workspace
- once older bytes are consumed, the decoder **compacts unread bytes in place** before accepting more input
- its capacity is determined by the decoder workspace reservation
- partial blocks return `GIFF_E_NEED_MORE_INPUT` until enough bytes arrive
- after `giff_decoder_finish_input()`, any still-incomplete block resolves to `GIFF_E_TRUNCATED`

## Build

```sh
make
make test
make fuzz
./bin/giff_smoke
```

## Shipped tests in v1.2

The test suite now covers:

- header inspection + one-shot decode
- callback streaming decode with chunked reads + comment capture
- push-style incremental `feed()` decode
- indexed encode -> decode roundtrip
- global RGBA quantize -> encode -> decode roundtrip
- local-palette RGBA encode -> decode roundtrip
- octree RGBA roundtrip through direct-map capable encode path
- per-frame minimum code-size tuning regression
- automatic sparse-global -> local remap regression
- aggressive LZW multi-clear regression
- median-cut palette spread sanity check
- octree palette spread sanity check
- Floyd–Steinberg dither sanity check
- interlaced encode -> decode roundtrip
- large animated push-feed compaction case
- malformed / truncated decode cases
- fixture-corpus regression coverage
- deterministic mutation-based fuzz-like coverage across memory / feed / callback decode paths

## Intentional limits still left after v1.2

- the octree is a **fixed-node** implementation aimed at bounded embedded-style workloads, not a dynamically growing tree
- feed mode is still backed by a **bounded contiguous accumulation buffer**, so callback mode remains the more scalable option for huge or open-ended streams
- the entropy / reset logic is still a **heuristic policy layer**, not a brute-force optimal parser
- temporal clustering is still **bounded and heuristic**, not a multi-frame global optimizer
