# Total Solver integration contract

## Goal

`vpTotalSolver` is an optional orchestration module around `vpWorld`. It does not replace the rigid-body solver and does not force the host to adopt a specific scene graph, collision library, animation system or matrix convention.

It provides two ports:

```text
TRANSFORM / MATH PORT  → poses, hierarchy, sockets, constraints
COLLISION PORT          → contacts and optional sweep TOI
```

Both ports can be absent. The old `vpWorldStep()` and manual contact flow remain valid.

## Memory

Total Solver owns no heap. Allocate its arena separately from the physics-world arena:

```c
vp_u32 totalBytes = vpTotalSolverMemSize(&desc);
vpTotalSolver* total = vpTotalSolverInit(
    totalMemory,
    totalBytes,
    world,
    &desc);
```

Internal segments are aligned to `VP_ARENA_ALIGNMENT`, including when the caller supplies an unaligned starting address.

## Transform provider

```c
typedef struct vpTransformProvider {
    void* user;
    int (*readLocal)(...);
    int (*readWorld)(...);
    int (*writeLocal)(...);
    int (*writeWorld)(...);
    int (*getParent)(...);
} vpTransformProvider;
```

A provider may implement local-space access, world-space access, or both.

Read order:

1. `readLocal` is attempted when present;
2. if it declines, `readWorld` is attempted;
3. world poses are converted to local poses using the current parent pose;
4. hierarchy composition resolves all node world poses.

Write order:

1. `writeWorld` is attempted when present;
2. if it declines, `writeLocal` is attempted.

`getParent` returns an external object identifier. The solver maps it to a registered transform node. Missing external parents become roots; self-parenting increments `cycleCount` and is ignored.

## Math provider

Set `capabilities` only for implemented callbacks. Missing callbacks use fixed-point fallback implementations.

```c
math.capabilities =
    VP_TRANSFORM_MATH_COMPOSE |
    VP_TRANSFORM_MATH_QUAT_MUL;
```

The transform solver also uses lower-level provider operations while executing fallback operations. For example, if `compose` is absent but `rotateVector` and `quatMul` are supplied, fallback composition delegates those suboperations to the provider.

## Nodes

A node contains:

- external object ID;
- optional rigid-body ID;
- optional parent node;
- local and world transforms;
- authority;
- read/write/drive flags;
- fixed-point blend weight.

Node IDs are one-based; zero means no node.

Default external nodes are enabled for both reading and writing. Flags can be adjusted through the returned `vpTransformNode*` when a one-way bridge is required.

### Dynamic-body driving

External/manual nodes drive kinematic bodies automatically. Driving a dynamic body is disabled by default because teleporting a dynamic body can invalidate physical continuity.

To explicitly permit it:

```c
node->flags |= VP_TRANSFORM_NODE_DRIVE_DYNAMIC;
```

## Constraint phases

### Pre-physics

Use for:

- animation to kinematic body;
- gameplay target to physical controller;
- parent hierarchy updates required by collision detection;
- held objects and kinematic attachments.

### Post-physics

Use for:

- rigid body to scene object;
- ragdoll body to bone;
- camera follow;
- weapon socket and muzzle placement;
- visual attachments dependent on final physical pose.

A constraint with `VP_TRANSFORM_PHASE_BOTH` is evaluated in both phases.

## Constraint priorities

Constraints are sorted from lower to higher priority. Higher-priority constraints run later and therefore have the final say when multiple constraints affect the same component.

Each phase can run multiple iterations. This supports small dependency chains and soft blending without allocating a graph solver on the heap.

## Collision provider

```c
typedef struct vpCollisionProvider {
    void* user;
    void (*beginStep)(...);
    void (*generateContacts)(...);
    void (*endStep)(...);
    vp_fx (*bodySweepTOI)(...);
} vpCollisionProvider;
```

Suggested division:

- `beginStep`: update broadphase state or explicitly call `vpContactsBegin()`;
- `generateContacts`: append raw contacts;
- `endStep`: consume collision-side diagnostics after the physical step;
- `bodySweepTOI`: return fixed-point TOI in `[0, VP_FX_ONE]`.

When neither the collision provider nor legacy callbacks supply `bodySweepTOI`, the original uniform CCD substepping fallback remains active.

## Legacy callback chaining

Attachment captures the current `vpCallbacks` from the world.

Per step:

```text
Total pull transforms
Total pre-physics constraints
legacy preStep
collision beginStep
collision generateContacts
existing physics step
Total post-physics constraints
Total push transforms
collision endStep
legacy postStep
```

CCD sweep priority:

1. Total Solver collision provider;
2. captured legacy sweep callback;
3. original uniform-substep fallback when neither exists.

## Manual pipeline

Hosts that need custom scheduling can call the stages directly:

```c
vpTotalSolverPullTransforms(total);
vpTotalSolverSolvePrePhysics(total, dt);

/* host collision work */
vpWorldStep(world, dt);

vpTotalSolverSolvePostPhysics(total, dt);
vpTotalSolverPushTransforms(total);
```

Do not also attach the automatic callback bridge when manually scheduling the same stages, or they will run twice.

## Diagnostics

`vpTotalSolver` exposes two counters:

- `cycleCount`: hierarchy cycles or self-parent attempts encountered;
- `providerErrorCount`: external transform reads/writes that were requested but declined.

They are cumulative so the host can inspect them after a run or reset them explicitly.

## Snapshot behavior

The existing `vpWorldSaveState()` and `vpWorldLoadState()` format remains unchanged and continues to snapshot the rigid-body world.

Transform-node state is intentionally separate because it often mirrors a host scene graph, animation state or ECS. Snapshot that host state together with the world when rollback of the complete scene is required.
