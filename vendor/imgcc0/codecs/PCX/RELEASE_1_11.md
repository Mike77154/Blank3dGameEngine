# Release 1.11.0

## Focus
API surface hygiene and install-tree header discipline.

## Highlights
- New installed public umbrella header at `include/pcx/pcx.h`.
- Internal headers are no longer installed by default.
- Added `tests/test_public_api.c` for public-header smoke coverage.
- Added `scripts/audit_api_surface.py`, `make install-api-smoke`, and `make api-surface-audit`.
