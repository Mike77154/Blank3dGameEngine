# Header hygiene and API surface (1.16.0)

This round introduces a single installed public header: `include/bmp/bmp.h`.

## Goals
- Keep the install tree stable and small.
- Stop shipping implementation-layer headers by default.
- Make the public umbrella self-contained.
- Verify install-time API surface with an automated audit.

## Policy
- Installed consumers should include `<bmp/bmp.h>`.
- Source-private headers remain in the repository for internal compilation only.
- Examples and package consumers must not include private headers.
