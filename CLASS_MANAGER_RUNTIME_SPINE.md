# ClassManager89 -> GFO Object runtime spine

## Frozen responsibility chain

```text
ClassManager89
    type semantics / inheritance / C3 / methods / properties
        |
        v
GFO ObjectDefinition
    concrete game-object recipe and lifecycle
        |
        v
ThingSystem89
    runtime instance identity / reserve-publish lifetime
        |
        v
ECS89
    component composition
        |\
        | +--> World3D89 spatial/world facet
        | +--> Scene3D89 hierarchy/scene facet
        `----> ActorSystem89 optional gameplay-actor facet
```

Faction, Inventory, Equipment, Weapon, Soquete3D, GAttach, Mechanimer,
perception and the T0 world-truth stack remain separate systems.  ClassManager
does not absorb them.

## Blank3D integration rules

- There is one `Blank3DClassSystem` for the application/object registry.
- It persists across `Blank3DRuntimeSpine` World/Scene resets.
- Every cached `Blank3DObjectDefinition` stores one `cm89_class_h`.
- If an entity INI has no `[class]` section, its class name defaults to the
  object/entity `name` and derives from root class `Object`.
- Optional authoring syntax:

```ini
[class]
name=Humanoid
bases=Object
```

  Multiple bases are comma-separated and must already be registered.
- Blank3D compiles ClassManager with internal instances disabled.  A Thing is
  the runtime instance; ClassManager receives it as `CM89_VALUE_HOST_HANDLE`.
- The registry is unsealed while object definitions are being discovered and
  sealed after the scene/object rebuild.

## class_manager89 v0.2 hardening

- optional internal instance pool (`CM89_ENABLE_INTERNAL_INSTANCES`)
- external receiver dispatch: `cm89_call_bound` / `cm89_call_bound_super`
- external property get/set helpers
- generational class handles (stale handles rejected after slot reuse)
- seal/unseal authoring gate
- inherited classmethods receive the dynamic class, while `owner_class`
  still reports the defining class
- no malloc/calloc/realloc/free/heap API
- no float/double

## Blank3D capacity profile

The main Makefile uses:

```text
CM89_ENABLE_INTERNAL_INSTANCES=0
CM89_MAX_CLASSES=64
CM89_MAX_BASES=4
CM89_MAX_MRO=16
CM89_MAX_CLASS_MEMBERS=32
CM89_NAME_MAX=32
```

This keeps the class registry manager around 170 KB on the 64-bit QA host and
avoids a second runtime-instance pool.

## Regression targets

```text
make test-class-manager-vendor
make test-class-object-spine
make test-runtime-object-spine
make syntax-check
make audit
```
