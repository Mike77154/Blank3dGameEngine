# Blank3D full-stack integration notes

## Authority map

| Domain | Authoritative system |
|---|---|
| Player/world transform | Gamlib3D `Transform` |
| Named body, camera, aim and muzzle positions | Soquete3D |
| Health and weapon multipliers | NUMSYS |
| Runtime permissions and feature gates | FlagStore |
| Weapon ownership, reserve ammo and per-weapon magazines | GKInventory adapter |
| Fire/reload/cadence/profile/event orchestration | `gweapon89_manager` |
| HUD bars | gbar89 |
| Weapon audio synthesis and PCM generation | `weapon_synth_sound_engine89` |
| Bullet sweep, world raycast and GWP89 raycast service | SICOL-DE + CCS |
| Runtime configuration | conf_total |

No subsystem shadows reserve ammunition. The weapon manager queries and consumes
ammo through the GKInventory provider. Magazine counts are stored by weapon id in
the same adapter, so they survive equipment changes.

## Weapon data flow

```text
mouse/keyboard + camera vectors + muzzle socket
                       |
                       v
                 GWP89_FireInput
                       |
                       v
                gweapon89 manager
        +--------------+--------------+
        |              |              |
 numeric query     flag query    inventory query/mutation
        |              |              |
        +--------------+--------------+
                       |
                       v
                  GWP89_Event
        +--------------+--------------+
        |              |              |
 projectile       muzzle/recoil      audio event
        |                                  |
        v                                  v
 SICOL swept shape                  superengine voices
 + CCS raycast                      report/reload/casing/
 provider                           projectile/impact/blast
```

The runner consumes projectile, muzzle, casing, trail, mesh assignment, visual
modifier, ammo, reload, weapon-change, active-reload, and cancellation events.
Projectile metadata now selects one of eight specialized provider meshes from
`gbulletmesh89` or `rocketmeshes` and retains the manager-provided scale, radius,
velocity, damage and trail id. Each mesh is normalized to local `+Z`; its runtime
pose is constructed from the projectile's effective velocity, so yaw, pitch and
shotgun spread all point along the actual trajectory.
Casings have a static pool with gravity/bounce. Grenades and rockets apply radial
damage and route their blasts through the synthesizer. Weapon id 8 is a provider-
managed Gatling profile with a persistent 300-round belt, 900-round default
reserve, 35 ms automatic cadence, dedicated ballistic mesh and a host-side
360 ms spin-up gate.



## Projectile mesh authority

```text
weapon event mesh_id
        |
        v
blank3d_projectile_mesh provider
        |
        +-- 1 pistol .............. gbulletmesh89 pistol projectile
        +-- 2 machine gun ......... gbulletmesh89 machine-gun projectile
        +-- 3 shotgun ............. gbulletmesh89 single pellet
        +-- 4 magnum .............. gbulletmesh89 magnum projectile
        +-- 5 sniper .............. gbulletmesh89 sniper projectile
        +-- 6 grenade launcher .... rocketmeshes 40 mm LV grenade
        +-- 7 rocket launcher ..... rocketmeshes RPG-7 cone projectile
        +-- 8 Gatling ............. alias of mesh 2 machine-gun projectile
```

The shotgun manager emits one projectile event per pellet, so mesh id 3 builds
one pellet—not a seven-pellet cluster. Spread values are interpreted as degrees and
converted to a bounded frontal cone; right/up offsets can no longer erase forward. The Gatling follows the same authoritative
`GWP89_EVENT_PROJECTILE_REQUEST` path as every other firearm. If the fixed bullet
pool is saturated, it reuses the oldest non-explosive Gatling tracer; it never
overwrites a live grenade or rocket.

## Gatling continuous-audio path

```text
trigger press
    -> WSSE89_EVENT_GATLING_START
       -> M134D motor + medium six-tube rotor + spin-up whistle
360 ms armed
    -> WSSE89_EVENT_GATLING_FIRE_START
       -> firing load + continuous whistle + belt feed
each accepted shot
    -> heavy weapon report + projectile + rifle-brass casing
trigger release / reload / weapon switch / dry fire
    -> belt stop + WSSE89_EVENT_GATLING_FIRE_STOP
       -> synthesized spin-down tail
```

The continuous Gatling voice is stateful and separate from the per-shot report
voices, so rapid shots do not restart the motor or rotor.

## Audio polyphony policy

The WinMM source owns eight static PCM buffers and every weapon sound is dispatched
through `weapon_synth_sound_engine89`. Reports use a fixed pool. When every report
voice is occupied, `gsynthreport89` stops and reuses the oldest report tail instead
of returning `GV89_NO_VOICE`; the newest muzzle transient therefore always plays.
Reload, mechanism, casing, projectile, impact, grenade and rocket voices retain
their separate fixed pools. No sound path allocates memory or starts an audio thread.

## Active sockets

```text
player
├── player.body
├── player.weapon
├── player.muzzle
├── player.aim
└── camera.chase

enemy.N
├── enemy.N.body
└── enemy.N.head
```

Bullets are published as named things (`bullet.N`) because they do not require
child sockets.

## Camera

Mouse-look is captured by default and samples displacement from the client-area
center every frame. Yaw and pitch are camera state; yaw is copied to the player
heading so movement and firing remain aligned. Pitch is clamped and participates
in view-forward, muzzle-forward, recoil, FPS eye position, and TPS chase look-at.
`V` changes presentation, not aiming semantics. `Ctrl` interpolates the FOV and
passes the resulting zoom factor into `GWP89_FireInput`.

Script reload calls `load_rpy()` and then reapplies the camera configuration, so
F5/hot reload cannot silently reset mouse sensitivity or FPS/TPS offsets.

## Fixed-point formats

```text
NUMSYS:           Q10
conf_total:       Q16.16
Gamlib3D:         Q20.12
Soquete3D:        Q16.16
Giffany Shapes3D: Q16.16
GWeapon89:        fixed-point `long` ABI
```

`engine_bridge.c` owns renderer conversion. `blank3d_systems.c` owns NUMSYS and
weapon-manager conversion. No gameplay object stores `float` or `double`.


## Collision path

```text
GWP89_SERVICE_RAYCAST
        |
        +--> CCS world raycast/broadphase
        |
        +--> SICOL-DE world raycast
                 |
physical projectile frame step
        |
        +--> SICOL-DE swept sphere (primary)
        |
        +--> CCS ray fallback (confirmation)
```

Enemy capsules are synchronized every frame. Ground and a long-range backstop are
registered as static world shapes. This avoids tunnelling when sniper and
machine-gun projectiles cross an enemy between rendered frames.


## Dedicated casing catalog (v3.3)

`projectile_mesh_id` and `shell_mesh_id` now select different fixed catalogs:

```text
1 pistol shell
2 machine-gun shell (also Gatling)
3 shotgun hull
4 Magnum shell
5 sniper shell
```

Grenade and rocket profiles do not emit conventional brass. The renderer consumes
the shell scale supplied by the weapon profile instead of drawing one generic cylinder.

## Universal weapon composition (v3.4)

The runner now resolves optional behavior through `Blank3DWeaponModules`:

```text
weapon profile event
        |
        v
composition recipe
├── trigger: standard / spin-up / charge-release
├── physics: linear / gravity / Bolt3D
├── projectile mesh or primitive
├── casing/muzzle enabled or suppressed
├── Aoi Trail profile
└── optic + sway + aim + scope preset
```

This keeps `gweapon89` authoritative for inventory-facing weapon rules while
allowing non-firearms to use the same event ABI. The first proof is weapon id 9,
a slingshot: hold/release input produces an absolute launch-speed override,
consumes one `sling_stones` item, emits one primitive stone, and transfers motion
to Bolt3D with gravity, drag, bounce and mass. It emits neither muzzle nor casing.

## Native sniper path (v3.4)

Weapon id 5 activates the actual split sniper libraries rather than local FOV/HUD
stand-ins. `Ctrl` drives `gtelescopiczoom89`; `Shift` drives the breath-hold input
of `gsway89`; `gaimquery89` queries the existing CCS/SICOL provider; and
`gsniperhud89` plus the selected `gscopepresets89` vector preset emit the overlay.
Sway yaw/pitch feed the camera and projectile direction.

## Native Aoi Trail path (v3.4)

Every recipe may select bullet, tracer, heavy or arc trail profiles. Projectiles
attach to a fixed Aoi Trail3D89 slot, emit Q8.8 temporal points as they move, and
build renderer-facing ribbon triangles. When all trail slots are occupied, the
adapter recycles them round-robin and detaches the old bullet mapping safely.
