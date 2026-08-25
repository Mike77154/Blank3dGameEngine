# TickOClock89 protocol89 audit

The core is ISO C89, caller-owned/fixed-capacity state, integer-only and has no
platform clock dependency. It consumes delta milliseconds/frames from a host.

Forbidden core dependencies/tokens: dynamic allocation, float/double, long long,
int64_t/uint64_t and stdint.h. `make test audit` is the standalone gate.
