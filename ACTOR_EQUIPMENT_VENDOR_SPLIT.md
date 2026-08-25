# ActorSystem89 + EquipmentSystem89 vendor split

## Goal

Blank3D should host actor/equipment services rather than own reusable policy.
The split deliberately keeps both vendors independent.

## actor_system89

Owns only lightweight actor identity/state:

- actor id
- kind
- host index
- team id
- alive/visible baseline state
- generic flags

Host-specific information is queried through providers:

- current alive state
- current visibility
- current position (opaque host fixed-point scalar; no arithmetic in core)
- locator/socket-system key

Blank3D registers the player and RPYL-created actors. Existing common helpers
for actor lookup, alive, position and locator now route through ActorSystem89.
Concrete `Enemy`, HP, Transform, AI, faction and perception state remain host
components.

## equipment_system89

Owns generic equipment lifecycle only:

- actor -> item association
- equip/unequip
- object instance id allocation from fixed storage
- socket name / attachment name / model metadata
- visibility
- runtime sync/trigger routing

Everything concrete is a provider:

- item descriptor resolution
- socket definition
- attach/detach/visibility
- object transform retrieval
- runtime init/sync/trigger
- presentation packet and muzzle query

`blank3d_actor_equipment` is now an adapter that binds EquipmentSystem89 to:

- GWeapon presentation profiles
- GAttach89
- GMechanicalWeapon89
- GWeapon89 manager state

The vendor itself does not include or depend on any of those libraries.

## Independence

`actor_system89` does not depend on `equipment_system89`.
`equipment_system89` does not depend on `actor_system89`.
A host can use either one alone, or connect them through its own policies.

## Constraints

Both vendors are strict C89, fixed/static storage, no malloc/calloc/realloc/free,
and contain no OS API, renderer, physics, weapon, AI, or engine backend.
