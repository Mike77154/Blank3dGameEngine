# Gunner aim and reload fix — v3.9.3

## Aim ownership

CamaraNaku is the player camera provider. It now returns PASS for non-player
weapon actors, preserving the NPC socket/camera basis supplied by FPIL.

The gunner calculates a torso target, constructs the muzzle position, and then
recalculates the final normalized direction from the real muzzle to that target.

## Reload

The gunner uses the normal gweapon89 state machine:

1. `gwp89_try_fire()` consumes the internal clip.
2. Empty clip returns `GWP89_NO_AMMO`.
3. The NPC calls `gwp89_begin_reload()`.
4. Continued actor updates advance the reload timer.
5. `RELOAD_BEGIN` and `RELOAD_END` use the normal event/audio path.
6. Firing resumes automatically after the clip is refilled.

The actor owns an internal reserve bank initialized to 9999 rounds and does not
consume the player's GKInventory ammunition.
