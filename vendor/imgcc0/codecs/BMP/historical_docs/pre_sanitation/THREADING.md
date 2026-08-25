# Threading and limits guidance

BMP 1.8.0 keeps the historical global limit setters for backward compatibility:

- `bmp_set_global_limits()`
- `bmp_get_global_limits()`

For concurrent services or multi-tenant hosts, prefer the new per-operation APIs:

- `bmp_parse_memory_with_limits()`
- `bmp_decode_to_rgba32_with_limits()`
- `bmp_decode_to_rgba32_alloc_with_limits()`

Passing a `bmp_limits` object per call avoids changing shared process-wide limit policy.
The per-call APIs do not mutate the global limit configuration.

## New in 1.8.0

- Workflow action refs pinned to immutable commit SHAs.
- Added GitHub policy auditing via `make policy-audit`.
- Added maintainer governance templates for CODEOWNERS and branch protection.
- Added dependency-review workflow for public-repository PRs.
