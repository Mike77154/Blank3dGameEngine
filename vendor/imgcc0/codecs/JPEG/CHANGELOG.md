# Changelog

## v1.2.0

### Added

- progressive Huffman JPEG (`SOF2`) decode on complete-memory and memory-to-sink paths
- caller-owned progressive coefficient workspace and `c89jpeg_decoder_progressive_workspace_size()`
- DC-first, AC-first, DC-refinement, and AC-refinement scan decoding
- spectral selection, successive approximation, EOB runs, and progressive restart handling
- a 64-scan safety cap (`C89JPEG_MAX_PROGRESSIVE_SCANS`)

### Changed

- SOF parsing now accepts both `SOF0` and `SOF2` for complete-memory decode
- `imgcc0` supplies progressive coefficient storage from its existing fixed temporary arena
- progressive images are reconstructed only after all scans are accumulated, using the existing fixed-point IDCT/color path

### Notes

- no heap allocation or floating-point path was introduced
- resumable/source-input progressive decode and progressive encode remain out of scope in v1.2.0

## v1.1.0

### Added

- `replay_buffer` and `replay_buffer_size` in `c89jpeg_encode_source_resume_params`
- `c89jpeg_encoder_resume_replay_buffer_size()`
- replay-backed table preparation for `C89JPEG_HUFFMAN_OPTIMAL` on the source-resume path
- replay-backed validation for `C89JPEG_HUFFMAN_CUSTOM` on the source-resume path
- `tests/test_c89jpeg.c`
- `Makefile`

### Changed

- `c89jpeg_encoder_resume_begin_source()` now accepts default, optimal, and custom Huffman modes
- the streamed source-resume state can emit prefix output before the replay image is fully available, then suspend at the table boundary and continue once replay-backed statistics are ready
- `examples/example_source_resume_encode.c` now demonstrates optimal Huffman with the replay layer

### Notes

- the replay layer is still caller-owned memory; the library continues to avoid heap allocation
- abbreviated/shared-table source-resume sessions continue to use the existing single-pass shared-table flow
