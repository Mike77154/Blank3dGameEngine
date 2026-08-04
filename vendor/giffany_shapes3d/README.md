# Giffany Shapes3D 1.0

A small, renderer-agnostic 3D primitive and collision library written in strict C89.

## Hard constraints

- C89 source.
- No `malloc`, `calloc`, `realloc`, `free`, arenas, or hidden heap use.
- No `float`, `double`, or `long double`.
- Signed Q16.16 fixed point.
- Caller-supplied static vertex and index arrays.
- No graphics API dependency.
- No operating-system dependency in the library.

The demo uses only `stdio` to export OBJ files. The library source and public header include no standard headers at all.

## Included mesh generators

- Triangle.
- Quad / square plane.
- Subdivided grid plane.
- Disc.
- Box / cube.
- Regular prism with any base from 3 sides upward.
- Regular pyramid with any base from 3 sides upward.
- Bipyramid; a four-sided bipyramid gives an octahedron.
- Frustum.
- Cylinder.
- Cone.
- UV sphere.
- Upper or lower hemisphere, optionally capped.
- Capsule.

All generated triangle meshes contain:

- Q16.16 positions.
- Q16.16 normals.
- Q16.16 UV coordinates.
- RGBA8 per-vertex color.
- 16-bit indices.

Shapes are centered at the origin and use a Y-up coordinate system. Planes lie on XZ.

## Effects and editing

- Uniform recolor.
- Vertical color gradient.
- UV checker recolor.
- UV scale and offset.
- Translation, XYZ rotation, and scale through `g3d_transform`.

Texture pixels are deliberately not owned by this library. It provides UVs so OpenGL,
Direct3D, a software rasterizer, or another renderer can consume the same mesh.

## Collision/query support

- Point/AABB.
- AABB/AABB.
- Sphere/sphere.
- Sphere/AABB.
- Point/capsule.
- Capsule/sphere.
- Capsule/capsule.
- Sphere/plane.
- Ray/plane.
- Ray/sphere.
- Ray/AABB.
- Ray/triangle.
- Ray/triangle-mesh, returning the nearest hit and triangle number.

## Memory model

You choose the hard limits at compile time by declaring arrays:

```c
#define MAX_VERTICES 4096
#define MAX_INDICES  12288

static g3d_vertex vertices[MAX_VERTICES];
static g3d_index indices[MAX_INDICES];
static g3d_mesh mesh;

g3d_mesh_init(&mesh, vertices, MAX_VERTICES, indices, MAX_INDICES);
```

No allocation is performed. A generator returns `G3D_ERR_CAPACITY` before writing past
an array. Exact requirement helpers let you check a buffer before generation:

```c
g3d_mesh_requirements needed;
needed = g3d_require_capsule(24U, 6U);

if (g3d_mesh_has_capacity(&mesh, needed)) {
    g3d_make_capsule(&mesh, radius, body_height, 24U, 6U, color);
}
```

## Fixed-point basics

```c
g3d_fx one = g3d_fx_from_int(1);
g3d_fx one_and_half = g3d_fx_from_ratio(3, 2);
g3d_fx product = g3d_fx_mul(one_and_half, g3d_fx_from_int(2));
```

The fixed multiply and divide routines do not require a 64-bit integer type. Out-of-range
results saturate to `G3D_FX_MIN` or `G3D_FX_MAX`.

## Generate a capsule

```c
g3d_color white;
int result;

white = g3d_color_rgba(255U, 255U, 255U, 255U);
result = g3d_make_capsule(&mesh,
                          g3d_fx_from_int(1),
                          g3d_fx_from_int(2),
                          24U,
                          6U,
                          white);
```

`body_height` is the distance between the centers of the two hemispherical ends. Total
capsule height is `body_height + 2 * radius`.

## Generic pyramids

```c
g3d_make_pyramid(&mesh, radius, height, 3U, color); /* triangular */
g3d_make_pyramid(&mesh, radius, height, 4U, color); /* square */
g3d_make_pyramid(&mesh, radius, height, 5U, color); /* pentagonal */
g3d_make_pyramid(&mesh, radius, height, 6U, color); /* hexagonal */
```

The same function supports higher regular polygon bases, subject only to your static
mesh capacity and the 16-bit index limit.

## Build with MSYS2/MinGW32

```sh
make
make check
./demo
```

Or run:

```bat
build_mingw32.bat
```

The demo writes OBJ files for the major shapes, allowing inspection in Blender or any
OBJ viewer. OBJ export is demonstration code and is not required by the library.

## Direct renderer integration

Read the arrays after generation:

```c
mesh.vertices[i].position
mesh.vertices[i].normal
mesh.vertices[i].uv
mesh.vertices[i].color
mesh.indices[i]
```

Convert Q16.16 only at the final API boundary when a renderer requires another numeric
format. A software renderer can consume the values directly.

## Practical limits

- `g3d_index` is 16-bit, so one mesh cannot exceed 65,535 vertices.
- Q16.16 represents approximately -32768 through +32767.99998.
- Multiplication and vector operations saturate when the representable range is exceeded.
- Primitive overlap tests normalize distance comparisons where practical, avoiding the
  common Q16.16 failure where two very large squared distances both saturate.
- Ray/mesh is a simple linear triangle scan. Use `g3d_mesh_compute_aabb` as broad phase,
  or place meshes in your own grid/BVH when scene size grows.
- Non-uniform scale followed by normal rotation is normalized, but this compact transform
  helper does not calculate a full inverse-transpose normal matrix.

## Source notes

The vertex layout intentionally separates position, normal, texture coordinates, and
color because graphics APIs consume per-vertex attribute streams. The ray/triangle query
uses the Moller-Trumbore barycentric method. Capsule tests reduce the rounded shape to
closest-point tests on its axis segment plus the combined radii. See `SOURCES.md` for the
technical references used while designing those interfaces.

## License

CC0 1.0 Universal. See `LICENSE`.
