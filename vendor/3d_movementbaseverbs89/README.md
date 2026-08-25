# 3d_movementbaseverbs89

A tiny engine-facing movement vocabulary for C89 projects.

## Design rule

- **Base verbs** say what geometric operation is requested: `move_forward`, `move_left`, `rotate_right`, etc.
- **Game verbs** say what the actor intends to do: `walk_forward`, `strafe_left`, `run_forward`, `fly_up`, etc.
- Game verbs default to base verbs, but both layers are providerable.

This means an editor, camera, transform component, ECS, physics controller, character controller, network predictor, root-motion system, or custom locomotion backend can own the actual movement while gameplay code keeps a stable vocabulary.

## Constraints

- ISO C89 source
- no `malloc`
- no `realloc`
- no `free`
- no heap ownership
- no `float`
- no `double`
- Q16.16 fixed-point values
- caller-owned/static structs only

## Fallback coordinate convention

The built-in reference backend is intentionally simple:

- forward/backward: +Z / -Z
- right/left: +X / -X
- up/down: +Y / -Y
- rotate forward/backward: pitch +/-
- rotate left/right: yaw -/+
- rotate up/down: roll +/-

A game engine should normally install a base provider and interpret these verbs in its own local/world transform convention.

## Provider flow

```text
game code
   |
   v
walk_forward(actor)
   |
   +--> game provider? ---- handled ----> locomotion / physics / animation / IK
   |
   `---- unhandled
          |
          v
      move_forward(actor, walk_step)
          |
          +--> base provider? ---- handled ----> engine transform/controller
          |
          `---- unhandled ----> tiny built-in Q16.16 reference transform
```

## Minimal use

```c
mbv89_context verbs;
mbv89_actor actor;

mbv89_context_init(&verbs);
mbv89_actor_init(&actor);

mbv89_walk_forward(&verbs, &actor);
```

## Engine integration

Install the base provider:

```c
mbv89_set_base_provider(&verbs, my_transform_provider, my_engine_component);
```

Return `MBV89_HANDLED` when your engine consumed the verb. Return `MBV89_UNHANDLED` to let the built-in fallback run.

Game systems can independently install a semantic provider:

```c
mbv89_set_game_provider(&verbs, my_locomotion_provider, my_character_controller);
```

A game provider can handle walking/running/flying itself, or return `MBV89_UNHANDLED` to inherit the default mapping into the providerable base layer.

## Default game mappings

```text
walk_forward   -> move_forward(walk_step)
walk_backward  -> move_backward(walk_step)
strafe_left    -> move_left(walk_step)
strafe_right   -> move_right(walk_step)
turn_left      -> rotate_left(turn_step)
turn_right     -> rotate_right(turn_step)
fly_up         -> move_up(fly_step)
fly_down       -> move_down(fly_step)
run_forward    -> move_forward(run_step)
run_backward   -> move_backward(run_step)
```

## Build test

```sh
gcc -std=c89 -pedantic -Wall -Wextra -Werror -Iinclude \
    src/3d_movementbaseverbs89.c tests/test_movementbaseverbs89.c \
    -o test_movementbaseverbs89
./test_movementbaseverbs89
```

## Optional Gamlib3D adapter

The core library remains independent of any engine math package. When Gamlib3D
is available, compile `adapters/gamlib3d/3d_movementbaseverbs89_gamlib3d.c`
and include its header. The adapter:

- binds `mbv89_actor.user` to a caller-owned `Transform`
- converts Q16.16 movement amounts to Gamlib3D Q20.12
- converts Q16.16 turns to Gamlib3D Q20.12 degrees
- supports flat FPS movement or full 6DOF movement
- uses Gamlib3D's local `-Z` forward convention without leaking it into the core

This keeps Gamlib3D a provider, not a mandatory dependency.
