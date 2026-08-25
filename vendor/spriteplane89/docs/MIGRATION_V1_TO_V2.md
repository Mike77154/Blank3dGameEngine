# Migration v1 to v2

## API compatibility

Core v1 calls remain available:

- `sprpl89_init`
- `sprpl89_set_camera`
- `sprpl89_set_tick_ms`
- `sprpl89_add_frame`
- `sprpl89_add_frame_px`
- `sprpl89_spawn_sprite`
- `sprpl89_get_sprite`
- `sprpl89_kill_sprite`
- `sprpl89_clear_sprites`
- `sprpl89_add_vector_rect`
- `sprpl89_add_vector_line`
- `sprpl89_add_vector_tri`
- `sprpl89_build_sort`
- `sprpl89_sort_far_to_near`
- `sprpl89_emit_begin`
- `sprpl89_emit_sprite`
- `sprpl89_emit_all_sorted`
- `sprpl89_select_view_index`
- `sprpl89_frame_for_sprite`

## Struct changes

`sprpl89_sprite` gained:

- `xflags`
- `render_queue`
- `blend_mode`
- `sort_layer`
- `sort_order`
- `velocity`
- `surface_right`
- `surface_up`
- `cull_distance`
- `fade_near_distance`
- `fade_far_distance`

If old code zero-initializes the struct then sets the known fields, v2 fills sane defaults in `sprpl89_spawn_sprite()`.

## Emit changes

`sprpl89_emit` gained:

- `packets[]`
- `points[]`
- `packet_count`
- `point_count`
- extra dropped counters

Old renderers can keep reading only `verts` and `tris`, but new renderers should use `packets` for queues/blend/material/page metadata.

## No DSL included

v2 intentionally does not include a DSL parser. Use `sprpl89_hooks` or fill structs from any DSL/runtime.
