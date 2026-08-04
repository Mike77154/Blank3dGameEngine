# Cameranaku89 v3.4.0

CC0 C89 fixed-point camera resolver for small engines and retro-style runtimes.

Cameranaku89 is renderer-agnostic, entity-agnostic and physics-agnostic. It owns no heap memory, performs no allocation, uses no `float`/`double`, and stores cameras, virtual cameras, zones, target groups and manager state in static structs supplied by the caller.

## Design goals

```text
C89
fixed-point
no malloc/realloc/free
no heap ownership
no float/double
static arenas/pools
renderer-agnostic
physics/collision-agnostic
profile-bank-agnostic
no franchise-specific camera names
```

## Core features

```text
Follow / third-person camera
OTS profile style
FPS profile style
Fixed cameras
Orbit / free look
Rail camera
Cutscene keyframes
Perspective / orthographic / frustum-style projection
FOV, near/far clip, viewport and offsets
World-to-view and world-to-screen
View matrix / projection matrix
Camera shake
Blending
Static camera arena
World-probe collision callback
```

## New in v3.4.0: receive-provider 3D transforms

Cameranaku89 can now receive `move`, `scale` and `rotate` math from the host engine or from an external math library. The built-in fixed-point path remains the default.

```text
CNK_TRANSFORM_MODE_INTERNAL
CNK_TRANSFORM_MODE_RECEIVE_PROVIDER

cnk_transform_move_fn
cnk_transform_scale_fn
cnk_transform_rotate_fn
cnk_transform_provider
```

Provider callbacks return non-zero on success. A missing callback or a callback that returns zero falls back to Cameranaku89's internal C89 fixed-point operation, so partial providers are valid.

The receive-provider path is used by:

```text
core FPS/follow/orbit/fixed/rail/cutscene camera solving
local offsets, zoom distance and camera shake placement
provider-driven camera basis rotation
world-to-view translation
freelook
spring arm
extended collision fallbacks
virtual cameras
camera manager blending
```

### Minimal receive-provider setup

```c
typedef struct my_math_context_s {
    int calls;
} my_math_context;

static int my_move(void *user, cnk_vec3 position, cnk_vec3 delta, cnk_vec3 *out_position)
{
    my_math_context *ctx;
    ctx = (my_math_context *)user;
    ctx->calls += 1;
    out_position->x = position.x + delta.x;
    out_position->y = position.y + delta.y;
    out_position->z = position.z + delta.z;
    return 1;
}

static int my_scale(void *user, cnk_vec3 value, cnk_vec3 scale, cnk_vec3 *out_value)
{
    (void)user;
    out_value->x = cnk_fx_mul(value.x, scale.x);
    out_value->y = cnk_fx_mul(value.y, scale.y);
    out_value->z = cnk_fx_mul(value.z, scale.z);
    return 1;
}

static int my_rotate(void *user, cnk_vec3 value, cnk_vec3 euler_deg, cnk_vec3 *out_value)
{
    (void)user;
    /* Replace this line with the engine/external library rotation. */
    *out_value = cnk_transform_rotate(0, value, euler_deg);
    return 1;
}

cnk_transform_provider provider;
cnk_camera camera;
my_math_context math_context;

math_context.calls = 0;
cnk_transform_provider_clear(&provider);
cnk_transform_provider_set(&provider, &math_context, my_move, my_scale, my_rotate);

cnk_camera_reset(&camera);
cnk_camera_set_transform_provider(&camera, &provider);
```

The rotation callback receives Euler angles in a `cnk_vec3` as:

```text
euler_deg.x = yaw
euler_deg.y = pitch
euler_deg.z = roll
```

All callback values use Cameranaku89 fixed-point units. A host using another representation can convert at the callback boundary.

### Pass the provider through the engine adapter

```c
cnk_solver_bind bind;

bind.camera = &camera;
bind.target_id = player_id;
bind.enabled = 1;

cnk_adapter_receive_transform_provider(&bind, &provider);
```

For virtual-camera managers:

```c
cnk_camera_manager_set_transform_provider_all(&manager, &provider);
```

See `demo/demo_transform_provider.c` and `RECEIVE_PROVIDER.md`.

> ABI note: `cnk_camera` and `cnk_camera_manager` gained provider fields in v3.4.0. Recompile the host engine and all modules that embed these structs.

## New in v3.3.0: Camera Director phase

v3.3 expands the v3.2 virtual-camera pipeline with director-grade tools:

```text
cnk_camera_zone_bank     trigger volumes / room volumes / fixed-camera volumes
cnk_camera_zone          force, enable, confine or bias a virtual camera
cnk_composer             rule-of-thirds / screen-space shot scoring
manager hysteresis       avoids rapid camera flip-flop
manager force/hold       force a vcam by id for N ticks or until cleared
priority_bias            temporary zone/cue priority without mutating base priority
collision strategies     active slide, shoulder-swap and cut-to-backup fallbacks
```

## Folder layout

```text
cameranaku89/
├─ include/
│  ├─ cameranaku89.h
│  ├─ cameranaku89_config.h
│  ├─ cameranaku89_profiles.h
│  ├─ cameranaku89_adapter.h
│  ├─ cameranaku89_collision.h
│  ├─ cameranaku89_spring_arm.h
│  ├─ cameranaku89_target_group.h
│  ├─ cameranaku89_composer.h
│  ├─ cameranaku89_virtual.h
│  ├─ cameranaku89_manager.h
│  ├─ cameranaku89_zones.h
│  ├─ cameranaku89_lens_ext.h
│  ├─ cameranaku89_freelook.h
│  ├─ cameranaku89_shake_ext.h
│  └─ cameranaku89_all.h
├─ src/
│  ├─ cameranaku89.c
│  ├─ cameranaku89_profiles.c
│  ├─ cameranaku89_adapter.c
│  ├─ cameranaku89_collision.c
│  ├─ cameranaku89_spring_arm.c
│  ├─ cameranaku89_target_group.c
│  ├─ cameranaku89_composer.c
│  ├─ cameranaku89_virtual.c
│  ├─ cameranaku89_manager.c
│  ├─ cameranaku89_zones.c
│  ├─ cameranaku89_lens_ext.c
│  ├─ cameranaku89_freelook.c
│  └─ cameranaku89_shake_ext.c
└─ demo/
   ├─ demo_cameranaku89.c
   ├─ demo_camera_manager.c
   ├─ demo_camera_zones.c
   └─ demo_transform_provider.c
```

## Compile

Core + profile demo:

```bash
gcc -std=c89 -pedantic -Wall -Wextra \
  -Iinclude \
  src/*.c \
  demo/demo_cameranaku89.c \
  -o demo_cameranaku89
```

Full manager demo:

```bash
gcc -std=c89 -pedantic -Wall -Wextra \
  -Iinclude \
  src/*.c \
  demo/demo_camera_manager.c \
  -o demo_camera_manager
```

Zone/director demo:

```bash
gcc -std=c89 -pedantic -Wall -Wextra \
  -Iinclude \
  src/*.c \
  demo/demo_camera_zones.c \
  -o demo_camera_zones
```

Receive-provider demo:

```bash
gcc -std=c89 -pedantic -Wall -Wextra \
  -Iinclude \
  src/*.c \
  demo/demo_transform_provider.c \
  -o demo_transform_provider
```

## Minimal manager usage

```c
#include "cameranaku89_all.h"

cnk_camera_manager mgr;
cnk_virtual_camera *vcam;
cnk_profile profile;
cnk_target target;

cnk_camera_manager_init(&mgr);
vcam = cnk_camera_manager_alloc(&mgr, 1);

cnk_profile_default(&profile);
cnk_profile_style_ots(&profile);
cnk_virtual_camera_set_profile(vcam, &profile);
cnk_virtual_camera_set_priority(vcam, 10);

/* Fill target from your entity bridge. */
cnk_virtual_camera_set_target(vcam, target);

/* Per frame. */
cnk_camera_manager_update(&mgr, 1, my_world_probe, my_probe_user);
```

## Minimal zone/director usage

```c
cnk_camera_zone_bank zones;
int zone;

cnk_camera_zone_bank_clear(&zones);
zone = cnk_camera_zone_add_box(&zones, 1, 200,
    cnk_vec3_make(-CNK_FX_FROM_INT(4), 0, -CNK_FX_FROM_INT(4)),
    cnk_vec3_make( CNK_FX_FROM_INT(4), CNK_FX_FROM_INT(4), CNK_FX_FROM_INT(4)),
    10,
    CNK_ZONE_FORCE_CAMERA | CNK_ZONE_USE_BLEND | CNK_ZONE_APPLY_CONFINER);

cnk_camera_zone_set_bias(&zones, zone, 8, 10, 8);

/* Before cnk_camera_manager_update each frame. */
cnk_camera_zone_bank_apply(&zones, &mgr, player_pos);
```

## World probe contract

The engine provides the collision query. Cameranaku89 does not know about BSP, rooms, grids, capsules, triangles, Sicol, CC, VPhysics, or custom geometry.

```c
typedef int (*cnk_world_probe_fn)(
    void *user,
    cnk_vec3 from,
    cnk_vec3 to,
    cnk_fx radius,
    cnk_vec3 *out_pos
);
```

Return non-zero when the probe changed `out_pos` to a safe camera position.

## Collision strategies

```text
CNK_COLLISION_PULL_FORWARD      classic deocclusion pull-in
CNK_COLLISION_PRESERVE_HEIGHT   pull-in but keep camera height
CNK_COLLISION_SLIDE             try safe left/right slide after hit
CNK_COLLISION_SHOULDER_SWAP     try opposite shoulder fallback
CNK_COLLISION_CUT_TO_BACKUP     cut to a configured backup offset
```

These are still probe-driven and engine-agnostic. A deeper engine bridge can replace the probe with Sicol, CC, VPhysics, BSP, grid sectors, triangle sweep, or room volumes.

## Notes

- The base solver remains usable alone.
- Profile styles are optional and futureable.
- The manager, virtual camera, zones and composer modules are optional.
- `OTS` and `FPS` are generic profile styles, not hard-coded game references.
- The projection math is intentionally small and fixed-point; engines can replace the final matrices if they need renderer-specific conventions.
