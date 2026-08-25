# PROVIDERS

`gloco89` keeps its original built-in locomotion and legacy `GLOCO_WorldProbeFn`, but now each movement/physics stage can be delegated to host code without heap ownership or engine dependencies.

## Fallback rule

Every provider callback that returns `int` follows one rule:

- `GLOCO_PROVIDER_HANDLED`: the provider handled that stage; gloco89 does not run the built-in implementation for it.
- `GLOCO_PROVIDER_FALLBACK`: run the original built-in/legacy path.

A provider can therefore implement only one stage and leave every other stage untouched.

## Movement provider

```c
GLOCO_MovementProvider movement;

gloco_movement_provider_init(&movement);
movement.horizontal = my_horizontal;
movement.vertical = my_vertical;
movement.integrate = my_integrate;
movement.impulse = my_impulse;
movement.stop = my_stop;

gloco_set_movement_provider(&ctx, &movement);
```

Stages:

1. `impulse`: discrete locomotion impulse. Currently used for evade. Return fallback to preserve the original evade impulse.
2. `horizontal`: receives `GLOCO_MotionIntent` and may write `actor->vel.x/z` and `actor->facing`.
3. `vertical`: may own gravity, jumping or another vertical controller by writing `actor->vel.y`.
4. `integrate`: may replace `pos + vel * dt` and output the requested next position.
5. `stop`: notification/synchronization hook for `gloco_actor_stop()`.

`GLOCO_MotionIntent` exposes the locomotion policy calculated by gloco89: desired direction, target velocity/speed, acceleration, friction, turn rate, buttons and mode (`MOVE`, `BRAKE`, `HARD_STOP`, `EVADE`, `SLIDE`).

If `horizontal` returns handled, gloco89 still maintains its state machine, flags, stamina, timers and events. The host provider only owns the mechanical motion stage.

## Physics provider

```c
GLOCO_PhysicsProvider physics;

gloco_physics_provider_init(&physics);
physics.user = my_world;
physics.move = my_move_body;
physics.actor_create = my_body_create;
physics.actor_destroy = my_body_destroy;
physics.teleport = my_body_teleport;
physics.post_move = my_post_move;

gloco_set_physics_provider(&ctx, &physics);
```

`physics.move` receives `from` and `to` plus the actor/profile and fills the existing `GLOCO_ProbeResult`:

- `corrected_pos`
- `ground_normal`
- `GLOCO_FLAG_GROUNDED`
- `GLOCO_FLAG_BLOCKED`
- `GLOCO_FLAG_STEEP`

It may also modify actor velocity, for example after wall collision.

Lifecycle hooks allow an external physics backend to keep an engine body synchronized with gloco actor slots.

## Compatibility chain

Physics resolution order is:

```text
GLOCO_PhysicsProvider.move
        |
        +-- HANDLED  -> use provider result
        |
        `-- FALLBACK -> legacy GLOCO_WorldProbeFn
                              |
                              `-- absent -> original flat-floor builtin
```

Movement resolution follows the same pattern independently for horizontal, vertical, integration and impulse.

Passing `NULL` removes a provider and restores the old behavior:

```c
gloco_set_movement_provider(&ctx, 0);
gloco_set_physics_provider(&ctx, 0);
```

## Partial provider example

A host can replace only horizontal movement and keep gloco gravity/integration/physics:

```c
static int my_horizontal(void *user,
                         int actor_id,
                         GLOCO_Actor *actor,
                         const GLOCO_Profile *profile,
                         const GLOCO_Input *input,
                         const GLOCO_MotionIntent *intent,
                         GLOCO_U16 dt_ms)
{
    (void)user;
    (void)actor_id;
    (void)profile;
    (void)input;
    (void)dt_ms;

    actor->vel.x = intent->target_velocity.x;
    actor->vel.z = intent->target_velocity.z;
    return GLOCO_PROVIDER_HANDLED;
}
```

See `demo/demo_providers.c` for a complete C89 example with both providers installed.
