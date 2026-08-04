# Engine integration notes

## Recommended ownership split

```text
configuration / editor
        |
        v
primitive descriptor
        |
        v
gprimitive89
        |
        +--> static vertex arena
        +--> static triangle arena
        |
        v
engine mesh resource
        |
        +--> renderer backend
        +--> collision backend
        +--> serialization
```

The primitive library should not become the owner of world transforms, entity
lifetime, renderer handles, or collision objects. It only materializes local
mesh geometry.

## Adapter mapping

A renderer adapter commonly maps:

```text
gp89_vertex.x/y/z      -> POSITION
gp89_vertex.nx/ny/nz   -> NORMAL
gp89_vertex.tx/ty/tz/tw-> TANGENT
gp89_vertex.u/v        -> TEXCOORD0
gp89_vertex.r/g/b/a    -> COLOR0
gp89_triangle.a/b/c    -> index buffer
gp89_triangle.material -> submesh/material selector
```

The source is Q16.16. A fixed-point renderer can consume it directly. A backend
that requires another representation should convert inside its upload adapter,
not inside `gprimitive89`.

## Working with a central transform provider

When another engine library owns transform math:

1. Generate the primitive in local space.
2. Leave `gp89_translate`, `gp89_scale`, and `gp89_rotate_xyz` unused.
3. Store the mesh as immutable local geometry.
4. Apply the central provider's model transform at rendering, collision build,
   or instance update time.

This avoids two competing transform authorities.

## Editable primitives

For an editor, retain the primitive parameters alongside the generated mesh:

```text
kind = capsule              # aliases: spherocylinder, rounded_cylinder
radius = 0.5
cylinder_height = 2
slices = 32
hemisphere_rings = 8
```

When a parameter changes, reset the same arena-backed mesh and regenerate it.
No heap churn is necessary.

## Texturing

Curved surfaces include continuous UV0 coordinates with explicit angular seam
vertices. Hard solids use face-local UV islands. For atlas packing or a custom
projection, call `gp89_uv_planar` or replace UV values in the adapter/editor.

## Collision use

The generated triangle list can feed a triangle-mesh collider, but many runtime
objects should use simpler collision primitives:

```text
visual capsule / spherocylinder -> analytic capsule collider
visual sphere   -> analytic sphere collider
visual box      -> analytic box collider
visual cylinder -> engine-specific cylinder or convex hull
```

This library intentionally does not mix mesh generation with collision queries.
