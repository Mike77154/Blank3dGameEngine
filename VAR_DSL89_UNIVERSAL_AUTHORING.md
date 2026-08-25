# VarDSL89 universal authoring layer

Blank3D keeps every domain system in its own role. The variable layer does not
replace NumSys, Flags, GFO, DDSL2, RPYL, FPIL, the FPSC-style language, INI
configuration, Thing, ECS, World, Scene, Actor, Soquete, GAttach, Mechanimer or
WeaponSystem.

It adds a small GameMaker-like authoring surface for changing state at runtime:

```text
health = 100
health -= 25
can_fire = false
alerted = true
foo = 3
var temp = 2
foo += temp
global.debug = true
```

The author does not need to know which storage owns the name.

## Roles

```text
GFO / DDSL2 / RPYL / FPIL / FPSC-style / INI configurators
                         |
                         | may feed/use the shared variable API
                         v
                     var_dsl89
               parsing / authoring syntax
                         |
                         v
                    var_runtime89
              scope + provider resolution
                  /          |          \
                 v           v           v
              NumSys       Flags      VarStore
              provider     provider   var_manager89
```

- `var_dsl89` parses assignments and mutations. It owns no state.
- `var_runtime89` resolves scopes, providers and fallback creation.
- NumSys remains authority for registered rich numeric systems.
- Flags remains authority for registered semantic state and dynamic booleans.
- `var_manager89` remains the storage for free/ad-hoc variables and event locals.

No specialized system is absorbed into another one.

## Scope mapping

Blank3D maps GameMaker-like variable scopes onto the runtime spine:

```text
var x        -> current GFO/event local frame
x / self.x   -> current Thing
 global.x    -> runtime global
```

`self` is a Thing owner, not an Actor. Doors, lights, elevators, enemies,
players and other runtime Things can therefore all own variables without being
actors.

The owner passed to the variable runtime is a nonzero opaque runtime key derived
from the Thing handle. The variable vendors themselves do not include or depend
on ThingSystem.

## Backend routing

For instance/global operations, `var_runtime89` asks providers by priority and
falls back to VarStore when no provider claims the name.

Blank3D currently binds:

1. NumSys provider: registered numeric type names.
2. Flags provider: existing flag keys; unknown boolean SETs become flags.
3. VarStore fallback: every other free variable.

Examples:

```text
health = 100              -> NumSys player.health when that numeric type exists
health -= 25              -> NumSys arithmetic
can_fire = false          -> existing FlagStore weapon.can_fire
alerted = true            -> Thing-scoped dynamic flag
foo = 3                   -> Thing-scoped dynamic VarStore value
var temp = 2              -> local event frame only
global.debug = true       -> global FlagStore boolean
```

Dynamic instance booleans are namespaced by Thing, for example:

```text
thing.1.alerted
thing.2.alerted
```

so two runtime objects can create the same author-facing name without collision.

## Numeric representation

`var_dsl89` / `var_manager89` use Q16.16. NumSys uses its own fixed-point scale
(1.0 = 1024). The Blank3D adapter performs the conversion at the provider
boundary. Neither vendor is changed to know about the other's representation.

No `float` or `double` is required for decimal authoring; decimal source text is
parsed directly into fixed-point.

## GFO integration

GFO directly exposes the new surface through script blocks:

```text
*create
script: vars
health = 100
health -= 25
alerted = true
foo = 3
var temp = 2
foo += temp
:
```

The local frame begins before a GFO lifecycle event and ends after the event, so
`var temp` remains visible throughout the block/event and disappears afterward.
Instance variables and provider-backed values persist according to their owning
systems.

Accepted GFO script language aliases are:

```text
vars
var
var_dsl89
gmlvars
```

The pre-existing DDSL2, RPYL and FPIL callbacks are unchanged. They keep their
specialized semantics. Any authoring/runtime adapter can call the shared
`blank3d_variables_*` / `var_runtime89` surface without acquiring NumSys or
FlagStore internals.

## GFO create lifecycle fix

While wiring event-local variables, an older GFO host bug was exposed:
`gfo_spawn()` executes `*create` synchronously, but the Blank3D object record was
previously marked alive only after spawn returned. Script/handler callbacks in
`*create` could therefore fail to resolve `self`.

The object record is now made visible during the transactional construction
window before `gfo_spawn()`. Any spawn failure rolls the record back. This is a
GFO lifecycle correctness fix and does not change ownership of Thing/ECS/Actor.

## Capacity and memory policy

Blank3D compiles the variable store with fixed capacities:

```text
VM89_MAX_GLOBALS=64
VM89_MAX_INSTANCES=128
VM89_MAX_INSTANCE_VARS=24
VM89_MAX_LOCAL_FRAMES=16
VM89_MAX_LOCALS_PER_FRAME=32
```

The variable runtime exists once in the engine. It is never embedded per Thing,
Entity, Actor, Scene node or ObjectDefinition.

On the 64-bit QA host this profile measures approximately:

```text
vm89_manager     644,488 bytes
vr89_runtime     645,096 bytes
Blank3DVariables 661,688 bytes  (includes 16 KiB script scratch/status)
```

This is fixed runtime state, not per-instance multiplication. On 32-bit MinGW
exact ABI sizes can differ.

## Restrictions

The new core/bridge follows the engine rules:

- strict C89
- fixed capacity
- no malloc/calloc/realloc/free
- no heap APIs
- no float/double
- fixed-point numeric paths
- provider-driven domain routing

## Preservation rule

This integration was built on the complete Class Spine tree. Before packaging,
every pre-existing file below `vendor/` was compared against the Class Spine
baseline by relative path and SHA-256.

```text
pre-existing vendor files: 3262
missing:                       0
changed:                       0
new variable-vendor files:    15
```

The only new vendor roots are:

```text
vendor/var_manager89
vendor/var_dsl89
vendor/var_runtime89
```

Soquete/world-truth T0, perception, GAttach, Mechanimer, weapon systems,
World3D, Scene3D and the rest of the ecosystem remain present and retain their
roles.
