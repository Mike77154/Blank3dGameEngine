# Blank3D runtime spine: World + Scene + GFO Object + Thing + ECS Entity + Actor

## Frozen ontology

Blank3D now treats these concepts as distinct systems rather than synonyms:

- **GFO Object**: authored GameMaker-like object definition. Owns GFO type/lifecycle program, defaults and host bindings. A definition is compiled once and may spawn many instances.
- **ThingSystem89 Thing**: runtime instance/lifecycle identity. Owns reserve -> publish -> destroy, generational stale-handle rejection, quarantine, locking, flags/namespaces/userdata.
- **ECS89 Entity**: composition identity. Owns component membership and lightweight references to the facets attached to a Thing.
- **World3D89 proxy**: spatial/world-management facet. It is not the canonical Entity identity.
- **Scene3D89 node**: scene/hierarchy facet. It is not the canonical Entity identity.
- **ActorSystem89 Actor**: optional gameplay-participant identity linked to an ECS Entity through an opaque entity key.

ActorSystem89 deliberately does not own faction, inventory, equipment, weapons, AI, player/enemy/ally classification, or host entity storage. Those are independent systems associated by IDs/providers.

## Runtime relationship

```text
GFO ObjectDefinition
    | compile once
    | spawn
    v
ThingSystem89 Thing     <- canonical runtime instance/lifecycle handle
    |
    +--> ECS89 Entity   <- canonical composition handle
    |       |
    |       +--> ObjectRef
    |       +--> ThingRef
    |       +--> WorldRef  --> World3D89 spatial proxy
    |       +--> SceneRef  --> Scene3D89 hierarchy node
    |       +--> NativePtr --> transitional host storage
    |       `--> ActorRef  --> optional ActorSystem89 actor
    |
    `--> publish only after required facets were created
```

Thing creation is transactional. Blank3D reserves the Thing first, creates ECS/world/scene facets, records references in ECS, and only then publishes the Thing. On failure, the partially-created facets are rolled back in reverse order.

## One manager per runtime

`Blank3DRuntimeSpine` owns one instance of each manager:

- one `TS89_System`
- one `ecs_world`
- one `w3d_world` plus one fixed world arena
- one `SM3D_Context`

Per-runtime Things do **not** embed any of these managers. A `Blank3DRuntimeInstance` contains only handles/references and transitional host metadata.

This is intentional protection against static-storage bloat. `SM3D_Context`, world arenas, ECS worlds and compiled GFO arenas are manager/definition state, never per-Actor or per-Entity state.

## GFO Object definition cache

The old host record mixed Object, Entity, native storage, source code, a large GFO arena and a GFO context per instance.

The new split is:

```text
Blank3DObjects
  definitions[]
    - INI path
    - Object defaults
    - GFO source
    - GFO arena
    - compiled gfo_ctx
    - ref_count

  entities[] / ObjectInstances
    - legacy subject id
    - Thing/runtime key
    - native pointer
    - definition slot
    - small init snapshot for compatibility
    - GFO instance handle
```

Two Things instantiated from the same INI/GFO share the same compiled ObjectDefinition. `gfo_spawn()` owns the single `create` lifecycle dispatch; the Blank3D wrapper no longer dispatches `create` a second time.

The current game has eight entity Object definitions and `B3D_OBJECT_MAX_DEFINITIONS` is eight. This limit is intentionally compile-time/fixed-storage and can be raised deliberately when the content catalog grows.

## ActorSystem89 v0.2 boundary

An Actor record contains only generic linkage/state:

```text
actor_id
owner_entity_key   (opaque packed ECS Entity key in Blank3D)
user_ref           (opaque host mapping during transition)
alive / visible    (queryable/provider-backed generic state)
generic flags
```

Removed from ActorSystem:

- player/enemy/ally/NPC kinds
- team/faction ownership
- armed/targetable gameplay semantics
- inventory/equipment/weapon ownership
- direct dependency on ECS89, GFO, World3D89, Scene3D89 or Blank3D Enemy types

`owner_entity_key` is deliberately opaque so ActorSystem89 remains usable with a different Entity implementation.

## Faction / Inventory / Equipment / Weapon

These stay parallel systems keyed by Actor identity:

```text
Actor #N
  |
  +--> FactionSystem      relationship / friend-hostile-neutral policy
  +--> Inventory provider
  +--> EquipmentSystem89
  `--> WeaponSystem       via providers/hooks
```

ActorSystem never answers `is_enemy()` or `is_ally()`.

## Transitional native storage

`g.player` and `g.enemies[]` remain existing host storage for now. The runtime spine links them through `NativePtr` and provider callbacks instead of pretending those arrays are the Entity ontology.

This makes migration incremental:

1. establish canonical Thing/ECS/Actor relationships;
2. keep existing stable gameplay storage working;
3. move individual data domains into components/providers later without changing IDs again.

Legacy subject IDs (`1`, `100+i`, etc.) remain only as compatibility keys for old callbacks. They are not the canonical runtime instance identity.

## World and Scene role

World3D89 is currently used as the spatial/world proxy manager for runtime Things. Scene3D89 is used as hierarchy/scene-node management. Existing Blank3D rendering/collision paths are still host providers during the transition; integration does not silently make World3D89 or Scene3D89 authoritative for every renderer/collider subsystem.

A host pose provider synchronizes final actor/native transforms into World3D89 proxies and Scene3D89 nodes after gameplay movement.

## Soquete3D

Soquete3D remains a pose/socket/locator provider. Its historical use of the word `thing` is not promoted to canonical lifecycle identity. Future glue may map `TS89_Thing` to Soquete keys, but ThingSystem89 remains the lifecycle authority.

## QA invariants

The engine now has dedicated tests for:

- ThingSystem89 context isolation and lifecycle
- ActorSystem89 agnostic Actor <-> opaque Entity linkage
- runtime spine `Thing -> ECS -> World/Scene -> optional ActorRef`
- GFO ObjectDefinition sharing across multiple runtime Things
- per-instance size guards to stop manager/arena state from creeping back into records
- EquipmentSystem and faction integration remaining separate
- Win32 input syntax and active no-heap audit

The Scene3D89 subtree destruction path also validates the node index against the configured node capacity before dereferencing it.

## Fixed-storage footprint rule

The architectural rule is:

```text
large manager/arena state: once per runtime or once per ObjectDefinition
small handles/refs:        per Thing / ECS Entity / Actor
```

Never place `SM3D_Context`, `w3d_world` arenas, an `ecs_world`, a `TS89_System`, or a compiled GFO arena inside a per-Entity/per-Actor/per-Thing record.
