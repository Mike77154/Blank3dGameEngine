# Shotgun runtime projectile fix — v3.6.7

## Runtime evidence

The v3.6.6 manager-level tests proved that seven `PROJECTILE_REQUEST` events
existed, but the real Win32 runner still displayed one ordinary projectile.
The missing coverage was the final boundary between the event bus and the
physical projectile pool.

## Corrections

1. `blank3d_shotgun.c` is now the final shotgun projectile provider.
2. Every shell is finalized as seven independent physical pellets.
3. The center of the pattern is obtained from the real center-HUD ray.
4. Every pellet receives its own deterministic angular offset around that ray.
5. The projectile launches from the physical muzzle toward its individual
   target on the zeroing plane.
6. Every pellet is forced to use projectile/mesh ID 3 from `gbulletmesh89`.
7. Shotgun projectile lifetime is at least 2800 ms.
8. Mesh scale is kept readable at a minimum of 0.36 in Q20.12.
9. A missing-pellet bitmask recovers only event indices lost by a provider or a
   saturated event queue; normal seven-event output is never duplicated.

## Runtime path

```text
one shotgun shell consumed
        |
        v
FIRE_ACCEPTED pellet_count=7
        |
        v
PROJECTILE_REQUEST indices 0..6
        |
        v
runner missing-index recovery
        |
        v
center-HUD raycast
        |
        v
seven independent muzzle-to-target directions
        |
        v
seven projectile pool slots
        |
        v
lifetime + CCS/SICOL sweep + Aoi Trail
```

## Validation

`make test-shotgun-runtime` verifies:

- seven unique directions;
- left, right, upper and lower pellets;
- every direction remains in the visible forward hemisphere;
- pellet mesh/projectile ID 3;
- lifetime of at least 2800 ms;
- manager-supplied single-projectile metadata is repaired at runtime.

The complete active Win32/OpenGL source also passes `make syntax-check` under
strict C89 declarations.
