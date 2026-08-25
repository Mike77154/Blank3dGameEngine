# Weapon launch boundary extraction

## Extracted in this revision

The portable launch invariant now belongs to the Weapon System:

```
vendor/weapon_system/gweaponlaunch89/
vendor/weapon_system/g3dweaponzeroing89/
```

Blank3D no longer owns the algorithm that repairs a projectile whose provider
exports a target/direction behind the rendered camera. `src/blank3d_ballistics.c`
is now only a host adapter from `Blank3DWeaponModules` to `GWL89_PhysicsConfig`.

The vendor owns:

- camera/muzzle target reconciliation;
- forward-hemisphere repair;
- TPS stable zero plane and muzzle clearance;
- linear versus gravity/Bolt final launch solve;
- fixed-point ballistic compensation;
- diagnostic result flags.

This includes the historical regression where a projectile could visually fire
behind the player/camera after a provider-axis mismatch while the actor was
moving.

## Still in Blank3D and good next candidates

### 1. Same-frame weapon/muzzle snapshot

`sync_actor_equipment()` republishes final actor transforms after movement so
weapon equipment and the visual muzzle do not trail a walking actor by one
frame. The *policy* should become a provider-driven weapon-frame/snapshot helper;
Blank3D should only provide its actor/socket callbacks.

### 2. Projectile runtime/pool

`spawn_bullet_event()`, projectile slot allocation, mesh/trail/backend selection,
flight update and release are still host code. They can become a
`gprojectileruntime89` consumer of `GWP89_EVENT_PROJECTILE_REQUEST` with host
providers for transform, collision, draw/audio and optional physics backends.

### 3. Weapon-family recovery policy

`process_weapon_events()` still contains explicit Gatling and shotgun recovery
rules (`GATLING_WEAPON_ID`, `B3D_SHOTGUN_WEAPON_ID`). Those policies belong in
weapon profiles/providers rather than the engine executable.

### 4. Casing runtime adapter

The low-level casing libraries are already vendorized, but Blank3D still owns
part of the event-to-casing runtime glue. This can become another Weapon System
adapter while keeping the actual world/collision provider host-owned.

## Things that should remain host-side

- window title/status text;
- HUD presentation decisions;
- final OpenGL draw calls;
- the concrete Blank3D collision world;
- actor/entity lookup and world ownership;
- platform input and OS APIs.
