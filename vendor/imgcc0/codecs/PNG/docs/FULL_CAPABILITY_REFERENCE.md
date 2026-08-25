# tiny_png / png platform layer — full C89/fixed89 capability reference

A small embeddable PNG library in C with two personalities:

- **legacy tiny API** for simple RGBA8 decode/encode
- **platform API** for configurable decode/encode, metadata retention, text chunks,
  unknown ancillary chunk preservation, stricter parsing, adaptive filters,
  incremental callback-based decoding, first-class special-purpose chunks,
  configurable public-format encode input, and a first APNG platform layer

## What works now

### Decode
- PNG signature / CRC validation
- strict `IHDR` / `IDAT` / `IEND` handling
- optional rejection of trailing bytes after `IEND`
- color types `0,2,3,4,6`
- bit depths allowed by the PNG spec for those color types
- Adam7 decode
- configurable public output formats:
  - 8-bit: `RGBA8`, `BGRA8`, `ARGB8`, `RGB8`, `BGR8`, `GA8`, `AG8`, `G8`
  - 16-bit (big-endian channels): `RGBA16`, `BGRA16`, `ARGB16`, `RGB16`, `BGR16`, `GA16`, `AG16`, `G16`
- in-place/public conversion helper: `png_image_convert_format()`
- opt-in transforms:
  - apply gamma
  - premultiply alpha
  - swap red/blue
  - invert alpha
  - strip alpha to opaque
- metadata retention:
  - `gAMA`, `cHRM`, `sRGB`, `pHYs`, `oFFs`, `sCAL`, `sTER`, `pCAL`, `bKGD`, `tIME`, `sBIT`
  - `tEXt`, `zTXt`, `iTXt`
  - `hIST`, `sPLT`
  - first-class legacy/special-purpose chunks: `gIFg`, `gIFx`, deprecated `gIFt`, `fRAc`, `dSIG`
  - `iCCP` profile payload
  - unknown ancillary chunks with safe/all policies

### Encode
- generic encoder for PNG color types `0,2,3,4,6`
- supports legal bit depths for those types
- supports both `interlace_method = 0` and `interlace_method = 1` (Adam7)
- indexed images with `PLTE`
- optional suggested `PLTE` for truecolor / truecolor+alpha write paths
- `tRNS` for indexed, grayscale, and truecolor
- adaptive per-row filter selection (W3C-style min-sum heuristic)
- writable metadata:
  - `gAMA`, `cHRM`, `sRGB`, `iCCP`, `pHYs`, `oFFs`, `sCAL`, `sTER`, `pCAL`, `bKGD`, `tIME`, `sBIT`
  - `tEXt`, `zTXt`, `iTXt`
  - `hIST`, `sPLT`
  - first-class legacy/special-purpose chunks: `gIFg`, `gIFx`, deprecated `gIFt`, `fRAc`, `dSIG`
- preserved/private ancillary chunks at `after IHDR`, `after PLTE`, or `after IDAT`
- public-format encode input pipeline (`PNG_OUTPUT_*` as source surfaces) with optional input transforms
- convenience wrapper `png_encode_image_ex[_zlib]()` to re-encode a decoded `png_image` directly

### Incremental API
- buffered incremental feed API
- advanced `*_feed_ex()` entry points with partial-consume reporting for event-loop / socket-driven callers
- info / row / end callbacks plus per-chunk callbacks on both PNG and APNG progressive decoders
- `png_progressive_control` lets callers cap accepted input per call, buffered unread bytes, parse work per call, row callbacks per call, chunk callbacks per call, and total fully-accounted chunk bytes per call
- `PNG_DEC_YIELDED` cleanly reports voluntary backpressure / work-budget yields without losing decoder state
- `png_decoder_unprocessed_bytes()` / `png_apng_decoder_unprocessed_bytes()` plus `*_input_room()` expose unread backlog and current intake room
- `png_decoder_poll()` / `png_apng_decoder_poll()` expose a poll/select-friendly state snapshot (`WANT_INPUT`, `CAN_DRAIN`, `HAVE_BUFFERED`, `PAUSED`, `DONE`) with a suggested next read size for socket/event-loop integrations
- `png_decoder_process_data_pause()` plus `png_decoder_pending_bytes()` / `png_decoder_process_data_skip()` for libpng-style pause/replay or pause/cache control on the progressive PNG decoder
- `png_apng_decoder_process_data_pause()` plus `png_apng_decoder_pending_bytes()` for cached pause/resume on the progressive APNG decoder
- row callbacks receive the requested public output format (`opt.output_format`)
- real row-by-row streaming decode for **both non-interlaced and Adam7** PNG when built with zlib
- Adam7 row callbacks expose progressively reconstructed output rows with pass numbers `1..7`
- usable for network/file chunk feeds without a full file read helper

### APNG
- typed APNG (`acTL`, `fcTL`, `fdAT`) decode to full **composited canvas frames**
- default image is also exposed through `png_apng.default_image`
- APNG encoder writes a standards-compliant animation stream with the first frame carried by `IDAT`
- APNG encoder accepts subframes for frames after the first
- incremental APNG decode can now surface **intra-frame** row callbacks while `IDAT` / `fdAT` bytes are still arriving (zlib build), instead of waiting for the frame-closing `fcTL` / `IEND`

## Current limits
- public output matrix now includes both **8-bit and 16-bit** surfaces; 16-bit channels are exposed in PNG/network byte order (MSB first), with an optional decode-time endian swap transform
- the public input pipeline now accepts `PNG_OUTPUT_*` 8/16-bit surfaces and can apply basic input transforms, but it is still smaller than libpng's full write-transform surface
- runtime safety limits exist, but there is not yet a libpng-sized policy surface for every chunk family
- the first APNG encoder path currently requires frame 0 to cover the full canvas at `(0,0)` so it can act as the default image / first frame
- APNG full decode remains available, and the separate incremental APNG decoder now emits composited row updates intra-frame and still hands off completed frames as soon as they close in the stream
- legacy/special-purpose chunks are first-class, but rarely used extension families beyond the current set are still preserved rather than fully interpreted


## Differential testing against libpng

A separate compatibility harness now compares the library against **libpng** for a curated set of static PNG files.

It checks:

- full decode vs libpng RGBA8 output
- incremental `feed_ex()` / `poll()` decode vs the same libpng reference
- selected metadata parity (`sRGB`, `gAMA`, `pHYs`, `iCCP`, text, `hIST`, `sPLT`)
- encoder compatibility by decoding library-produced PNG output with libpng

Local entry point:

```sh
./differential/run_local.sh
```

The curated corpus is listed in `differential/corpus_static_pngs.txt`. The restored differential gate compares the curated static corpus against libpng; Adam7 inputs are checked pixel-for-pixel in both full and incremental decode paths.

## Build

### With zlib (recommended)

```sh
gcc -std=c89 -O2 -DPNG_DEC_USE_ZLIB -I. -c png_*.c
ar rcs libtiny_png.a *.o
```

Link with `-lz`. The sanitized codec does not require libm.

## Legacy decode example

```c
#define PNG_DEC_USE_ZLIB
#include "png_decoder.h"

png_image img;
int err = png_load_file_zlib("in.png", &img);
if (err == PNG_DEC_OK) {
    png_free_image(&img);
}
```

## Modern decode example

```c
#define PNG_DEC_USE_ZLIB
#include "png_decoder.h"

png_image img;
png_decode_options opt;
png_decode_options_init(&opt);
opt.keep_text = 1;
opt.keep_unknown_chunks = PNG_DEC_KEEP_UNKNOWN_SAFE;
opt.strict_trailing_data = 1;
opt.transform_flags = PNG_DEC_TRANSFORM_NONE;
opt.max_width = 16384;
opt.max_height = 16384;
opt.max_chunk_bytes = 64u * 1024u * 1024u;

opt.output_format = PNG_OUTPUT_BGRA16;

if (png_load_file_ex_zlib("in.png", &opt, &img) == PNG_DEC_OK) {
    /* img.pixels is BGRA16 here; each channel is MSB-first */
    png_image_convert_format(&img, PNG_OUTPUT_RGBA16);
    if (img.has_pCAL) {
        png_fixed89 physical_value;
        if (png_pcal_map_stored_to_physical(img.source_bit_depth, &img.pcal, 128u, &physical_value) == PNG_DEC_OK) {
            /* physical_value is signed Q16.16 for stored sample 128 */
        }
    }
    png_free_image(&img);
}
```

## Modern encode example

```c
#define PNG_DEC_USE_ZLIB
#include "png_decoder.h"

png_encode_options opt;
png_u8* out_png = 0;
png_u32 out_size = 0;

png_encode_options_init(&opt);
opt.color_type = PNG_COLOR_INDEXED;
opt.bit_depth = 4;
opt.input_is_packed = 0;
opt.palette = palette_rgb_triplets;
opt.palette_entries = palette_count;
opt.interlace_method = 1; /* Adam7 */

if (png_encode_memory_ex_zlib(index_bytes, width, height, &opt, &out_png, &out_size) == PNG_DEC_OK) {
    png_free_file(out_png);
}
```

## Re-encode a decoded image directly

```c
png_image img;
png_decode_options dopt;
png_encode_options eopt;
png_u8* out_png = 0;
png_u32 out_size = 0;

png_decode_options_init(&dopt);
dopt.output_format = PNG_OUTPUT_BGRA8;

if (png_load_file_ex_zlib("in.png", &dopt, &img) == PNG_DEC_OK) {
    png_encode_options_init(&eopt);
    eopt.color_type = PNG_COLOR_TRUECOLOR_ALPHA;
    eopt.bit_depth = 8u;
    if (png_encode_image_ex_zlib(&img, &eopt, &out_png, &out_size) == PNG_DEC_OK) {
        png_free_file(out_png);
    }
    png_free_image(&img);
}
```

## APNG decode example

```c
png_apng anim;
png_decode_options opt;
png_decode_options_init(&opt);
opt.output_format = PNG_OUTPUT_RGBA8;

if (png_load_apng_file_ex_zlib("anim.png", &opt, &anim) == PNG_DEC_OK) {
    /* anim.frames[i].pixels contains full composited canvas frames */
    png_free_apng(&anim);
}
```

## APNG encode example

```c
png_apng_encode_frame frames[2];
png_encode_options opt;
png_u8* out_png = 0;
png_u32 out_size = 0;

memset(frames, 0, sizeof(frames));
frames[0].pixels = frame0_rgba;
frames[0].width = canvas_w;
frames[0].height = canvas_h;
frames[0].blend_op = PNG_APNG_BLEND_OP_SOURCE;

frames[1].pixels = frame1_rgba;
frames[1].width = sub_w;
frames[1].height = sub_h;
frames[1].x_offset = 10;
frames[1].y_offset = 20;
frames[1].blend_op = PNG_APNG_BLEND_OP_OVER;

png_encode_options_init(&opt);
opt.color_type = PNG_COLOR_TRUECOLOR_ALPHA;
opt.bit_depth = 8u;

if (png_encode_apng_memory_ex_zlib(frames, 2u, canvas_w, canvas_h, 0u, &opt, &out_png, &out_size) == PNG_DEC_OK) {
    png_free_file(out_png);
}
```

## New in 2.13.0

## New in 2.13.1

- `png_progressive_control` gained `hard_fail_on_budget_exhaustion`, allowing progressive budget exhaustion to return `PNG_DEC_ERR_WORK_BUDGET` instead of only `PNG_DEC_YIELDED` when callers want a hard failure
- The upgrade test suite now has dedicated regressions for every named limit error added in the hardened branch: dimensions, pixel count, inflated bytes, chunk size, chunk count, text bytes, APNG frame count, temporary memory, work budget, and conversion expansion

- Progressive PNG/APNG callbacks now include optional **chunk callbacks** with typed chunk metadata (`type`, `length`, file offsets, total size, and chunk-name property bits) so event-loop or instrumentation code can budget and observe parsing at chunk granularity
- `png_progressive_control` gained `max_chunk_callbacks_per_call` and `max_chunk_bytes_per_call` for finer-grained backpressure than raw parse-byte limits alone
- `png_decoder_poll()` and `png_apng_decoder_poll()` now provide a small reactor-style status snapshot with `WANT_INPUT` vs `CAN_DRAIN` guidance, buffered-byte counters, pending-byte counts, and a suggested next read size
- Chunk callback offsets stay stable across internal buffer compaction because the progressive decoders now track absolute stream offsets instead of transient buffer-relative offsets
- If a feed call yields immediately after finishing `IEND`, the next zero-byte drain/resume call now still finalizes the image/animation correctly instead of leaving the decoder stranded between “last chunk parsed” and “done”

## New in 2.11.0

- Progressive PNG decode can now be **paused from callbacks** and resumed later, with `PNG_DEC_PAUSED` signaling that the decoder intentionally stopped mid-stream
- `png_decoder_process_data_pause(dec, save)` mirrors the libpng progressive pattern closely: with `save != 0` unread bytes stay cached internally, while `png_decoder_process_data_skip()` lets callers switch to replaying those pending bytes themselves
- `png_decoder_pending_bytes()` / `png_decoder_is_paused()` expose the cached unread-byte count and pause state so event-loop / socket readers can integrate cleanly
- Progressive APNG decode gained matching pause/resume control through `png_apng_decoder_process_data_pause()`, `png_apng_decoder_pending_bytes()`, and `png_apng_decoder_is_paused()`
- APNG row-callback pauses now ride through the inner synthetic-frame PNG decoder, so a pause during `IDAT` / `fdAT` composition can resume cleanly without losing incremental state

## New in 2.10.0

- APNG incremental decode now feeds each frame through the same streaming PNG row pipeline used by the static decoder, so `png_apng_decoder_*` can emit **intra-frame composited row callbacks** while `IDAT` / `fdAT` bytes are still arriving
- APNG progressive callbacks now have an optional **pass-aware** hook (`frame_row_pass_fn`) so Adam7 consumers can see the pass number for each emitted canvas row while the compatibility `frame_row_fn` stays unchanged
- Later APNG frame rows are wrapped as synthetic `IDAT` chunks and parsed by the normal incremental PNG decoder, which means Adam7/non-interlaced frame data both ride the existing row-by-row inflate/unfilter path in zlib builds
- Incremental APNG composition now updates only the pixels touched by the current Adam7 pass instead of recompositing the whole refined row from scratch, which avoids redundant work on repeated row callbacks
- Completed APNG frames are still appended to the typed `png_apng` result, and rows that never changed during live composition are backfilled on frame end so callback consumers still receive a full canvas row set per frame

## New in 2.9.0

- Progressive/incremental APNG decoder: `png_apng_decoder_*` with info, frame-info, frame-row, frame-end, and end callbacks
- Frames are emitted as fully composited canvas images in the requested public output format, and each frame is available as soon as its terminating `fcTL` or `IEND` arrives in the byte stream
- `png_apng_decoder_take_animation()` hands off the accumulated typed `png_apng` result after the stream completes
- APNG public decode paths now honor `PNG_DEC_TRANSFORM_SWAP_16_ENDIAN` for 16-bit outputs

## New in 2.8.0

- APNG auto-optimizer: `png_encode_apng_auto_memory_ex[_zlib]()` accepts full-canvas frames and crops each later frame to the minimal changed rectangle
- Auto-optimized frames are emitted as `blend_op=SOURCE`, `dispose_op=NONE` subframes over the previous canvas, matching the APNG output-buffer model
- `png_encode_apng_memory_ex()` now respects per-frame `stride_bytes`, so padded source rows work for both manual and auto-optimized APNG encoding

## New in 2.7.0

- First APNG platform layer: typed `acTL`/`fcTL`/`fdAT` decode to composited frames
- APNG encode helper for animated streams, with frame 0 carried by `IDAT` and later frames emitted as `fdAT`
- `png_apng.default_image` preserves the static/default image view of the animation stream
- Earlier 2.6.x additions remain: `gIFg`, `gIFx`, deprecated `gIFt`, `fRAc`, modern color metadata, typed `dSIG`, and the public-format encode input pipeline

## Hardened safety limits and explicit limit errors

This branch adds explicit runtime limits and named failure modes for:

- `max_width`
- `max_height`
- `max_pixels`
- `max_file_bytes`
- `max_image_bytes`
- `max_inflated_bytes`
- `max_chunks`
- `max_chunk_bytes`
- `max_text_entries`
- `max_text_bytes`
- `max_unknown_chunks`
- `max_apng_frames`
- `max_temp_bytes`
- `max_conversion_expansion`
- progressive zlib work budgets per call

New explicit errors include:

- `PNG_DEC_ERR_DIMENSIONS_TOO_LARGE`
- `PNG_DEC_ERR_TOO_MANY_PIXELS`
- `PNG_DEC_ERR_INFLATED_TOO_LARGE`
- `PNG_DEC_ERR_CHUNK_TOO_LARGE`
- `PNG_DEC_ERR_TOO_MANY_CHUNKS`
- `PNG_DEC_ERR_TEXT_TOO_LARGE`
- `PNG_DEC_ERR_FRAME_LIMIT`
- `PNG_DEC_ERR_TEMP_MEMORY_LIMIT`
- `PNG_DEC_ERR_WORK_BUDGET`
- `PNG_DEC_ERR_CONVERSION_LIMIT`

See `png_strerror()` for string forms and `fuzz/` for the matching fuzz targets and seed corpus.
