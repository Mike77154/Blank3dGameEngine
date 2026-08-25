# TimeVerbs89 protocol89 audit

TimeVerbs89 is a vocabulary/lookup microvendor. It owns no clock, timers,
filesystem, renderer, OS integration or DSL runtime. The host maps resolved
verbs to its own time services.

The core is ISO C89, integer-only, no heap and no 64-bit or floating-point
requirements. `make test audit` is the standalone gate.
