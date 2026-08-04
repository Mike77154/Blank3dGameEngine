# NationalMecanicanimal89 integration

## Vendor layout

```text
vendor/nationalmecanicanimal89/
├── include/nationalmecanicanimal89.h
├── src/nationalmecanicanimal89.c
├── tests/test_nm89.c
├── examples/pistol_provider_demo.c
└── upstream documentation and CC0 license
```

The Blank3D adapter is intentionally separate:

```text
src/blank3d_mechanical_weapon.h
src/blank3d_mechanical_weapon.c
```

No changes were required inside the upstream source.

## Responsibility boundary in v3.20.0

NationalMecanicanimal89 does **not** attach the weapon to the carrier. Its root
world matrix comes from the already-resolved child object owned by GAttach89:

```text
character Soquete3D socket
    -> GAttach89 equipped child object
        -> NationalMecanicanimal89 root world provider
            -> mechanical child parts
```

This lets the same animated object move from hand to back, hip, floor, another
character, or another compatible socket without rebuilding its internal rig.

## Runtime chain

```text
GWeapon89 player state
├── FIRE_ACCEPTED ───────────────► semantic mechanical fire action
├── reload_active ───────────────► magazine transform provider
├── reload_elapsed_ms ───────────► extraction/removal/insertion phase
└── current weapon id ───────────► reset on equipment change

GAttach89 equipped child object
└── resolved Q20.12 transform ───► NationalMecanicanimal89 root matrix

NationalMecanicanimal89
├── resolves hierarchy
├── applies clip and provider samples
├── enforces mechanical constraints
├── emits four geometry packets
└── resolves a non-rendered muzzle socket

engine_bridge.c
└── final Q16.16 matrix boundary ─► OpenGL/Giffany Shapes3D meshes
```

## Initial rig

```text
weapon_root                 world supplied by GAttach89 child object
└── recoil_carriage         constrained Z recoil and X kick
    ├── body                procedural body mesh
    ├── slide               fire clip, constrained travel
    ├── magazine            reload transform/visibility provider
    ├── barrel              fixed child mesh
    └── muzzle_socket       no mesh; independent systems consume it
```

Exactly four geometry packets are flushed: body, slide, magazine and barrel.
The muzzle is intentionally absent from the geometry packet set.

## Fire action

`GWP89_EVENT_FIRE_ACCEPTED` triggers `B3D_MECH89_ACTION_FIRE` with restart
policy. The seven-tick clip:

- moves the recoil carriage backward and returns it;
- rotates the carriage by a small X kick and returns it;
- cycles the slide;
- emits a `slide_rear` mechanical marker through the provider bus.

At a 16 ms mechanical tick, one cycle lasts roughly 112 ms. Re-triggering the
action restarts it, which also supports automatic weapons.

The clip does not draw a muzzle flash, smoke, gas, light, sound or casing. Those
remain consumers of the weapon event and the resolved muzzle socket.

## Reload provider

The real `GWP89_UserState` drives the magazine without a second reload timer:

- 0–30%: magazine extracts downward and slightly outward;
- 38–55%: magazine is hidden, representing the removed magazine;
- 55–100%: magazine reappears and returns to its home transform.

The provider can request any transform, but NationalMecanicanimal89 clamps the
result to the configured mechanical envelope before producing world geometry.

## Muzzle output

After hierarchy resolution, `blank3d_mechanical_weapon_muzzle_world()` exposes
the final world matrix of the invisible `muzzle_socket`. Blank3D republishes it
as the Soquete3D socket `player.muzzle`.

```text
NationalMecanicanimal89 muzzle_socket
    -> Soquete3D player.muzzle
        -> projectile / flash / gas / smoke / light / audio providers
```

## Renderer boundary

`bridge_gl_apply_q16_matrix()` is the decimal/OpenGL boundary. It converts the
final 4×4 Q16.16 matrix to a column-major `GLfloat` matrix. Rig evaluation,
interpolation, provider sampling and constraints remain fixed point.

## Build and tests

```bash
make test-gattach-vendor
make test-attachment-stack
make test-nationalmecanicanimal-vendor
make test-mechanical-weapon
make syntax-check
make audit
```

The attachment test verifies ownership and socket movement independently from
the rig. The mechanical test verifies the attached root transform, four
geometry packets, slide/recoil, all reload phases, constraints, the non-rendered
muzzle socket and exact return to the home pose.

See `GATTACH_OBJECT_ANIMATOR_INTEGRATION.md` for the complete carrier/object
architecture.
