# Gatling geometric projectile origin: bulletspin89 + bulletcircle89 + bulletinline89

## Intent

The Gatling is now authored as a composition of three generic Weapon System capabilities. None of the vendors contains weapon ID 8 or any Blank3D actor/camera type.

```text
accepted shot
    |
    v
bulletspin89          per shooter+weapon state
    |                 slot 0..N-1
    v
bulletcircle89        center + right/up + radius + slot
    |                 -> displaced physical origin
    v
bulletinline89        displaced origin + shared aim target
    |                 -> convergent direction
    v
gprojectilespawn89
```

## Ownership

- **bulletspin89** owns only slot sequencing state. Blank3D stores one fixed-capacity state per actor/weapon pair.
- **bulletcircle89** owns only circular geometry. It uses integer CORDIC, with no `sin`, `cos`, float or heap.
- **bulletinline89** owns only convergence from an arbitrary origin to a target point.
- **GWeapon / gprojectilespawn89** still own fire acceptance, ammo, cadence and projectile creation.
- **Blank3D aim providers** remain the authority for the target point.

## Gatling INI

```ini
[modules]
bullet_circle=1
bullet_circle_count=6
bullet_circle_radius=0.18
bullet_circle_phase=0.25

bullet_spin=1
bullet_spin_start=0
bullet_spin_step=1
bullet_spin_direction=clockwise

bullet_inline=1
bullet_inline_distance=125
```

`bullet_circle_phase` is measured in turns: `0.25` is a quarter turn. The Gatling starts at the top of the barrel ring and advances one slot clockwise for each accepted projectile request.

`bullet_inline_distance` is a fallback convergence distance. For player physical projectiles, Blank3D first resolves the camera-authoritative impact/aim point and bulletinline89 converges from the selected barrel to that point. If an exact target point is unavailable, the fallback point lies on the centerline at the configured distance.

## Player hitscan interaction

Any weapon enabling `bullet_circle`, `bullet_spin` or `bullet_inline` is kept on the physical projectile path. This is intentional: a circular muzzle origin must exist in gameplay space rather than only as a cosmetic tracer. Blank3D's projectile update uses swept collision, so high-speed Gatling rounds retain continuous segment collision.

## Generic uses beyond Gatling

The three modules may be combined or used independently:

- ring-shaped spell emitters;
- rotating missile pods;
- multi-barrel cannons;
- boss attacks emitted from a geometric ring;
- circular particle/projectile launchers;
- arbitrary off-axis muzzle sockets converging on a shared crosshair.

## C89/static-storage contract

All three vendors are C89, integer/fixed-point only, caller-owned/static state, and use no allocator.
