# Blank3D v3.9.7 - Enemy self-hit and friendly-fire filter

## Runtime defect

NPC projectiles were sent through the same `ENEMY | WORLD` swept-sphere
query as player projectiles. The gunner muzzle is close to its own capsule.
At some aim angles fixed-point rounding made the owner's capsule the nearest
hit, so its machine-gun rounds reduced its own HP until `alive` became zero.
This looked like view-angle culling, but the entity was actually killed.

## Fix

- Added `blank3d_collision_sweep_bullet_mask()`.
- Player projectiles use `ENEMY | WORLD`.
- Team-2/NPC projectiles test the player separately, then sweep only `WORLD`.
- NPC projectiles therefore cannot hit their owner or allied enemies.
- Gunner muzzle forward clearance increased from 0.80 to 1.10 units.
- Existing `blank3d_collision_sweep_bullet()` remains as a compatible wrapper.

## Regression

`make test-collision` now reproduces a projectile starting beside the owner
capsule and confirms that the enemy mask detects it while the NPC world-only
mask does not.
