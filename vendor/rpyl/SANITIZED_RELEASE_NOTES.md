# RPYL Lib Lang — sanitized ExtLang release

SPDX-License-Identifier: CC0-1.0

This release keeps the existing C89, fixed-storage architecture and the public
ExtLang/PolySym workflow while making `init <language>:` failures explicit and
testable.

## ExtLang hardening

- Added `rpyl_set_error(...)` as a public host/module diagnostic hook.
- Missing language plugins now report the language, file, and source line.
- A failing plugin callback now produces a useful fallback diagnostic.
- Plugin callbacks may provide their own detailed diagnostic through
  `rpyl_set_error(...)`; ExtLang preserves it.
- Lines longer than `RPYL_EXTLANG_MAX_LINE` are rejected instead of silently
  truncated.
- `init` priorities are parsed with checked arithmetic and reject values outside
  the C `int` range.
- Language names that are invalid or too long are rejected during registration.
- Top-level malformed `init` headers receive a direct diagnostic.
- Non-blank `init` body lines require at least
  `RPYL_EXTLANG_BODY_INDENT` spaces (default: 4).
- Tabs in `init` indentation are rejected to prevent ambiguous dedentation.
- Script, init-block, block-count, file-open, file-read, and file-size limits now
  report specific errors.
- Parser errors after ExtLang extraction retain the caller-provided filename.

## Compatibility

The API remains source compatible. `RpylLangHost` is still opaque, existing
registration and loading functions are unchanged, and `rpyl_set_error(...)` is
an additive API.

Behavior is intentionally stricter for malformed input. Scripts that relied on
silent line truncation, one-to-three-space init bodies, tab-indented init code,
out-of-range priorities, or malformed top-level `init` lines must be corrected.

## Tests added

`tests/test_extlang_sanitized.c` covers:

- Stable priority ordering, including negative and tied priorities.
- Four-space dedentation and nested indentation preservation.
- Missing plugins and callback failures.
- Plugin-supplied diagnostics.
- Priority overflow rejection.
- Long-line rejection without truncation.
- Tab and short-indentation rejection.
- Maximum init-block and init-code limits.
- Language registration validation.
- Preservation of the caller filename on downstream parser failure.

## Validation performed

```sh
make check
make strict-core
make test
```

The dedicated ExtLang suite was also compiled and executed with GCC's undefined
behavior sanitizer.
