# Actor fire-gate isolation (v3.9.6)

## Runtime symptom

When the stationary gunner emptied its magazine, the player could no longer
fire and the gunner never entered a successful reload cycle.

## Actual cause

The two actors already had separate `GWP89_Manager` instances, but both
managers reused Blank3D's flags provider with the same `Blank3DSystems`
context. When enemy bullets reduced player health to zero, the runner wrote:

```c
blank3d_systems_set_flag(&g.systems, "weapon.can_fire", 0);
```

That flag was then read for every actor. The next NPC `try_fire()` was cancelled
at `FIRE_VALIDATE` before it could return `GWP89_NO_AMMO`; consequently the NPC
never called `gwp89_begin_reload()`.

The apparent magazine coupling was therefore an actor-flag coupling.

## Correction

- Player death is evaluated as an actor-local health gate inside the flags
  provider. It is no longer written into the shared FlagStore.
- Player gates remain:
  - `weapon.can_fire`
  - `weapon.can_reload`
  - `weapon.active_reload`
- NPC gates are independent:
  - `npc.weapon.can_fire`
  - `npc.weapon.can_reload`
  - `npc.weapon.active_reload`
- NPC active reload defaults to disabled; normal timed reload remains enabled.
- The one-shot player launch-speed override is now ignored for NPC actors.

`weapon.enabled` remains the global master switch shared by all actors.

## Regression test

```sh
make test-actor-weapon-isolation
```

The test performs the complete sequence:

1. NPC fires all 40 machine-gun rounds.
2. NPC receives `GWP89_NO_AMMO` and starts reload.
3. Player fires while the NPC reload timer is active.
4. NPC refills its own clip from its own reserve.
5. Player health is reduced to zero.
6. Player firing is cancelled locally.
7. NPC continues firing and can still reload.

This covers the exact runtime boundary that previous tests missed.
