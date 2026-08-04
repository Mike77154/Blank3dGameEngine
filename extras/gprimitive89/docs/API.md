# API guide

## Fixed-point conventions

`gp89_fx` is signed Q16.16. One world unit is `GP89_FX_ONE`.
Angles are unsigned turns:

```text
0                    0 degrees
GP89_QUARTER_TURN    90 degrees
GP89_HALF_TURN       180 degrees
65535                just under 360 degrees
```

Use `gp89_fx_from_int` for whole values and `gp89_fx_mul` / `gp89_fx_div` for
fixed-point arithmetic.

## Mesh initialization

```c
gp89_mesh_init(&mesh,
                 vertex_buffer,
                 vertex_capacity,
                 triangle_buffer,
                 triangle_capacity);
```

The mesh stores pointers only. It never releases or resizes the buffers.
Every generator resets the counts before writing a new primitive.

## Generator return values

```text
GP89_OK                    success
GP89_ERR_ARGUMENT          invalid parameters or null pointer
GP89_ERR_VERTEX_CAPACITY   caller vertex buffer is too small
GP89_ERR_TRI_CAPACITY      caller triangle buffer is too small
GP89_ERR_INDEX_RANGE       result cannot fit the 16-bit index contract
GP89_ERR_DEGENERATE        reserved for degenerate-input checks
```

The same status is written to `mesh.status` for capacity and index failures.

## Surface generators

```c
gp89_make_triangle
gp89_make_quad
gp89_make_plane
gp89_make_disc
gp89_make_annulus
```

The plane lies on XZ with a +Y normal. Triangle and quad lie on XY with a +Z
normal. Disc and annulus lie on XZ with a +Y normal.

## Rounded generators

```c
gp89_make_uv_sphere
gp89_make_capsule
gp89_make_spherocylinder
gp89_make_rounded_cylinder
gp89_make_cylinder
gp89_make_cone
gp89_make_frustum
gp89_make_torus
```

Sphere, capsule, cylinder body, cone body, frustum body, and torus have a UV seam
at the first/last angular column. Seam vertices are duplicated so U can run from
zero through one without discontinuous values on one vertex.

`gp89_make_capsule`, `gp89_make_spherocylinder`, and
`gp89_make_rounded_cylinder` take:

```text
radius
straight cylinder height
angular slices
rings per hemisphere
```

A zero straight height is allowed and produces a sphere-like capsule topology.
The three function names are aliases of the same implementation and produce
identical vertices, indices, normals, tangents, UVs, colors, and materials.

## Faceted generators

```c
gp89_make_box
gp89_make_cube
gp89_make_prism
gp89_make_pyramid
gp89_make_antiprism
gp89_make_bipyramid
gp89_make_wedge
gp89_make_regular_polyhedron
gp89_make_custom_flat
```

Faceted generators duplicate vertices across hard boundaries. This allows a
single position to carry different face normals and UV values.

## Regular and semi-regular polyhedron identifiers

```text
GP89_POLY_TETRAHEDRON
GP89_POLY_OCTAHEDRON
GP89_POLY_DODECAHEDRON
GP89_POLY_ICOSAHEDRON
GP89_POLY_CUBOCTAHEDRON
GP89_POLY_RHOMBIC_DODECAHEDRON
GP89_POLY_TRUNCATED_OCTAHEDRON
```

## Appearance

```c
gp89_color_all
gp89_color_axis_gradient
gp89_material_all
gp89_material_cycle
gp89_uv_transform
gp89_uv_planar
```

Colors are stored per vertex. Materials are stored per triangle. The renderer
adapter decides how a material number maps to an engine material, shader, or
texture set.

## Transform and geometry processing

```c
gp89_translate
gp89_scale
gp89_rotate_xyz
gp89_apply_transform
gp89_reverse_winding
gp89_recalculate_smooth_normals
gp89_recalculate_tangents
```

Non-uniform scale applies an inverse-scale correction to normals before
renormalizing them.

## Deformation

```c
gp89_deform_taper_y
gp89_deform_twist_y
gp89_deform_shear
gp89_deform_spherize
gp89_deform_wave_y
gp89_deform_custom
```

The custom callback receives a mutable vertex, its index, and a caller pointer.
No temporary allocation is performed.

## OBJ exporter

Include `gprimitive89_obj.h` and compile `src/gprimitive89_obj.c` to enable:

```c
gp89_write_obj(file, &mesh, "object_name");
```

The exporter writes positions, UV0, normals, and indexed triangle faces. Vertex
colors and material slots remain in the in-memory mesh and are not serialized by
this minimal OBJ helper.
