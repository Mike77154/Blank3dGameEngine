# trail3d89 design notes

## Why this is not only a bullet trail library

The important abstraction is not a bullet. The important abstraction is a temporal sample stream:

```txt
sample(time, position, optional orientation, optional A/B segment, width, color)
```

Once a system owns that abstraction, the same core can draw bullets, dash smears, sword sweeps, claws, monster tails, muzzle streaks, beam quads, fake smoke volume, and debug paths.

## Industrial choices

### 1. Ring buffers

Each trail owns a fixed-capacity ring. New points overwrite the oldest when full. This avoids allocation and keeps behavior deterministic.

### 2. Sampling policy

The library supports three normal samplers:

- Distance: good for object movement.
- Time: good for slow movement or steady visual density.
- Curvature: good for bends, arcs, and dash/sword curves.

Curvature is implemented as deviation from linear extrapolation. This avoids expensive trigonometry and square roots.

### 3. Builders are separate from emitters

The emitter writes samples. The builder decides how to convert those samples into geometry.

This allows the same sample stream to become a view-facing ribbon, an axis ribbon, a crossed ribbon, a tube-lite, a beam, or a socket sweep.

### 4. Mesh output is agnostic

The library writes to caller-owned `t3d89_vertex` and `unsigned short` index buffers. It does not call OpenGL, Direct3D, SDL, software rasterizers, or engine-specific APIs.

### 5. LOD by stride

If a trail has too many points for the declared vertex budget, the mesh builder skips historical points by increasing stride. It keeps the tail and head shape approximately readable while reducing emitted vertices.

## Recommended runtime placement

```txt
engine/
├─ vendor/
│  └─ trail3d89/
├─ runtime/
│  ├─ vfx_trails.c         wrapper that owns t3d89_ctx
│  ├─ weapons_trails.c     bullet/muzzle/casing profiles
│  ├─ player_dash.c        dash profile
│  └─ melee_sweeps.c       socket sweep profile
└─ renderer/
   └─ draw_trail_mesh.c    adapts t3d89_mesh to renderer vertex format
```

## Suggested profiles for Emmerald/Blank3D-style runtime

### Guns

- Bullet: `VIEW_RIBBON`, short lifetime, yellow/orange fade.
- Tracer: `TUBE_LITE`, longer min distance, thin width.
- Muzzle streak: `BEAM_AB`, one tick.
- Casing streak: `CROSS_RIBBON`, small width, short lifetime.

### Movement

- Dash: `AXIS_RIBBON`, wider head, floor/world-up axis.
- Air dash: `VIEW_RIBBON` or `CROSS_RIBBON`.
- Teleport afterimage path: `TUBE_LITE` or multiple `VIEW_RIBBON` trails.

### Melee / creatures

- Sword: `SOCKET_SWEEP`, base socket and tip socket.
- Claws: several `SOCKET_SWEEP` trails, one per claw pair.
- Tail: `ORIENTED_RIBBON` if you have orientation, otherwise `VIEW_RIBBON`.
- Wing slash: `SOCKET_SWEEP` across wing root/tip.


## Transform provider boundary

Provider mode deliberately does not make trail3d89 own physics, animation, or
entity transforms. The external runtime owns those systems and exposes a single
fixed-point affine transform callback.

```txt
external movement / physics / skeleton / ECS
                  |
                  v
      t3d89_transform_provider_fn
                  |
         move + rotate + scale
                  |
                  v
   local trail sample -> world sample
                  |
                  v
       existing sampler + ring + mesh
```

This keeps all existing manual emitters working while allowing a solver-driven
runtime to bind one local sample per trail. A local sample may be a point,
orientation basis, or A/B socket pair. The provider never receives or mutates
the trail ring buffer.

The affine matrix is row-major Q8.8 and scale is applied before rotation.
Translation is applied only to points; velocity/right/up are transformed as
vectors. Width scaling is optional because some effects want object-relative
width while others want a stable world-space width.
