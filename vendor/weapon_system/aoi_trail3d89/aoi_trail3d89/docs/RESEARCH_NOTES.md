# Research notes

This implementation follows common real-time VFX trail patterns found in mainstream engines, adapted to a deterministic C89/fixed-point/no-heap runtime.

## Trail point sampling

Unity's Trail Renderer uses a minimum vertex distance to decide how far an object must travel before a new trail segment is added. Unity also notes that low values create smoother trails, high values create jagged trails, and the largest visually acceptable value is better for performance.

trail3d89 maps this to:

```txt
desc.min_dist
desc.sampler_mask |= T3D89_SAMPLE_DISTANCE
desc.lod_vertex_budget
```

## Lifetime, width and color along the trail

Unity exposes trail lifetime, width over the trail, and color gradient sampled at vertices. trail3d89 maps this to fixed-point integer fields:

```txt
desc.life_ticks
desc.width_head / desc.width_tail
desc.color_head / desc.color_tail
```

No floating curves are used; this keeps the library C89/fixed-point.

## Ribbon, cross ribbon, tube

Godot's 3D particle trails distinguish ribbon trails and tube trails. Godot's RibbonTrailMesh can be flat or cross-shaped; its TubeTrailMesh is cylindrical and uses radial steps/sections/rings.

trail3d89 maps this to:

```txt
T3D89_MODE_VIEW_RIBBON
T3D89_MODE_AXIS_RIBBON
T3D89_MODE_ORIENTED_RIBBON
T3D89_MODE_CROSS_RIBBON
T3D89_MODE_TUBE_LITE
```

`TUBE_LITE` intentionally emits a cheap 4-sided diamond tube rather than a high-resolution cylinder. This matches the low-level C89 budget goal.

## Ribbon facing and UV by distance

Unreal Niagara's Ribbon Renderer exposes ribbon facing modes and UV tiling based on distance traversed by the ribbon. trail3d89 maps this to:

```txt
view-facing ribbon builder
axis/oriented ribbon variants
desc.uv_mode = T3D89_UV_BY_DISTANCE
desc.uv_tile_dist
```

## Why curvature sampling exists

Distance-only sampling can miss sharp turns if the object moves slowly but changes direction. Time-only sampling can oversample straight motion. Curvature sampling adds samples when a new point deviates from the line predicted by the last two points. It is a cheap integer approximation of adaptive tessellation.

```txt
desc.sampler_mask |= T3D89_SAMPLE_CURVE
desc.curve_dist = threshold
```

## Source pages reviewed

- Unity Manual: Trail Renderer component.
- Godot docs: 3D Particle trails, RibbonTrailMesh, TubeTrailMesh.
- Epic docs: Niagara Ribbon Renderer render module reference.
