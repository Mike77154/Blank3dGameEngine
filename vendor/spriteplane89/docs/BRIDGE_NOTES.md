# spriteplane89 v2 bridge notes

## What this library emits

`spriteplane89` does not draw by itself. It outputs:

- `sprpl89_vertex` array
- triangle index array
- `sprpl89_packet` array with material/page/queue/blend/owner metadata
- optional `sprpl89_point_cmd` array for point-sprite expansion
- dropped/culling counters for diagnostics

Your engine maps those to whatever backend you use: software rasterizer, OpenGL, Direct3D, Genesis-style renderer, custom fixed pipeline, etc.

## Hook management contract

Hooks are the external control surface for DSLs and editors.

```text
DDSL2 / RPYL / editor / hardcoded AI / config table
        ↓
sprpl89_hooks
        ↓
spriteplane89 emit phase
```

Available hooks:

- `cull_sprite`: external culling, portal visibility, sector tests, occlusion tests.
- `resolve_frame`: custom animation table, scripted atlas selection, state machine frames.
- `mutate_sprite`: temporary per-frame changes without modifying stored sprite state.
- `post_emit`: telemetry, debug, collecting emitted ranges.
- `depth_fade`: soft-particle/depth fade bridge.

The library never owns the hook user pointer.

## Suggested render order

1. Opaque world geometry.
2. Alpha-test sprites that can depth-write, such as cutout grass or solid monster sprites.
3. Alpha-blend/additive sprites sorted far-to-near.
4. Optional HUD/overlay sprites with depth disabled.

`sprpl89_sort_for_render()` sorts by queue/layer/order and uses far-to-near only for transparent-style queues.

## Modes

- `SP89_BILLBOARD_Y_AXIS`: best for Doom-style enemies, signs, trees, pickups.
- `SP89_BILLBOARD_VIEW_ALIGNED`: best for particles, fire, smoke, magic circles.
- `SP89_BILLBOARD_CAMERA_FACING`: good for round-ish impostors and objects near camera.
- `SP89_BILLBOARD_FIXED`: literal 3D plane at a fixed orientation, useful for posters and decals.
- `SP89_BILLBOARD_AXIS_LOCKED`: sparks, beams, or special planes rotating around a custom axis.
- `SP89_BILLBOARD_VELOCITY`: tracers, sparks, slashes, motion sheets.
- `SP89_BILLBOARD_SURFACE`: decals/panels aligned to a supplied surface basis.
- `SP89_BILLBOARD_CROSSED_Y`: two crossed quads for cheap volume: grass, flame columns, bushes.

## Doom-like rotations

Use:

- `SP89_FRAME_VIEW_8` for 8-direction monster sprites.
- `SP89_FRAME_VIEW_16` for smoother rotation sets.
- `SP89_FRAME_ANIM_VIEW_8` / `16` for animation frames per view.

Frame layout is:

```text
base_frame + view_index * frames_per_view + anim_index
```

So a monster with 8 views and 4 walk frames uses 32 atlas frames.

## Text

Text is atlas-based. Generate a bitmap or SDF/MSDF atlas offline, then register glyph frames/metrics with:

- `sprpl89_add_font`
- `sprpl89_font_set_glyph`
- `sprpl89_font_set_ascii_grid`
- `sprpl89_add_text`

The library emits one quad per glyph and tags it as `SP89_MAT_TEXT` / `SP89_FLAG_TEXT_LAYER`.

## Panels

`sprpl89_add_panel_9slice()` emits 9 quads over the owning sprite plane. Useful for:

- signs
- terminals
- speech balloons
- world-space UI
- VN-style text panels inside 3D

## Picking and collisions

`sprpl89_pick_sprite_ray()` intersects a ray with the visual plane and returns local/UV coordinates. For gameplay collision, keep using real primitives from your engine:

```text
visual sprite plane + collision capsule/AABB/cylinder
```

Use picking for mouse/use/aim interaction, not as a full physics substitute.

## Point commands

`SP89_XFLAG_POINT_COMMAND` emits an entry in `emit.points` instead of a quad. This is ideal for particles if the renderer can expand points. Otherwise, do not use that flag and v2 emits classic quads.
