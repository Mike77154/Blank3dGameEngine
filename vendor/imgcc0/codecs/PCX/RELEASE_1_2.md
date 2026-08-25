# PCX 1.2.0

## Highlights

- Added per-operation decode/inspect APIs with explicit `PCXDecodeLimits`.
- Fixed indexed decode path to enforce parsed limit checks consistently.
- Added benchmark tool: `make benchmark`.
- Added GitHub Actions CI workflow with GCC/Clang matrix job.
- Added threading guidance for embedding in concurrent services.
