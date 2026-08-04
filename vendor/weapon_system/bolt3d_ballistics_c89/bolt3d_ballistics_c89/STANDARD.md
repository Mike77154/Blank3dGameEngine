# bolt3d_ballistics_c89 standard

## 1. Hard constraints

```txt
language: C89
math: Q16.16 fixed point
ownership: caller-owned buffers
allocation model: fixed external storage only
renderer dependency: none
entity-system dependency: none
platform dependency: none
```

## 2. Module responsibilities

```txt
b3d_fixed      saturating fixed-point arithmetic
b3d_vec3       fixed-point vector operations
b3d_transform  basis and transform operations
b3d_collision  primitive collision tests and hit records
b3d_solver     motion states, contact contracts, iterative responses
b3d_events     ordered event payloads
b3d_projectile projectile definitions and runtime state
b3d_world      arenas, hooks, update dispatch, traces, explosions
```

## 3. Ownership model

The caller owns all persistent and scratch storage:

```c
static B3D_Projectile projectiles[MAX_PROJECTILES];
static B3D_Event events[MAX_EVENTS];
static B3D_Collider colliders[MAX_COLLIDERS];
static B3D_SolverState solvers[MAX_PROJECTILES];
static B3D_Contact contacts[MAX_CONTACTS];
```

The world stores pointers and capacities only. It never resizes a buffer.

## 4. Dual motion paths

```txt
ballistic path  preserves the original projectile behavior
solver path     resolves desired transforms through contacts
external path   receives a desired transform through a contract
```

The ballistic path must not require a solver arena. Solver state must remain parallel storage rather than enlarging every projectile slot.

## 5. Transform contract

A solver may read and write a bound object's transform through optional hooks. The bound object remains owned by the host engine.

A transform contains position, an orthonormal basis, and scale. All operations use fixed-point values.

## 5.1 Solver gravity contract

Solver-mode objects may request a world-space gravity acceleration vector from the host when `B3D_PROJ_USE_GRAVITY` is enabled.

The provider owns no Bolt3D state. It receives immutable projectile, solver, transform, velocity, timestep, and binding context and writes one fixed-point vector. Returning false preserves the internal `(0, -gravity, 0)` behavior. A returned zero vector is a valid accepted gravity field.

## 6. Collision contract

The built-in path performs sphere sweeps against caller-owned sphere colliders and the legacy world trace hook.

The solver query hook may return contacts for any host shape or geometry system. The shared contact arena is scratch storage and must not be retained by the hook after it returns.

Every external contact should provide deterministic target identity, impact time, point, normal, and layer data.

## 7. Response contract

Standard responses are:

```txt
ignore
block
slide
bounce
stick
pierce
overlap
```

Triggers default to overlap. Projectile flags may select pierce, bounce, or stick before the solver default is applied. A response hook may override the final decision.

## 8. Event contract

The library reports state changes but does not mutate host entities directly.

Solver event payloads must preserve:

```txt
source identity and pointer
target identity and pointer
previous transform
desired transform
solved transform
contact response
solver iteration
```

The event queue and immediate callback may both be used.

## 9. Determinism

For deterministic behavior:

```txt
use the same fixed timestep
use the same arena ordering
update colliders before projectiles
return contract contacts deterministically
avoid host-side nondeterministic filters
consume events in order
```

## 10. Error behavior

```txt
spawn failure             returns B3D_ID_NONE
missing solver arena      solver enable/spawn fails cleanly
missing transform hook    stored transform remains authoritative
missing gravity hook      built-in solver gravity remains authoritative
missing query hook        built-in query remains available
missing contact arena     external query is skipped
full event queue          overflow is recorded
invalid response          falls back to the default response
```

## 11. Extension rules

Preferred extensions:

```txt
new fixed-point transform helpers
new optional collision adapters
new deterministic force-field adapters
new event types
new standard response helpers
new caller-owned broadphase arenas
new deterministic replay data
```

Avoid global singletons, hidden storage, renderer dependencies, host entity assumptions, or platform-specific state in core modules.
