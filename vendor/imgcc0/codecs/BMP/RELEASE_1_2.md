# BMP 1.2.0

## Highlights

- Added per-operation parse/decode APIs with explicit `bmp_limits`.
- Decode allocation path now checks decoded-size limits before allocating output.
- Added benchmark tool: `make benchmark`.
- Added GitHub Actions CI workflow with GCC/Clang matrix job.
- Added threading guidance for embedding in concurrent services.
