# gskybox89 integration notes

## What the three layers are for

| Layer | Cost | Purpose |
|---|---:|---|
| `GSKYBOX89_LAYER_SCREEN` | 1 triangle | Cheapest clear/fallback/background gradient hook |
| `GSKYBOX89_LAYER_CUBE6` | 12 triangles | Classic 6-face skybox/cubemap-compatible shell; default fast path rotates 8 shared corners once |
| `GSKYBOX89_LAYER_DOME` | small configurable mesh | Horizon tint, fake atmosphere, cloud layer, color ramp |

## Why no translation?

The sky should look infinitely far away. Pass only camera rotation to `gskybox89_render`. Do not put the camera position into this matrix.

## Backend contract

`set_state` receives hints. The backend decides exact API calls.

Typical mapping:

```txt
lighting_enabled = 0       -> disable fixed-function light or bind unlit shader
fog_enabled = 0            -> no expensive fog unless the dome needs tinting
depth_write = 0            -> do not dirty depth buffer
depth_func = LEQUAL/EQUAL  -> let existing depth reject covered sky pixels
cull_mode = FRONT          -> see inside of the cube
blend_mode = ALPHA         -> dome overlay can blend over cube/screen
```

## Practical render order

Most engines should do this:

```txt
1. clear depth
2. render opaque world
3. render skybox with depth writes off
4. render alpha/particles
5. render HUD/UI
```

For very old/simple renderers, doing the sky first as a cheap background clear is acceptable, but it costs overdraw where the world later covers it.


## Cube fast path

`GSKYBOX89_LAYER_CUBE6` emits a cube as exactly two triangles per face:

```txt
6 faces * 2 triangles = 12 triangles
```

The default implementation does not rebuild/rotate four independent vertices per face. It rotates the 8 physical cube corners once, then emits face-local UVs and face IDs through an index table. That keeps texture binding simple while reducing CPU-side fixed-point matrix work.

Useful knobs:

```c
cfg.cube_fast_shared_vertices = 1;          /* default fast path */
cfg.cube_face_mask = GSKYBOX89_CUBE_ALL_FACES;
```

For fixed-camera rooms or custom portal culling, turn off faces you know cannot contribute. One active face is always exactly two emitted triangles.

## Texture strategies

### Six textures

Bind one texture per face inside `bind_face(layer, face)`.

### Atlas

Bind the atlas once and remap UVs inside the backend. This can save texture binds on older APIs.

### Native cubemap

Bind the cubemap once and sample with `(dir_x, dir_y, dir_z)`. Ignore UVs and face IDs if your shader does native cubemap sampling.

## Dome size

The dome uses a fixed 16-step trig table. You can request fewer segments/rings:

```c
cfg.dome_segments = 8;
cfg.dome_rings = 4;
```

Good defaults:

```txt
low-end:    8 segments, 4 rings
balanced: 12 segments, 5 rings
max here: 16 segments, 5 rings
```
