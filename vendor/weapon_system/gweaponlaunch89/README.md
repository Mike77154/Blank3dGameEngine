# gweaponlaunch89

Portable weapon/projectile launch-boundary policy for the Weapon System.

This library owns the invariant that an emitted projectile must remain in the
visible camera hemisphere and must launch from the physical muzzle toward the
camera-selected target. It also delegates gravity/Bolt compensation to the
vendored `g3dweaponzeroing89` solver.

It does **not** know Blank3D, Win32, input, rendering, collision worlds, actor
movement, or heap allocation.

## Why this belongs in the Weapon System

A host engine should provide snapshots (camera, muzzle, target, speed, gravity)
and spawn whatever projectile backend it wants. It should not need to carry a
local patch for camera/muzzle convergence or reverse-hemisphere repair.

The regression that historically made projectiles visibly launch backward is
now tested here:

```
provider exports target/direction behind camera
        -> gweaponlaunch89
        -> g3dweaponzeroing89 hemisphere repair
        -> launch_direction remains in visible hemisphere
```
