# Migration to caller-owned storage

## Encoding

1. Initialize `bmp_encode_options`.
2. Ask for the required workspace size.
3. Provide a static/application-owned output buffer and workspace.
4. Call the corresponding `*_into` encoder.

No codec function takes ownership of either buffer.

## Decoding

1. Parse from the caller-owned input bytes.
2. Call `bmp_calc_rgba32_buffer_size`.
3. Verify the returned 32-bit size fits your static arena.
4. Call `bmp_decode_to_rgba32` with a caller-owned output buffer.

## Embedded payloads

Use `bmp_copy_embedded_payload_into`; the destination and capacity are supplied by the caller.

## Overflow policy

Dimensions, strides, offsets, file sizes and workspace sizes are checked in 32-bit arithmetic. A result that cannot be represented by the 32-bit contract fails with `BMP_ERR_OVERFLOW` instead of silently wrapping.
