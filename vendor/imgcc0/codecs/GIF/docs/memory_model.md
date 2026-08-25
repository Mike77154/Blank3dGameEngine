# GIFF memory model

GIFF keeps a **mixed** memory model.

## Inline state

`giff_decoder` and `giff_encoder` contain the small control fields that stay hot during parsing or emission:

- configuration
- counters
- active frame metadata
- pending Graphic Control Extension state
- event emission cursors
- parse pointers for in-memory / feed decode
- palette snapshots
- bounded segment-palette sharing state for animated encodes
- pointers into workspace slices
- octree/direct-map bookkeeping for the current quantized palette
- per-frame compression-analysis stats and selected LZW strategy

## Workspace state

Large or bounded-but-bulky structures live in a caller-owned workspace:

- LZW tables
- LZW expansion stack
- GIF packet / sub-block scratch buffers
- indexed canvas
- RGBA canvas
- previous-canvas snapshot buffer
- quantizer histogram
- median-cut active-bin arrays
- fixed octree node pool
- error-diffusion rows
- incremental feed buffer
- full indexed frame staging buffer for encoder-side planning
- encoder-side LZW phrase-length table used by reset heuristics
- bounded temporal palette model (`temporal_palette`, `prev_palette`, `temporal_weights`) for animated clustering/remap
- bounded temporal-window palette/history slices for multi-frame clustering and cost-model decisions
- per-band LZW tuning state (`frame_band_rows`, `frame_band_count`, switch/clear counters)

This keeps the library heap-free while still allowing the decoder and encoder to handle serious use cases.

## Decoder layout in v1.2

Depending on output mode, the decoder workspace can hold:

- always:
  - `prefix[4096]`
  - `suffix[4096]`
  - `stack[4096]`
  - one data sub-block scratch buffer
  - one scanline scratch buffer
  - one stream / feed buffer
- `GIFF_OUTPUT_RGBA8888`:
  - RGBA canvas
  - optional RGBA previous buffer
- `GIFF_OUTPUT_INDEXED`:
  - indexed canvas
  - optional indexed previous buffer

## Encoder layout in v1.2

The encoder workspace now reserves:

- LZW hash tables
- one **LZW code-length table** for heuristic reset decisions
- one packet buffer for GIF sub-block emission
- one indexed row buffer
- one full indexed frame staging buffer for **all** frame encodes
- histogram counts for 5:5:5 quantization
- two bin arrays for median-cut partitioning
- one fixed octree node pool
- two error rows for Floyd–Steinberg diffusion

That full-frame staging buffer is what makes these v1.2 features possible without hidden allocation:

- entropy analysis
- sparse local-palette remap
- auto-local palette synthesis for sparse global-index frames
- per-frame LZW reset-strategy planning
- explicit keep/local/temporal-remap cost-model evaluation
- multi-frame temporal-window aggregation without heap allocation
- cross-frame cost lookahead against window and segment history
- bounded animation-segment palette sharing without heap allocation

## Why the previous buffer is optional

GIF disposal mode 3 (`restore to previous`) requires the decoder to restore the area that existed before the frame was drawn.

GIFF exposes this cost explicitly:

- disable it entirely
- restore from caller-visible previous storage if available
- use full-canvas storage when the configured mode requests it

Bounds-vs-full remains a policy knob even though the current storage reservation is conservative and still allocates full previous surfaces when previous storage is enabled.

## Event API and memory ownership

The progressive event path does **not** allocate per event.
Pointers returned in `giff_event` borrow internal decoder storage:

- `rgba_row` / `indexed_row` point into the decoder canvas
- `text_bytes` points into the decoder sub-block scratch buffer

Those pointers stay valid until the next decoder call mutates the underlying state.

## Feed mode ownership

The push-style feed path stores incoming bytes in a decoder-owned workspace slice.
As bytes are consumed, GIFF can compact the unread tail in place before accepting more input. The buffer is still bounded by workspace size, so callers that expect very large or long-lived streams should still prefer callback mode.
