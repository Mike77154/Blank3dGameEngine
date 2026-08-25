# 3DKin-GF + gameverbs89 integration

## Purpose

This layer improves Blank3D game verbs without creating a new movement authority.

`3DKin-GF` answers contextual kinematic questions (free placement, floor, wall,
ceiling, overlaps). `gameverbs89` exposes those questions and movement actions as
one provider-driven named vocabulary that authoring languages can consume.

The runtime chain is:

```text
DDSL2 / FPIL / RPYL / GFO / FPSC-style aliases
                  |
                  v
             gameverbs89
          condition/action bus
             /          \
            v            v
       3DKin-GF      MovementBaseVerbs89
       conditions      game actions
            \            /
             v          v
          contextual movement preflight
                  |
                  v
               GLOCO89
          locomotion controller
                  |
                  v
        Collision / VPhysics final
                  |
                  v
            host Transform
                  |
           +------+------+
           v      v      v
         World  Scene  Soquete T0
```

3DKin-GF is **not** the final collision solver and never owns the canonical
Transform. Its walk/run/strafe preflight is advisory/contextual; Collision and
VPhysics remain final physical authorities.

## Shared condition vocabulary

The Blank3D 3DKin adapter publishes, among others:

- `place_meeting` / `placeMeeting`
- `place_free` / `placeFree`
- `instance_place` / `instancePlace`
- `is_on_floor` / `isonfloor` / `isOnFloor`
- `is_on_wall` / `isonwall` / `is_by_wall` / `isByWall`
- `is_under_ceiling` / `isUnderCeiling`
- overlap aliases
- `can_walk_forward` / `canWalkForward`
- `can_walk_backward`
- `can_strafe_left`
- `can_strafe_right`
- `can_run_forward`
- `can_run_backward`

The `can_*` predicates test a hypothetical local-flat displacement and never
mutate the real Transform.

## Shared movement actions

MovementBaseVerbs89 actions registered on the same bus include the canonical
names and compatibility aliases for:

- `walk_forward`, `walk_foward`
- `walk_backward`, `walk_back`
- `strafe_left`, `strafe_right`
- `run_forward`, `run_foward`
- `run_backward`, `run_back`
- `move_forward`, `move_foward`
- `move_backward`, `move_back`
- `move_left`, `move_right`

Player actions go through the normal MovementBaseVerbs provider, then 3DKin
preflight, then GLOCO. Actor/NPC actions use the actor-specific GLOCO binding but
keep the same preflight rule.

## DSL authoring

### DDSL2

GameVerb conditions are exposed to the DDSL store before execution and handled
GameVerb actions are attempted before the legacy action fallback.

```text
If can_walk_forward then walk_forward
If is_on_floor then run_forward
```

### FPIL / FPSC-style aliases

FPIL condition/action dispatch checks the GameVerb bus first, then preserves the
existing FPIL host fallback. Therefore the same names can be used by AI scripts.

Conceptually:

```text
:can_walk_forward:walk_forward
```

Legacy FPSC-style aliases already routed through FPIL keep their existing role;
this integration does not replace the FPIL vendor.

### RPYL

RPYL keeps its vendor untouched. Blank3D registers two generic host commands:

```text
verb walk_forward
gameverb walk_forward
```

This avoids hardcoding every future provider verb in the RPYL compiler.

### GFO

GFO host invocation now attempts the shared action bus after its existing native
handlers. A known GameVerb action name therefore reaches the same runtime path
without making GFO depend directly on 3DKin or GLOCO.

## Identity and ownership

3DKin object IDs and GameVerb registry slots are internal adapter state. They are
not Thing, ECS Entity, ActorSystem, World3D, Scene3D, or Soquete identities.

The owning subject is resolved from the current Blank3D Thing/native actor
binding. The canonical runtime identity remains Thing.

## Fixed point and allocation

- 3DKin-GF: Q24.8 internally.
- Blank3D Transform: Q20.12.
- Conversion is isolated in `blank3d_kinverbs.c`.
- No malloc/calloc/realloc/free.
- No heap APIs.
- No float/double.
- No long long.

## Provider extension rule

`gameverbs89` is intentionally generic. A future vendor may register a condition
or action without modifying DDSL2, FPIL, RPYL, or GFO. If the shared bus handles
it, the DSL receives the vocabulary; otherwise the existing language-specific
fallback remains active.

This is the key boundary: **providers expand vocabulary; languages keep their
own syntax and semantic roles.**
