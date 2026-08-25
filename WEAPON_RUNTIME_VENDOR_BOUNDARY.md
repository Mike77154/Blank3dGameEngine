# Weapon Runtime vendor boundary

This revision moves five reusable policies out of the Blank3D runner and into
`vendor/weapon_system/` while keeping engine-specific world services as providers.

## 1. gplayerprojectileaim89

Player/local-view only. A presentation camera chooses the target; the physical
projectile still launches from the real muzzle toward that target. The vendor core
knows no Cameranaku, Blank3D collision world, NPC or enemy type. A target-query
provider supplies the camera-ray result. NPCs/turrets do **not** enter this path.

## 2. gtrigger89

Owns temporal trigger policy after a logical `shoot` verb:

- normal: press/hold/release
- spin-up: delay before arming, then continuous hold
- charge-release: accumulate hold time and emit one fire pulse on release

DDSL2/input decides *what* means shoot. Audio/presentation reacts to the trigger
runtime events but does not own the timer.

## 3. gprojectilespawn89

Provider-orchestrated projectile creation. A weapon event is converted to an
agnostic request and dispatched through:

`world_spawn -> collision_register -> render_spawn -> audio_spawn -> world_publish`

Blank3D currently provides its fixed bullet pool, Aoi trail renderer, projectile
audio and locator publisher. Another engine can replace every provider.

## 4. gcasingruntime89

Provider-orchestrated casing emission:

`world_spawn -> physics_spawn -> render_spawn -> audio_spawn -> world_publish`

Blank3D connects VPhysics as the physics provider. Bounce, gravity, friction,
tumble and sleep remain physics-provider behavior rather than casing-runtime
platform assumptions.

## 5. gweaponsnapshot89

Fixed-capacity per-actor/per-frame snapshot store. Blank3D publishes the player
snapshot after final locomotion, equipment attachment update and muzzle republish.
The weapon update then consumes the exact same `frame_stamp` snapshot.

This prevents an actor from moving on frame N while its muzzle/weapon input still
comes from frame N-1. The store is generic and may also be populated by NPC or
network actor providers.

## Host responsibilities that intentionally remain outside

- OS/window/input backend
- concrete collision world implementation
- concrete render/audio/physics engines
- actor lookup and faction rules
- presentation camera implementation

The Weapon System owns the policies; the host owns the services.
