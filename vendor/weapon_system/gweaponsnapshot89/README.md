# gweaponsnapshot89

Fixed-capacity actor/frame weapon and muzzle snapshot store.

- C89
- caller-owned/static state
- no malloc/calloc/realloc/free
- no float/double
- no OS dependency

See `include/gweaponsnapshot89.h` for the public API and `tests/` for a minimal regression.
