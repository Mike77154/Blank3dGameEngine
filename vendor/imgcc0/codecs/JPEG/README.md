# c89jpeg v1.2.0

JPEG/JFIF library in strict C89 with caller-owned memory and fixed-point transform paths.

Project rules:

- no `malloc/calloc/realloc/free` inside the library
- no floating-point image transform path
- baseline sequential Huffman decode/encode (`SOF0`)
- progressive Huffman **memory decode** (`SOF2`)
- source/sink, ROI, abbreviated-stream, and resumable baseline decode paths
- resumable baseline encoder output
- shared tables-only and abbreviated-image workflows
- source-input encode from a row callback
- fully resumable source-input encode state that combines partial row ingestion and partial output emission

## New in v1.2.0: progressive JPEG decode

The memory decode path now accepts progressive Huffman JPEG (`SOF2`). Progressive
images accumulate quantized DCT coefficients in a workspace supplied by the caller,
then run the existing fixed-point IDCT/color conversion after the final scan.

Implemented progressive scan classes:

- DC first
- AC first
- DC successive-approximation refinement
- AC successive-approximation refinement
- spectral selection (`Ss` / `Se`)
- successive approximation (`Ah` / `Al`)
- EOB runs
- restart intervals/markers between progressive MCUs
- DHT/DQT/DRI and legal metadata segments between scans

The decoder validates the progressive scan constraints and caps a file at
`C89JPEG_MAX_PROGRESSIVE_SCANS` (64) to bound pathological scan churn.

### Caller-owned progressive workspace

No heap was introduced. Query the coefficient-storage requirement after probing the
image and pass that workspace into the memory decode parameters:

```c
c89jpeg_decoder dec;
c89jpeg_image_info info;
c89jpeg_decode_params dp;
c89jpeg_u32 need;

c89jpeg_decoder_init(&dec);
if (c89jpeg_decoder_probe(&dec, jpg, jpg_size, &info) != C89JPEG_OK)
    return 0;

memset(&dp, 0, sizeof(dp));
dp.output_format = C89JPEG_PIXFMT_RGBA32;

if (info.progressive) {
    need = c89jpeg_decoder_progressive_workspace_size(&info);
    dp.progressive_workspace = progressive_storage;
    dp.progressive_workspace_size = need;
}

if (c89jpeg_decoder_decode(&dec, jpg, jpg_size, &dp,
                           rgba, rgba_size) != C89JPEG_OK)
    return 0;
```

`imgcc0` provides this workspace from its existing fixed temporary arena, so Blank3D
callers do not need a separate allocation API.

### Progressive API boundary

Progressive support in v1.2.0 is intentionally on the complete-memory decode and
memory-to-sink decode paths used by `imgcc0`. The resumable/source-input decoder is
still baseline-only because progressive input suspension needs transaction-safe
coefficient and entropy state across scan boundaries. The encoder also remains
baseline (`SOF0`) only.

## v1.1.0 replay layer

`c89jpeg_encode_source_resume_params` accepts caller-owned `replay_buffer` and
`replay_buffer_size`, with `c89jpeg_encoder_resume_replay_buffer_size()` providing
the required size. This lets optimal/custom Huffman source-resume encode collect
statistics without heap allocation.

## Build

```sh
make clean
make all
make test
make examples
```

Artifacts are written into `build/`.

## Validation highlights

- strict `-std=c89 -pedantic-errors -Wall -Wextra -Werror` build passes
- existing baseline/resumable JPEG regression tests pass
- matched baseline/progressive 4:2:0 regression decodes to byte-identical RGB
- Blank3D decodes the supplied 512x512, 10-scan progressive muzzle JPEG natively
- the muzzle test verifies both dark-corner and white-hot-center decoded pixels
- `imgcc0` protocol audit still reports no dynamic allocation, floating types, explicit 64-bit integer types, or C99 line comments in active C/header files
- optimal source-resume + replay still matches `c89jpeg_encode_source_memory()` byte-for-byte

## Current project scope

Supported:

- `SOF0` baseline sequential Huffman JPEG
- `SOF2` progressive Huffman JPEG on complete-memory decode paths
- 8-bit samples
- grayscale and YCbCr/RGB output paths already supported by the baseline decoder
- `4:4:4`, `4:2:2`, `4:2:0`
- fixed-point transform path
- caller-owned buffers only

Not implemented:

- progressive source/resume streaming decode
- progressive JPEG encode
- arithmetic coding
- lossless JPEG
- coefficient-domain transcode path
