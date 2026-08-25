# Pickup render/GFO fix

The first Uzi pickup integration built the procedural meshes correctly, but the
GFO `*render -> draw_mesh()` host callback in `monika_blank3d.c` was a no-op.
A parallel `draw_pickups()` path existed, leaving two competing render paths
and making the GFO render contract false.

This revision makes GFO rendering authoritative for live pickup Objects:

- `b3d_object_draw_mesh()` recognizes pickup subject IDs and draws the native
  `Blank3DPickupInstance` parts.
- `set_camera()` now runs before `blank3d_objects_render()`, so GFO render
  callbacks use the current camera rather than stale matrix state.
- `draw_pickups()` is retained only as a fallback for a logical pickup whose
  GFO instance could not be made live.
- Pickup meshes are rendered unlit so the small dark test geometry remains
  visible in the dark demo scene.
- The GFO unit test now asserts that both pickup Objects dispatch `draw_mesh`
  during their render lifecycle.

No heap allocation or float/double simulation state was added.
