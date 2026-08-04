# Motion library selection for Blank3D v3.19.0

Three C89 candidates were reviewed from source, examples, tests and public APIs.
The selection criterion was not raw feature count: it was the cleanest fit for
Blank3D's fixed-point, no-heap, provider-oriented engine.

## Decision matrix

| Criterion | NRPA3D 0.1.0 | Mechanimer89 1.3.0 | NationalMecanicanimal89 0.1.0 |
|---|---|---|---|
| Numeric model | Q16.16 | Q8.8 | Q16.16 |
| Explicit 32-bit fixed type | yes | `long` (ABI-dependent) | yes, compile-time checked |
| Caller-owned/static storage | yes | yes | yes |
| Hierarchy/world matrices | engine resolves | built in | built in |
| Pivots and sockets | minimal/provider output | built in | built in plus providers |
| Static constraints | basic movement limits | limited | per-property lock/clamp/wrap |
| Dynamic constraints | no | no | provider-driven |
| Visibility tracks/providers | basic event visibility | yes | yes |
| Multiple geometry bindings | limited | yes | yes |
| Provider registry | simple output provider | one provider per service | up to eight multi-service providers |
| Clips/actions/events | deterministic clips | sequence-oriented | tracks, keys, easing, actions, events |
| Renderer dependency | none | none | none |
| Blank3D integration risk | low, but too small | medium | low |

## Why NRPA3D was not selected

NRPA3D is the smallest and easiest candidate. Its Q16.16 timeline is useful,
and its output-provider design is clean. However, it intentionally leaves
hierarchical resolution, pivots, sockets and richer mechanical constraints to
the host. Adopting it would mean rebuilding much of the missing machinery in
Blank3D before it could coordinate a multipart weapon or articulated prop.

## Why Mechanimer89 was not selected

Mechanimer89 already owns hierarchy, matrices, sockets and geometry bindings.
Its provider bridge is capable, but the fixed type is based on `long` and the
library uses Q8.8. `long` is 32-bit under MinGW32 and 64-bit on the Linux QA
host, so identical source has different storage and overflow behavior between
the two environments. Q8.8 also provides less transform precision than the
Q16.16 stack already used by Soquete3D and the selected engine modules.

## Why NationalMecanicanimal89 won

NationalMecanicanimal89 supplies the missing middle layer without becoming a
renderer or asset loader:

- signed 32-bit Q16.16 arithmetic;
- static rig storage and no allocator calls;
- parent-child hierarchy and world matrices;
- pivots, sockets and point transforms;
- lock, clamp and wrap constraints;
- dynamic transform, world, visibility, pivot, socket and constraint providers;
- multiple geometry bindings and a resolver/output boundary;
- keyframed tracks, easing, semantic actions and marker events;
- deterministic integer ticks;
- provider registry rather than one hardwired callback.

That makes it suitable not only for the current weapon demonstration, but for
future doors, machinery, multipart enemies, reload mechanisms and destructible
assemblies.

## Upstream validation

The unmodified vendor suite passes under strict C89 with warnings promoted to
errors. It verifies fixed-point math, constraints, hierarchy, providers,
geometry resolution, clips, events, sockets, visibility and action routing.
The measured `nm89_rig` size on the QA host is 37,016 bytes; the active rig is
stored inside the static global engine state, not allocated at runtime.
