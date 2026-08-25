# gskybox89

`gskybox89` is a tiny C89/fixed-point/no-heap sky background helper for old-school or constrained 3D engines.

It mixes three practical sky techniques:

1. **Level 2: cube / 6-face skybox**  
   Six ordinary faces, exactly 2 triangles per face / 12 triangles total. The default cube path now rotates only the 8 physical cube corners once, then emits indexed face triangles.

2. **Level 3: screen-space sky background**  
   One oversized fullscreen triangle. Useful as the cheapest fallback, a color clear replacement, or a shader/backend hook.

3. **Level 4: skydome-lite overlay**  
   A tiny hemisphere made from a fixed 16-step trig table. Good for horizon tint, fog color, fake atmosphere, cloud-band overlays, or day/night color ramps.

## Constraints

- C89-compatible API and implementation.
- No `malloc`, `free`, `realloc`, heap ownership, or hidden allocation.
- No `float` or `double`.
- Fixed-point math, Q20.12-ish using `long` and `GSKYBOX89_FX_ONE = 4096`.
- Backend agnostic: you receive render-state hints and triangles through callbacks.
- Cube fast path: 8 shared corner rotations instead of 24 per-face corner rotations.
- No renderer assumptions: OpenGL, D3D, software rasterizer, PS1-ish renderer, SDL blitter, or custom engine bridge can consume the same emitted triangles.

## Recommended render order

For hardware saving, call the sky pass after opaque world geometry when your depth buffer already contains walls, props, enemies and weapons:

```c
render_opaque_world();
gskybox89_render(&sky, &camera_rotation_only);
render_alpha_world();
render_ui();
```

Suggested backend state:

```txt
Depth test: enabled
Depth write: disabled
Depth func: LEQUAL, or EQUAL for pure screen background on untouched depth
Lighting: disabled
Fog: disabled, unless you intentionally tint the dome layer
Cull: front-face cull for inside-out cube, or no cull for dome
```

## Files

```txt
gskybox89/
├─ include/gskybox89.h
├─ include/gskybox89_assets.h
├─ src/gskybox89.c
├─ src/gskybox89_assets.c
├─ demo/demo_null_backend.c
├─ demo/demo_preview_ppm.c
├─ demo/demo_asset_mapper.c
├─ demo/demo_asset_preview_ppm.c
├─ tests/test_build.c
├─ docs/integration_notes.md
├─ docs/asset_conventions.md
├─ tools/convert_png_faces_to_ppm.py
├─ Makefile
└─ LICENSE
```

## Build

```sh
make
make check
./demo_null_backend
./demo_preview_ppm
./demo_asset_mapper Sky_Night01FT.vmt Sky_Night01BK.vmt px.png nx.png
./demo_asset_preview_ppm out.ppm Sky_Night01RT.tga Sky_Night01LF.tga Sky_Night01UP.tga Sky_Night01DN.tga Sky_Night01FT.tga Sky_Night01BK.tga
```

On MSYS2/MinGW32:

```sh
gcc -std=c89 -Wall -Wextra -pedantic -O2 -Iinclude src/gskybox89.c demo/demo_null_backend.c -o demo_null_backend.exe
```

## Minimal usage

```c
Gskybox89_Context ctx;
Gskybox89_Config cfg;
Gskybox89_Backend be;
Gskybox89_Mat3 rot;

gskybox89_default_config(&cfg);
cfg.layer_mask = GSKYBOX89_LAYER_HYBRID;
cfg.cube_fast_shared_vertices = 1; /* default: 8 shared cube corners */
cfg.cube_face_mask = GSKYBOX89_CUBE_ALL_FACES;

be.user = your_renderer;
be.begin_pass = your_begin_pass;
be.end_pass = your_end_pass;
be.set_state = your_set_state;
be.bind_face = your_bind_face;
be.emit_tri = your_emit_tri;

gskybox89_mat3_identity(&rot); /* use camera rotation only, never camera translation */
gskybox89_init(&ctx, &cfg, &be);
gskybox89_render(&ctx, &rot);
```


## Cube fast path

The cube layer is intentionally the tiny mesh version: six faces, two triangles per face.

```txt
physical cube corners rotated: 8
logical face vertices emitted: 24
triangles emitted: 12
heap allocations: 0
```

Default cube path:

```c
cfg.layer_mask = GSKYBOX89_LAYER_CUBE6;
cfg.cube_fast_shared_vertices = 1;
cfg.cube_face_mask = GSKYBOX89_CUBE_ALL_FACES;
```

For conservative debugging you can force the old safe path, which still emits exactly 12 triangles but recalculates each face corner separately:

```c
cfg.cube_fast_shared_vertices = 0;
```

For fixed cameras, portals, or engine-specific visibility, `cube_face_mask` can skip known-unused faces. Each active face costs exactly two triangles.

## Face order

```txt
GSKYBOX89_FACE_POS_X
GSKYBOX89_FACE_NEG_X
GSKYBOX89_FACE_POS_Y
GSKYBOX89_FACE_NEG_Y
GSKYBOX89_FACE_POS_Z
GSKYBOX89_FACE_NEG_Z
```

The backend may bind six separate textures, six atlas rects, or a real cubemap handle. The library only emits face IDs and direction vectors.

## Notes for a PS1-ish renderer

Use `GSKYBOX89_LAYER_CUBE6` with six ordinary textures or a single atlas. Disable lighting and Z writes. Keep the cube centered on the camera by ignoring camera translation. For fixed-camera rooms, you can use only `GSKYBOX89_LAYER_SCREEN` as the cheapest background.

## Notes for a modern renderer

Use the emitted direction vectors as cubemap lookup vectors, or ignore UVs and bind a native cubemap texture. The fullscreen layer can become a shader-only sky path using an inverse projection matrix in your backend.


## Asset convention helper

`gskybox89_assets` maps real-world skybox file names to the six internal faces without heap allocation.

Supported naming styles:

```txt
Source/Valve: FT BK LF RT UP DN
Axis cubemap: px nx py ny pz nz
Word names:   right left up down front back top bottom
```

Source VMF/VMT helpers are included:

```c
char skyname[64];
char basetexture[96];
gskybox89_asset_vmf_extract_skyname("map.vmf", skyname, sizeof(skyname));
gskybox89_asset_vmt_extract_basetexture("Sky_Night01FT.vmt", basetexture, sizeof(basetexture));
```

The C89 preview demo decodes only uncompressed BMP/TGA and PPM P6. PNG/JPG/VTF
are mapper-only by design; a real engine should load those through its existing
texture system. For the preview demo, `tools/convert_png_faces_to_ppm.py` can
convert `px/nx/py/ny/pz/nz.png` files into PPM.

Per-face UV transforms are available for engine-specific conventions:

```c
gskybox89_cube_set_uv_xform(&cfg, flip_u_mask, flip_v_mask, swap_uv_mask);
```

See `docs/asset_conventions.md`.
