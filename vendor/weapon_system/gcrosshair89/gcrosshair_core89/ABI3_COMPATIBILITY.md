# gcrosshair_core89 ABI 3 compatibility

`GC89_CORE_ABI_VERSION` and `GC89_TYPES_ABI_VERSION` are both 3.

ABI 3 preserves every vector-shape field introduced in ABI 2 and appends
renderer-agnostic outline fields to `GC89_Variant` and `GC89_DrawSpec`.
All three libraries and every consumer sharing `gcrosshair89_types.h` must be
rebuilt together.

## Preserved ABI 2 shape fields

- `shape_type`
- `shape_segment_mask`
- `shape_direction_mask`
- `spread_mode`
- `shape_radius_x_fx`, `shape_radius_y_fx`
- `shape_depth_fx`
- `shape_rotation_deg_fx`
- `shape_break_fx`

## ABI 3 outline fields

- `outline_enabled`
- `outline_width_fx`
- `outline_color_rgba`

The core emits outline geometry first and normal geometry second through the
existing callbacks. No callback signature changed. The original cross, all ABI 2
shapes, dot, image and hybrid paths remain supported. Image outlining is not
performed by the core.
