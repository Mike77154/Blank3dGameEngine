# Blank3D muzzle lifetime, flash illumination, and casing render-state fix

## Scope

This patch fixes two regressions observed after wiring the raster muzzle image pipeline and adds a real short-lived scene light for weapon flashes.

## 1. Persistent muzzle ghost: root cause and fix

`SpritePlane89` correctly killed finite-life sprites, but the host-owned `sprpl89_emit` command buffer was not reset between Blank3D world sprite passes. `sprpl89_emit_all_sorted()` appends to the caller's buffer by design, so triangles emitted by a muzzle flash survived after the sprite instance itself had expired.

`blank3d_spriteplanes_emit()` now begins every complete Blank3D world-sprite pass with `sprpl89_emit_begin(out)`. A regression test verifies that the pistol muzzle emits two packets while alive (core + glow) and zero packets/triangles/vertices after its lifetime expires.

## 2. Casing regression: additive state leaked into the following 3D pass

World SpritePlanes are rendered immediately before casings. The previous raster SpritePlane path enabled additive blending for the muzzle and did not restore OpenGL state before returning. Consequently, casings and later 3D geometry could inherit the muzzle's additive blend state, making brass look transparent/faded or otherwise visually broken even though VPhysics continued to integrate the casing bodies.

`blank3d_image_gl_draw_spriteplanes()` now owns and restores its presentation state:

- disables lighting while drawing raster sprites (full-bright presentation),
- keeps depth testing but disables depth writes for transparent/additive packets,
- restores depth writes,
- disables texturing and blending,
- restores normal alpha blend function and white color,
- restores depth testing and lighting.

`bridge_gl_begin_frame()` also establishes a deterministic 3D render baseline each frame.

The casing solver itself was not replaced or bypassed. `test-casing-physics` still verifies ejection impulse, gravity, floor rebound, angular damping, draw matrix generation, and sleep.

## 3. Real muzzle illumination pulse

The sprite glow and scene illumination are separate systems.

A new fixed-point runtime module, `blank3d_muzzle_light`, stores:

- finite lifetime,
- world-space muzzle position,
- peak intensity (Q16),
- radius (Q16),
- RGB color.

Its envelope is a fast quadratic decay, preserving one complete render frame at peak energy. There is no heap allocation and no floating-point math in the simulation module.

At the OpenGL boundary, `bridge_gl_set_muzzle_light()` maps the fixed-point sample to `GL_LIGHT1`, with constant/linear/quadratic attenuation. Floating point exists only in this renderer bridge, matching the existing Blank3D graphics boundary.

### Weapon recipe keys

```ini
muzzle_image_glow=1
muzzle_image_glow_scale=1.55
muzzle_image_glow_alpha=160

muzzle_light=1
muzzle_light_ms=72
muzzle_light_intensity=3.25
muzzle_light_radius=9.0
muzzle_light_r=255
muzzle_light_g=205
muzzle_light_b=112
```

The pistol uses the original progressive JPEG muzzle source directly:

`config/weapons/assets/pistol_muzzle_flash.jpg`

## Verification

- `make test-pistol-muzzle-flash` — PASS
- `make test-casing-physics` — PASS
- `make test-muzzle-image-pipeline` — PASS
- `make test-image-stack` — PASS
- `make test-sprite-runtime89` — PASS
- `make syntax-check` — PASS
- `make syntax-check-sprite-runtime89-win32` — PASS (Win32 preprocessor/stub syntax gate)
- `blank3d_muzzle_light.[ch]` protocol scan for heap/float/double/64-bit declarations — PASS

A native i686-w64-mingw32 cross compiler was not available in the build container, so final interactive Win32 behavior should still be confirmed on the user's MSYS2/MinGW32 machine.
