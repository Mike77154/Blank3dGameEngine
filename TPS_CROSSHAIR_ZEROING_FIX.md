# Blank3D v3.6.5 — Third-person crosshair zeroing

## Symptom

First-person zeroing was correct, but third-person projectiles could climb far
above the reticle or converge only at the weapon's maximum range.

## Cause

The camera is above and behind the physical muzzle. The v3.6.4 hemisphere
safety rebuilt invalid/near camera targets using their raw camera distance; a
small distance produced a steep muzzle-to-eye-height launch. When there was no
hit, the weapon's full range became the convergence distance, leaving the
projectile visibly offset from the reticle for too long.

## Fix

- Valid world hits in front of the muzzle remain exact.
- Empty TPS space uses a 32-unit zero plane on the rendered center ray.
- Targets at or behind the muzzle plane are rebuilt on that same stable plane.
- The projectile still spawns from the physical muzzle and keeps full collision,
  trail, casing, audio and weapon-family behavior.
- FPS zeroing and CamaraNaku RECEIVE_PROVIDER are unchanged.
