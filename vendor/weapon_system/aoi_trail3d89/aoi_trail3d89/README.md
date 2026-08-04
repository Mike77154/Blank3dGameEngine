# aoi_trail3d89_c89_v2_general

Generic temporal 3D trail mesh library for small C89 engines.

This library is designed for effects such as bullet streaks, dash trails, sword sweeps, claw trails, beams, muzzle streaks, casing streaks, magic, smoke-ish fake volume trails, camera path debug trails, and any other visual effect that can be represented as a short history of 3D samples.

## Goals

- C89 only.
- Fixed-point Q8.8.
- No heap.
- No `malloc`, `free`, `realloc`.
- No `float` / `double`.
- No `long long`.
- Ring buffers per trail.
- Declarative profiles through `t3d89_desc`.
- Renderer-agnostic mesh output.
- LOD by vertex budget.
- Samplers by distance, time, and curvature.
- Builders for ribbons, cross-ribbons, tube-lite, beams, and socket sweeps.
- Optional external transform providers for move, rotate, and scale.

## Mental model

```txt
3D object / projectile / dash / socket pair / beam
        |
        +--> manual: trail3d89_emit_*()
        |
        +--> provider: move + rotation matrix + scale
        |
        v
ring-buffer of temporal samples
        |
        v
trail3d89_build_mesh() or trail3d89_build_all()
        |
        v
caller-owned vertices + indices
        |
        v
your renderer
```

The library does not know what a player, zombie, gun, projectile, casing, bone, or camera is. It only knows trails and samples.

## Modes

| Mode | Best for | Output |
|---|---|---|
| `T3D89_MODE_VIEW_RIBBON` | bullets, magic, speed streaks | camera-facing ribbon |
| `T3D89_MODE_AXIS_RIBBON` | dash trails, floor streaks | ribbon constrained by an axis |
| `T3D89_MODE_ORIENTED_RIBBON` | object-local trails | uses per-sample right vector |
| `T3D89_MODE_CROSS_RIBBON` | fake volume, cheap smoke/energy | two crossed ribbons |
| `T3D89_MODE_TUBE_LITE` | plasma, thick laser-ish trails | 4-sided diamond tube |
| `T3D89_MODE_BEAM_AB` | lasers, ropes, continuous beams | billboarded A/B quads |
| `T3D89_MODE_SOCKET_SWEEP` | sword, claw, wing, tail arcs | quads between historical A/B segments |

## Minimal usage

```c
#include "trail3d89.h"

static t3d89_vertex verts[2048];
static unsigned short indices[4096];

void example(void)
{
    t3d89_ctx ctx;
    t3d89_desc desc;
    t3d89_mesh mesh;
    t3d89_camera cam;
    int id;

    t3d89_init(&ctx);
    t3d89_default_desc(&desc);

    desc.mode = T3D89_MODE_VIEW_RIBBON;
    desc.life_ticks = 12;
    desc.min_dist = T3D89_FP_ONE / 4;
    desc.width_head = T3D89_FP_ONE / 8;
    desc.width_tail = 0;
    desc.color_head = t3d89_color_make(255, 240, 64, 220);
    desc.color_tail = t3d89_color_make(255, 80, 0, 0);

    id = t3d89_create(&ctx, &desc);

    t3d89_emit_point(&ctx, id, t3d89_fp_from_int(0), 0, 0);
    t3d89_tick(&ctx, 1);
    t3d89_emit_point(&ctx, id, t3d89_fp_from_int(1), 0, 0);

    cam.x = 0;
    cam.y = t3d89_fp_from_int(2);
    cam.z = t3d89_fp_from_int(8);
    cam.up_x = 0;
    cam.up_y = T3D89_FP_ONE;
    cam.up_z = 0;

    mesh.vertices = verts;
    mesh.indices = indices;
    mesh.max_vertices = 2048;
    mesh.max_indices = 4096;
    mesh.flags = 0;

    t3d89_build_mesh(&ctx, id, &cam, &mesh);
}
```

## External transform provider mode

The manual emit API remains unchanged. Provider mode is optional and lets an
engine, physics solver, animation system, or entity component table supply a
full object transform.

The transform is fixed-point Q8.8:

```txt
world = translation + rotation_matrix * (local * scale_xyz)
```

The matrix is row-major:

```txt
world_x = tx + m00*x + m01*y + m02*z
world_y = ty + m10*x + m11*y + m12*z
world_z = tz + m20*x + m21*y + m22*z
```

Every multiplication above is Q8.8 multiplication performed internally without
`float`, `double`, `long long`, or dynamic allocation.

### Provider callback

```c
typedef int (*t3d89_transform_provider_fn)(
    void *user,
    int trail_id,
    int tick,
    t3d89_transform *out_transform
);
```

Return:

- `T3D89_PROVIDER_READY` when the transform is valid.
- `T3D89_PROVIDER_SKIP` when no sample should be emitted this tick.
- A negative value to report a provider-side failure.

`t3d89_step_provider()` reports provider failures as `T3D89_ERR_PROVIDER`, while
`t3d89_get_provider_last_result()` preserves the provider's original negative
result for diagnostics.

### Binding a local point

```c
static int object_provider(
    void *user,
    int trail_id,
    int tick,
    t3d89_transform *out_transform
)
{
    my_object *object;

    (void)trail_id;
    (void)tick;
    object = (my_object *)user;

    t3d89_transform_identity(out_transform);
    out_transform->tx = object->x;
    out_transform->ty = object->y;
    out_transform->tz = object->z;
    out_transform->m00 = object->m00;
    out_transform->m01 = object->m01;
    out_transform->m02 = object->m02;
    out_transform->m10 = object->m10;
    out_transform->m11 = object->m11;
    out_transform->m12 = object->m12;
    out_transform->m20 = object->m20;
    out_transform->m21 = object->m21;
    out_transform->m22 = object->m22;
    out_transform->sx = object->scale_x;
    out_transform->sy = object->scale_y;
    out_transform->sz = object->scale_z;
    return T3D89_PROVIDER_READY;
}

/* Local offset from the object's origin. */
t3d89_sample_clear(&local_sample);
local_sample.x = 0;
local_sample.y = T3D89_FP_ONE;
local_sample.z = 0;

t3d89_bind_transform_provider(
    &ctx,
    trail_id,
    object_provider,
    &object,
    &local_sample,
    T3D89_PROVIDER_AUTOSTEP
);
```

With `T3D89_PROVIDER_AUTOSTEP`, `t3d89_tick()` asks the provider for a transform
and emits the transformed sample after aging the old points. Without that flag,
call `t3d89_step_provider()` exactly where your engine wants the sample.

### Local sockets and orientation

The same local sample can contain:

- `x/y/z`: local trail origin.
- `a_x..b_z` plus `T3D89_EMIT_HAS_AB`: local sword, claw, wing, or beam sockets.
- `right_*` and `up_*` plus `T3D89_EMIT_HAS_ORIENT`: local orientation vectors.
- `width` plus `T3D89_EMIT_HAS_WIDTH`: optional per-sample width.

`T3D89_PROVIDER_SCALE_WIDTH` scales an explicit sample width by the largest
absolute XYZ scale. Leave the flag unset when width must remain in world units.

The provider binding survives `t3d89_reset()` so a trail can clear its history
without reconnecting to the engine object. `t3d89_destroy()` and
`t3d89_unbind_transform_provider()` remove the binding.

## Samplers

`desc.sampler_mask` decides when a new point is accepted:

- `T3D89_SAMPLE_DISTANCE`: add a point after `min_dist` movement.
- `T3D89_SAMPLE_TIME`: add a point after `min_ticks`.
- `T3D89_SAMPLE_CURVE`: add a point when the new sample deviates from linear extrapolation by `curve_dist`.
- `T3D89_SAMPLE_FORCE`: always accept.

Default sampler logic is `T3D89_SAMPLER_ANY`, which is usually best for responsive visuals. `T3D89_SAMPLER_ALL` is stricter.

## LOD by budget

`desc.lod_vertex_budget` gives the builder a soft per-trail vertex budget. When the estimated trail mesh would exceed the budget, the builder increases its stride through the historical points.

This keeps the trail alive and visually readable instead of failing immediately when a trail is long.

## Profiles

### Bullet trail

```c
t3d89_default_desc(&d);
d.mode = T3D89_MODE_VIEW_RIBBON;
d.life_ticks = 8;
d.min_dist = T3D89_FP_ONE / 6;
d.width_head = T3D89_FP_ONE / 8;
d.width_tail = 0;
d.color_head = t3d89_color_make(255, 240, 64, 230);
d.color_tail = t3d89_color_make(255, 80, 0, 0);
```

### Dash trail

```c
t3d89_default_desc(&d);
d.mode = T3D89_MODE_AXIS_RIBBON;
d.life_ticks = 16;
d.min_dist = T3D89_FP_ONE / 3;
d.width_head = T3D89_FP_ONE;
d.width_tail = T3D89_FP_ONE / 8;
d.axis_x = 0;
d.axis_y = T3D89_FP_ONE;
d.axis_z = 0;
```

### Sword or claw sweep

```c
t3d89_default_desc(&d);
d.mode = T3D89_MODE_SOCKET_SWEEP;
d.life_ticks = 8;
d.sampler_mask = T3D89_SAMPLE_FORCE;

/* Per frame: base socket + tip socket. */
t3d89_emit_segment(&ctx, id, base_x, base_y, base_z, tip_x, tip_y, tip_z);
```

### Beam

```c
t3d89_default_desc(&d);
d.mode = T3D89_MODE_BEAM_AB;
d.life_ticks = 2;
d.sampler_mask = T3D89_SAMPLE_FORCE;
t3d89_emit_segment(&ctx, id, muzzle_x, muzzle_y, muzzle_z, hit_x, hit_y, hit_z);
```

### Fake volumetric energy

```c
t3d89_default_desc(&d);
d.mode = T3D89_MODE_CROSS_RIBBON;
d.life_ticks = 20;
d.width_head = T3D89_FP_ONE / 2;
d.width_tail = 0;
```

### Tube-lite

```c
t3d89_default_desc(&d);
d.mode = T3D89_MODE_TUBE_LITE;
d.life_ticks = 20;
d.tube_sides = 4;
d.width_head = T3D89_FP_ONE / 2;
```

## Preset profiles

Optional helper presets live in `trail3d89_profiles.h` / `trail3d89_profiles.c`:

```c
#include "trail3d89_profiles.h"

t3d89_desc d;
t3d89_profile_bullet(&d);
trail_id = t3d89_create(&ctx, &d);
```

Included presets:

```txt
bullet, tracer, ground dash, air dash, casing, beam, socket sword, claw, energy cross, debug path
```

You can still ignore these helpers and fill `t3d89_desc` manually. The presets are just declarative profiles for faster integration.

## Build

```sh
make clean
make check
make demo
make provider_demo
./build/test_trail3d89
./build/trail3d89_demo
./build/trail3d89_provider_demo
```

The Makefile uses `-std=c89 -pedantic -Wall -Wextra`.

## Integration notes

- Call `t3d89_tick(&ctx, 1)` once per simulation tick or visual tick.
- Manual mode: emit after moving the object, so the trail follows the current transform.
- Provider mode: update the external object first, then call `t3d89_tick()` or `t3d89_step_provider()`.
- For FPS bullets, emit from the camera/muzzle visual path.
- For third-person bullets, emit from the weapon/cube/muzzle object path.
- For sword/claw/wing sweeps, feed two sockets with `t3d89_emit_segment` and use `T3D89_MODE_SOCKET_SWEEP`.
- For lasers, use `T3D89_MODE_BEAM_AB`; one or two ticks of lifetime usually feels correct.
- Render output is caller-owned: upload `mesh.vertices` and `mesh.indices` to whatever renderer you already have.

## Fixed-point and overflow note

The math is intentionally small-engine friendly. It uses shifted integer math instead of `float`, `double`, or `long long`. Keep world coordinates in sane Q8.8 ranges or convert to camera/local space before feeding samples if your world is huge.

## License

CC0-1.0 / public domain style. See `LICENSE`.
