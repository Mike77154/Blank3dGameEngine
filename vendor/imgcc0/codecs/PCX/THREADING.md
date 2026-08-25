# Threading and limits guidance

PCX 1.8.0 keeps the historical global limit setters for backward compatibility:

- `pcx_set_global_decode_limits()`
- `pcx_get_global_decode_limits()`

For concurrent servers, plugins, or libraries embedded in larger processes, prefer the new per-operation APIs:

- `pcx_load_fp_ex()` / `pcx_load_file_ex()` / `pcx_load_memory_ex()`
- `pcx_load_fp_indexed_ex()` / `pcx_load_file_indexed_ex()` / `pcx_load_memory_indexed_ex()`
- `pcx_inspect_*_with_limits()` and `pcx_inspect_*_info_with_limits()`

Passing a `PCXDecodeLimits` object per call avoids shared mutable state for limit policy.
The per-call APIs do not modify the global limit configuration.

## New in 1.8.0

- Workflow action refs pinned to immutable commit SHAs.
- Added GitHub policy auditing via `make policy-audit`.
- Added maintainer governance templates for CODEOWNERS and branch protection.
- Added dependency-review workflow for public-repository PRs.
