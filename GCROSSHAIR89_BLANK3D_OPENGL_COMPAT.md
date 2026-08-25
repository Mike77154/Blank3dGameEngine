# gcrosshair89: Blank3D OpenGL compatibility recipes

This revision restores the original Blank3D pistol crosshair without returning
to hardcoded HUD geometry.

## Recipe source

The vendor catalog remains 192 presets. Blank3D appends two project recipes:

- `blank3d_pistol_first_person`: gap 5 px, arm 8 px, 1 px white, no dot.
- `blank3d_pistol_third_person`: gap 8 px, arm 5 px, 1 px white, no dot.

Both reproduce the original `cx +/- 13` HUD endpoints. `pistol.ini` selects the
appropriate recipe with `crosshair_preset_first_person` and
`crosshair_preset_third_person`. `crosshair_preset` remains the general fallback.

## Primitive provider routing

Blank3D no longer occupies gcrosshair89's external primitive-provider slot.
Instead, its OpenGL callbacks are installed as the primitive fallback:

`external provider (optional) -> internal gcrosshair vector path -> Blank3D OpenGL primitives`

The callbacks terminate in the existing OpenGL HUD renderer (`GL_LINES`,
`GL_QUADS`, sprite provider). This keeps plug-in providers possible while
guaranteeing local rendering when none is registered.

## Line rasterization fix

`b3d_crosshair_line()` preserves the historical one-pixel `GL_LINES` path.
Axis-aligned lines thicker than one pixel use centered filled rectangles, which
removes the old even-width `-1,0` bias. Diagonal fallback offsets alternate
around the centerline.
