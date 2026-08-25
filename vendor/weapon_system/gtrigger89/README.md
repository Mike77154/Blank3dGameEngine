# gtrigger89

Logical trigger state machine: normal, spin-up and charge-release.

- C89
- caller-owned/static state
- no malloc/calloc/realloc/free
- no float/double
- no OS dependency

See `include/gtrigger89.h` for the public API and `tests/` for a minimal regression.
