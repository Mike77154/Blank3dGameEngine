# bolt3d_ballistics_c89

**bolt3d_ballistics_c89 0.3.0** is a renderer-agnostic C89 projectile and motion-solver library for 3D games and deterministic simulations.

The caller owns every buffer. The library keeps no hidden storage, performs no dynamic allocation, and uses Q16.16 fixed-point math throughout.

## What 0.2 adds

The original ballistic path remains available and source-compatible. A second path can now treat any bound object as a projectile-like motion agent:

```txt
requested transform
        |
        v
collision contracts
        |
        v
iterative response solver
        |
        v
solved transform + ordered events
```

The solver supports:

- Fixed-point position, basis, orientation, and scale transforms.
- Local-to-world and world-to-local point/vector conversion.
- Transform composition and interpolation.
- Small-step angular integration and velocity alignment.
- Caller-owned solver-state and contact arenas.
- Built-in sphere sweeps plus optional collision-query contracts.
- Blocking, sliding, bouncing, sticking, piercing, overlap, and ignore responses.
- External transform read/write contracts.
- Optional solver-gravity provider hook with built-in fallback.
- Separate source and target pointers in events.
- Persistent solver objects or projectile-style contact destruction.

## Hard constraints

```txt
language: C89
math: Q16.16 fixed point
storage: caller-owned fixed-capacity buffers
hidden allocation: none
renderer dependency: none
entity dependency: none
platform dependency: none
```

## Folder layout

```txt
bolt3d_ballistics_c89/
├─ include/bolt3d/
│  ├─ bolt3d.h
│  ├─ b3d_config.h
│  ├─ b3d_types.h
│  ├─ b3d_fixed.h
│  ├─ b3d_vec3.h
│  ├─ b3d_transform.h
│  ├─ b3d_collision.h
│  ├─ b3d_solver.h
│  ├─ b3d_events.h
│  ├─ b3d_projectile.h
│  └─ b3d_world.h
├─ src/
│  ├─ b3d_fixed.c
│  ├─ b3d_vec3.c
│  ├─ b3d_transform.c
│  ├─ b3d_collision.c
│  ├─ b3d_solver.c
│  ├─ b3d_events.c
│  ├─ b3d_projectile.c
│  └─ b3d_world.c
├─ examples/
│  ├─ demo_basic.c
│  ├─ demo_hooks.c
│  └─ demo_solver.c
├─ tests/
│  ├─ test_fixed.c
│  ├─ test_collision.c
│  ├─ test_projectiles.c
│  ├─ test_hitscan.c
│  ├─ test_transform.c
│  └─ test_solver.c
├─ tools/audit_forbidden_tokens.sh
├─ STANDARD.md
├─ CHANGELOG.md
├─ Makefile
├─ LICENSE
└─ README.md
```

## Build and verify

```sh
make
make test
make audit
```

The default build uses:

```txt
-std=c89 -pedantic -Wall -Wextra -Werror
```

## Ballistic mode

Existing use stays the same:

```c
#include "bolt3d/bolt3d.h"

#define MAX_PROJECTILES 128
#define MAX_EVENTS 128
#define MAX_COLLIDERS 128

static B3D_Projectile projectiles[MAX_PROJECTILES];
static B3D_Event events[MAX_EVENTS];
static B3D_Collider colliders[MAX_COLLIDERS];

int main(void)
{
    B3D_World world;
    B3D_ProjectileDef bullet;

    b3d_world_init(
        &world,
        projectiles,
        MAX_PROJECTILES,
        events,
        MAX_EVENTS,
        colliders,
        MAX_COLLIDERS
    );

    bullet = b3d_projectile_def_bullet();
    b3d_spawn_projectile(
        &world,
        &bullet,
        42,
        b3d_vec3_zero(),
        b3d_vec3(B3D_FIXED_ONE, 0, 0)
    );

    b3d_world_update(&world, B3D_FIXED_ONE);
    return 0;
}
```

## Solver arena setup

Solver storage is attached separately, so ordinary projectiles do not pay for transform state:

```c
#define MAX_PROJECTILES 128
#define MAX_CONTACTS 16

static B3D_SolverState solvers[MAX_PROJECTILES];
static B3D_Contact contacts[MAX_CONTACTS];

b3d_world_init(
    &world,
    projectiles,
    MAX_PROJECTILES,
    events,
    MAX_EVENTS,
    colliders,
    MAX_COLLIDERS
);

b3d_world_attach_solver_arena(
    &world,
    solvers,
    MAX_PROJECTILES,
    contacts,
    MAX_CONTACTS
);
```

The solver arena should normally have at least as many slots as the projectile arena. Solver states are mapped by projectile slot. The contact arena is shared scratch storage for one collision-contract call at a time.

## Spawn a persistent solver object

Use `b3d_projectile_def_solver_object()` when the bound thing should not expire or die after its first contact:

```c
B3D_ProjectileDef object_def;
B3D_SolverDef solver_def;
B3D_Transform start;
int object_id;

object_def = b3d_projectile_def_solver_object();
object_def.hit_mask = B3D_LAYER_WORLD | B3D_LAYER_PROP;
object_def.radius = b3d_fixed_div(B3D_FIXED_ONE, b3d_fixed_from_int(2));

b3d_solver_def_clear(&solver_def);
solver_def.default_response = B3D_RESPONSE_SLIDE;
solver_def.max_iterations = 6;
solver_def.flags |= B3D_SOLVER_ALIGN_TO_VELOCITY;

start = b3d_transform_identity();
start.position = b3d_vec3_zero();

object_id = b3d_spawn_solver(
    &world,
    &object_def,
    &solver_def,
    owner_id,
    &start,
    initial_velocity
);
```

Default solver definitions include `B3D_SOLVER_KEEP_ACTIVE_ON_CONTACT`. Clear that flag when a solver-driven projectile should obey `max_hits` and disappear like a conventional projectile.

## Motion modes

```txt
B3D_MOTION_BALLISTIC  original projectile update path
B3D_MOTION_SOLVER     velocity and acceleration produce the desired transform
B3D_MOTION_EXTERNAL   another system supplies the desired transform
```

### External transform mode

External mode can consume animation, vehicle, tween, AI, or engine transforms and return a collision-resolved result:

```c
static int read_transform(
    void *user_data,
    const B3D_TransformRequest *request,
    B3D_Transform *out_transform
)
{
    /* Copy the engine object's desired transform into out_transform. */
    return B3D_TRUE;
}

static int write_transform(
    void *user_data,
    const B3D_TransformRequest *request,
    const B3D_Transform *transform_value
)
{
    /* Apply the resolved transform to the engine object. */
    return B3D_TRUE;
}
```

Register them through `B3D_Callbacks`:

```c
b3d_callbacks_clear(&callbacks);
callbacks.user_data = engine_context;
callbacks.read_transform = read_transform;
callbacks.write_transform = write_transform;
b3d_world_set_callbacks(&world, &callbacks);
```

## Solver gravity hook

`B3D_MOTION_SOLVER` objects with `B3D_PROJ_USE_GRAVITY` may ask the host for a world-space gravity acceleration vector. This supports planetary gravity, gravity volumes, moving reference frames, scripted fields, or another physics system without replacing Bolt3D collision solving.

```c
static int query_gravity(
    void *user_data,
    const B3D_SolverGravityRequest *request,
    B3D_Vec3 *out_gravity
)
{
    (void)user_data;

    /* Full vector acceleration, not a pre-multiplied velocity delta. */
    *out_gravity = b3d_vec3(0, b3d_fixed_from_int(-4), 0);
    return B3D_TRUE;
}

b3d_callbacks_clear(&callbacks);
callbacks.query_gravity = query_gravity;
b3d_world_set_callbacks(&world, &callbacks);
```

The hook is consulted only in solver motion mode and only when `B3D_PROJ_USE_GRAVITY` is enabled. Returning `B3D_TRUE` accepts `out_gravity`, including a zero vector. Returning `B3D_FALSE`, or leaving the hook unset, applies the original fallback `(0, -projectile->gravity, 0)`.

The request includes the current transform, velocity, ordinary acceleration, age, timestep, projectile and solver flags, binding identity/pointer, and `fallback_gravity`. A provider may copy or modify that fallback when it only wants to perturb the built-in field.

Then configure:

```c
b3d_solver_def_clear(&solver_def);
solver_def.motion_mode = B3D_MOTION_EXTERNAL;
solver_def.flags |= B3D_SOLVER_READ_TRANSFORM;
solver_def.flags |= B3D_SOLVER_WRITE_TRANSFORM;
```

A desired transform can also be assigned directly:

```c
b3d_solver_set_desired_transform(&world, object_id, &desired_transform);
```

## Collision contract

`query_collision` can connect the solver to BSP, triangle meshes, voxels, sectors, portals, an existing engine collision system, or a declarative condition layer.

```c
static int query_collision(
    void *user_data,
    const B3D_SolverQuery *query,
    B3D_Contact *contacts,
    int contact_capacity
)
{
    /* Fill up to contact_capacity contacts and return the count. */
    return 0;
}
```

A returned contact can specify:

```txt
hit kind and target identity
layer and user type
normalized time of impact in Q16.16
world-space point and normal
source-specific target pointer
optional response override
```

Set `contact.response` to `B3D_RESPONSE_DEFAULT` to use the solver's normal policy. A contract can directly request any standard response, including `B3D_RESPONSE_IGNORE`.

The built-in fallback remains sphere-based. Capsule, AABB, OBB, and custom shape records are passed to the external collision contract; the built-in path uses the shape radius as its conservative sweep radius.

## Response contract

A second hook can choose a response after a contact is found:

```c
static int resolve_contact(
    void *user_data,
    const B3D_SolverResponseRequest *request
)
{
    if ((request->contact.target_layer & B3D_LAYER_TRIGGER) != 0) {
        return B3D_RESPONSE_OVERLAP;
    }
    return request->default_response;
}
```

Responses:

```txt
B3D_RESPONSE_IGNORE
B3D_RESPONSE_BLOCK
B3D_RESPONSE_SLIDE
B3D_RESPONSE_BOUNCE
B3D_RESPONSE_STICK
B3D_RESPONSE_PIERCE
B3D_RESPONSE_OVERLAP
```

## Transform math

`B3D_Transform` contains:

```c
B3D_Vec3 position;
B3D_Basis3 basis;
B3D_Vec3 scale;
```

Important helpers:

```txt
b3d_basis_from_forward_up
b3d_basis_orthonormalize
b3d_basis_integrate_angular
b3d_transform_point
b3d_transform_vector
b3d_transform_point_to_local
b3d_transform_vector_to_local
b3d_transform_compose
b3d_transform_lerp
```

Angular integration is a deterministic small-step basis update followed by orthonormalization. Use fixed timesteps and reasonable angular steps.

## Event contract

Solver events include complete transform snapshots:

```txt
B3D_EVENT_SOLVER_BEGIN
B3D_EVENT_TRANSFORM_READ
B3D_EVENT_CONTACT
B3D_EVENT_OVERLAP
B3D_EVENT_BLOCKED
B3D_EVENT_SLIDE
B3D_EVENT_STICK
B3D_EVENT_TRANSFORM_SOLVED
B3D_EVENT_TRANSFORM_WRITE
B3D_EVENT_SOLVER_FAILED
```

`B3D_Event` now keeps both sides of the contract:

```c
void *source_user_ptr;
void *target_user_ptr;
```

The legacy `user_ptr` remains available. For target events it mirrors `target_user_ptr`; for source-only events it mirrors `source_user_ptr`.

Events also carry:

```txt
previous_transform
desired_transform
solved_transform
binding_id
motion_mode
response
iteration
target_user_type
```

## Built-in collision and projectile hooks

The original hooks remain:

```txt
trace_world
filter_hit
filter_explosion
on_event
```

`filter_hit` is now applied to external world/custom trace results as well as built-in colliders. Collider `user_type` is propagated into hits, filter requests, contacts, and events.

## Determinism notes

For repeatable results:

```txt
use a fixed timestep
update external colliders before world update
keep callback ordering deterministic
return contacts in deterministic order
use fixed solver iteration counts
consume events in queue order
```

## Verification included

The test suite covers:

```txt
fixed-point arithmetic
vector and collision helpers
legacy projectile behavior
hitscan behavior
transform round trips
persistent blocking
contract-driven sliding
external transform read/write
source/target event pointers
```

See `examples/demo_solver.c` for the smallest complete solver setup.
