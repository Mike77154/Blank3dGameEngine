# spriteplane89 v2

C89 fixed-point 3D sprite-plane / billboard library.

It lets a game engine place 2D images, drawings, animated atlas frames, Doom-like 8/16 direction sprites, text, 9-slice panels, fake shadows, point-sprite commands, vector marks, fire/smoke sheets, signs, pickups, and flat enemy impostors inside a real 3D world.

## Constraints

- C89 / gnu89 friendly
- fixed-point 16.16
- no malloc/free/realloc
- no heap ownership
- no float/double
- no renderer dependency
- no texture loader dependency
- static buffers only
- API-only: no DSL, no parser, no file format assumptions

## What v2 adds over v1

- More billboard modes:
  - fixed plane
  - view-aligned
  - camera-facing
  - Y-axis/cylindrical
  - axis-locked
  - velocity-aligned
  - surface-aligned
  - crossed-Y billboards
- Render queues and blend hints:
  - opaque
  - alpha-test
  - transparent
  - additive
  - multiply
  - overlay
- Packet output per emitted primitive group.
- Optional point-sprite command output for renderers that can expand points on GPU/backend side.
- Bitmap font atlas support.
- Text commands attached to any sprite plane.
- 9-slice panel commands attached to any sprite plane.
- Vector commands:
  - rect
  - line
  - triangle
  - circle approximation
- Fake shadow blob emitter.
- Picking/ray interaction against sprite planes.
- Distance culling field.
- Hook management for any external DSL/runtime.
- Mutate/cull/frame/depth/post hooks.

## Hook management

The lib does not parse scripts. It accepts hooks so any DSL can drive it.

```c
sprpl89_hooks hooks;
hooks.user = your_runtime;
hooks.cull_sprite = my_cull;
hooks.resolve_frame = my_frame_resolver;
hooks.mutate_sprite = my_mutator;
hooks.post_emit = my_post_emit;
hooks.depth_fade = my_depth_fade;
sprpl89_set_hooks(&ctx, &hooks);
```

The intended bridge is:

```text
Any DSL / editor / runtime
        ↓ fills structs or callbacks
spriteplane89 v2
        ↓ emits geometry / packets / point commands
Your renderer
        ↓
World 3D
```

## Minimal sprite usage

```c
sprpl89_ctx ctx;
sprpl89_camera cam;
sprpl89_sprite spr;
sprpl89_emit emit;
int frame_id;
int sprite_id;

sprpl89_init(&ctx);
sprpl89_set_camera(&ctx, &cam);
frame_id = sprpl89_add_frame_px(&ctx, 256, 256, 0, 0, 64, 64, 0, 100);

/* Zero-init your own struct in real code if you prefer. This demo fills all fields. */
spr.active = 0;
spr.flags = SP89_FLAG_ALPHA_BLEND | SP89_FLAG_DEPTH_TEST;
spr.xflags = 0;
spr.billboard_mode = SP89_BILLBOARD_Y_AXIS;
spr.frame_mode = SP89_FRAME_STATIC;
spr.anchor_mode = SP89_ANCHOR_BOTTOM_CENTER;
spr.render_queue = SP89_QUEUE_TRANSPARENT;
spr.blend_mode = SP89_BLEND_ALPHA;
spr.pos = sprpl89_v3(sprpl89_fx_from_int(0), 0, sprpl89_fx_from_int(4));
spr.axis_right = sprpl89_v3(SP89_FX_ONE, 0, 0);
spr.axis_up = sprpl89_v3(0, SP89_FX_ONE, 0);
spr.axis_lock = sprpl89_v3(0, SP89_FX_ONE, 0);
spr.velocity = sprpl89_v3(0, 0, 0);
spr.surface_right = spr.axis_right;
spr.surface_up = spr.axis_up;
spr.width = sprpl89_fx_from_int(2);
spr.height = sprpl89_fx_from_int(3);
spr.scale_x = SP89_FX_ONE;
spr.scale_y = SP89_FX_ONE;
spr.base_frame = (sprpl89_u16)frame_id;
spr.frame_count = 1;
spr.view_count = 1;
spr.texture_page = 0;
spr.material_id = 1;
spr.color = sprpl89_rgba(255,255,255,255);

sprite_id = sprpl89_spawn_sprite(&ctx, &spr);
sprpl89_emit_begin(&emit);
sprpl89_emit_all_sorted(&ctx, &emit);
```

Then your renderer draws `emit.verts`, `emit.tris`, `emit.packets`, and optionally `emit.points`.

## Text on a plane

```c
int font_id;
font_id = sprpl89_add_font(&ctx, 8, 4, 2);
sprpl89_font_set_ascii_grid(&ctx, font_id, 256, 256, 0, 32, 16, 6, 8, 8, 8);
sprpl89_add_text(&ctx, sprite_id, font_id, "LAB-01", -SP89_FX_HALF, sprpl89_fx_from_int(2),
              SP89_FX_ONE / 16, SP89_FX_ONE / 16, sprpl89_rgba(255,255,255,255), SP89_TEXT_LEFT);
```

The font atlas is pre-baked. The runtime only receives glyph atlas frames and metrics.

## 9-slice panel on a plane

```c
sprpl89_add_panel_9slice(&ctx, sprite_id, panel_frame,
    -sprpl89_fx_from_int(1), sprpl89_fx_from_int(1),
     sprpl89_fx_from_int(1), sprpl89_fx_from_int(2),
     sprpl89_fx_from_int(1) / 8,
     sprpl89_rgba(50, 80, 180, 180));
```

## Point-sprite command fallback

If your renderer can expand one point into a textured quad, set:

```c
spr.xflags |= SP89_XFLAG_POINT_COMMAND;
```

Then the sprite is added to `emit.points` instead of CPU-emitted quads. If the renderer cannot support it, ignore this flag and spawn normal quads instead.

## Picking

```c
sprpl89_pick_hit hit;
sprpl89_pick_all_ray(&ctx, camera.pos, camera.forward, &hit);
if (hit.hit) {
    /* hit.sprite_index, hit.uv_x, hit.uv_y, hit.world_pos */
}
```

## Suggested render bridge

1. Draw opaque world geometry.
2. Draw `SP89_QUEUE_OPAQUE` sprites front-to-back.
3. Draw `SP89_QUEUE_ALPHA_TEST` sprites/decor that can depth-write.
4. Draw `SP89_QUEUE_TRANSPARENT` and `SP89_QUEUE_ADDITIVE` far-to-near.
5. Draw `SP89_QUEUE_OVERLAY` last.

## Build

```sh
mingw32-make -f Makefile.mingw
./spriteplane89_demo
```

The demo emits `out/spriteplane89_v2_demo.obj` as a smoke test.

## Files

```text
spriteplane89_v2/
├─ include/spriteplane89.h
├─ src/spriteplane89.c
├─ demo/demo_spriteplane89.c
├─ docs/BRIDGE_NOTES.md
├─ docs/MIGRATION_V1_TO_V2.md
├─ Makefile.mingw
└─ README.md
```
