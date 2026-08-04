# Static buffer capacity formulas

Use these formulas to size caller-owned buffers exactly or conservatively.

Let:

```text
S = angular sides or slices
T = sphere stacks
H = capsule rings per hemisphere
X = plane X segments
Z = plane Z segments
M = torus major segments
N = torus minor segments
C = number of enabled caps: 0, 1, or 2
F = custom polyhedron triangle count
```

| Primitive | Vertices | Triangles |
|---|---:|---:|
| Triangle | 3 | 1 |
| Quad | 4 | 2 |
| Plane | `(X + 1)(Z + 1)` | `2XZ` |
| Disc | `S + 2` | `S` |
| Annulus | `2(S + 1)` | `2S` |
| UV sphere | `(S + 1)(T + 1)` | `2S(T - 1)` |
| Capsule / spherocylinder | `2(H + 1)(S + 1)` | `4SH` |
| Full frustum body | `2(S + 1)` | `2S` |
| Cone body | `2(S + 1)` | `S` |
| Each frustum cap | `S + 2` | `S` |
| Torus | `(M + 1)(N + 1)` | `2MN` |
| Box / cube | 24 | 12 |
| Prism | `S(4 + 3C)` | `S(2 + C)` |
| Pyramid | `3S(1 + C)` | `S(1 + C)` where `C` is 0 or 1 |
| Antiprism | `S(6 + 3C)` | `S(2 + C)` |
| Bipyramid | `6S` | `2S` |
| Wedge | 18 | 8 |
| Custom flat polyhedron | `3F` | `F` |

The mesh uses 16-bit indices and intentionally reserves index value 65,535, so a
single generated mesh may contain no more than 65,534 vertices.

## Example: capsule / spherocylinder

For 32 slices and 8 rings per hemisphere:

```text
vertices  = 2(8 + 1)(32 + 1) = 594
triangles = 4(32)(8)          = 1024
```

A safe declaration is therefore:

```c
static gp89_vertex vertices[594];
static gp89_triangle triangles[1024];
```
