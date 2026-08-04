# SICOL DE

Librería de colisión determinista en C89-ish, sin floats, con fixed-point Q20.12.

## Qué trae ahora

- AABB / OBB / Plane / Ray / Sphere / Capsule / Segment / **Triangle / Mesh**
- **Convex hulls** (`SICOL_SHAPE_CONVEX`)
- Broadphase sweep-and-prune con poda 3D
- Narrowphase analítica + **ruta genérica GJK/EPA**
- Raycasts de escena + **raycast GJK** para convexos support-mapped
- **Triangle mesh** con BVH y shape `SICOL_SHAPE_MESH`
- **Raycast / overlap / shape-cast** contra triángulos y mesh, con casts refinados por triángulo
- **Refit de BVH** para mesh dinámica/deformable ligera sin reconstruir toda la topología
- **Shape-cast / TOI** por par y de escena
- World estático con recycling de IDs, layers/masks y **manifolds persistentes**
- **Warm start / cache de GJK**
- Solver lineal + angular con **CCD**, islands y sleeping
- **Tensor de inercia local 3x3 configurable**
- **Fuerzas, torque, gravedad y damping** por solver-body
- **Joints**:
  - distance joint
  - **spring joint** dedicado
  - point / ball-socket joint
  - **fixed joint** (posición + orientación ancladas)
  - **hinge joint** con **límites angulares** y **motor**
  - **slider / prismatic joint** con límites lineales y motor
  - **cone-twist joint** con límite de swing y límites de twist

## Novedades de la mutación quimera

### Fixed joint
Bloquea una pose contra otro body o contra el mundo (`id_b = -1`).

```c
fx local_anchor[3];
fx world_anchor[3];

fx_zero3(local_anchor);
fx_set3(world_anchor, FX_FROM_INT(4), 0, 0);

sicol_world_joint_create_fixed(
    &world,
    id_body, local_anchor,
    -1, world_anchor,
    FX_ONE,
    0
);
```

### Hinge joint
Deja libre la rotación sobre un eje, pero bloquea el resto del frame angular.

```c
fx local_anchor[3];
fx local_axis[3];
fx world_anchor[3];
fx world_axis[3];

fx_zero3(local_anchor);
fx_set3(local_axis, 0, 0, FX_ONE);
fx_zero3(world_anchor);
fx_set3(world_axis, 0, 0, FX_ONE);

sicol_world_joint_create_hinge(
    &world,
    id_body,
    local_anchor,
    local_axis,
    -1,
    world_anchor,
    world_axis,
    FX_ONE,
    FX_FROM_RATIO(1, 8)
);
```

### Límites angulares
```c
sicol_world_joint_set_hinge_limits(
    &world,
    joint_id,
    -FX_FROM_RATIO(1, 8),
     FX_FROM_RATIO(1, 8)
);
```

### Motor de bisagra
```c
sicol_world_joint_set_hinge_motor(
    &world,
    joint_id,
    FX_FROM_RATIO(-3, 2),
    FX_FROM_INT(8)
);
```

## Solver

```c
sicol_solver_body_t bodies[SICOL_WORLD_MAX];
sicol_solver_config_t cfg;
sicol_solver_stats_t stats;

sicol_solver_config_default(&cfg);
cfg.dt = FX_FROM_RATIO(1, 60);
fx_set3(cfg.gravity, 0, FX_FROM_INT(-10), 0);

sicol_world_step_solver(&world, bodies, &cfg, &stats);
```

`stats` incluye, entre otras cosas:

- `joint_count`
- `total_joint_impulse`
- `total_angular_impulse`
- `island_count`
- `ccd_hits`

## Build

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Compile manual estricta

```bash
cc -std=c89 -Wall -Wextra -Wpedantic -I. \
  sicol_math_fx.c sicol_shape.c sicol_mesh.c sicol_convex.c sicol_gjk.c sicol_cast.c \
  sicol_broadphase.c sicol_narrowphase.c sicol_raycast.c sicol_world.c \
  sicol_solver.c sicol-de_compat.c tests/test_sicol.c \
  -o /tmp/test_sicol && /tmp/test_sicol
```

## Headers principales

- `sicol.h`
- `sicol_shape.h`
- `sicol_mesh.h`
- `sicol_convex.h`
- `sicol_gjk.h`
- `sicol_cast.h`
- `sicol_world.h`
- `sicol_solver.h`

## Notas

- Planes siguen como half-spaces analíticos.
- La EPA mantiene fallback aproximado cuando el politopo degenera por fixed-point.
- Los joints nuevos ya permiten cosas más pesadas de gameplay: locks de pose, bisagras, topes y motores compactos.
- El solver sigue siendo rígido y determinista, no un stack AAA completo de constraints avanzadas con todo el paquete de articulaciones complejas.

## Novedades del siguiente zarpazo

### Spring joint
Versión dedicada del vínculo elástico entre anclas. Útil cuando quieres una semántica de resorte explícita en lugar de reciclar `distance`.

```c
sicol_world_joint_create_spring(
    &world,
    id_a, local_anchor_a,
    id_b, local_anchor_b,
    FX_FROM_INT(4),
    FX_ONE,
    FX_FROM_RATIO(1, 8)
);
```

### Slider / prismatic joint
Bloquea la traslación perpendicular al eje, conserva el deslizamiento axial y puede usar límites + motor.

```c
int slider_id = sicol_world_joint_create_slider(
    &world,
    id_body, local_anchor, local_axis,
    -1, world_anchor, world_axis,
    FX_ONE,
    0
);

sicol_world_joint_set_slider_limits(
    &world, slider_id,
    FX_FROM_INT(-2),
    FX_FROM_INT( 2)
);

sicol_world_joint_set_slider_motor(
    &world, slider_id,
    FX_FROM_INT(2),
    FX_FROM_INT(8)
);
```

### Cone-twist joint
Ancla el punto como una ball-socket, limita el swing dentro de un cono y además puede limitar el twist alrededor del eje principal.

```c
int ct_id = sicol_world_joint_create_cone_twist(
    &world,
    id_body, local_anchor, local_axis,
    -1, world_anchor, world_axis,
    FX_FROM_RATIO(1, 8),
    FX_ONE,
    0
);

sicol_world_joint_set_cone_twist_limits(
    &world, ct_id,
    FX_FROM_RATIO(1, 8),
    -FX_FROM_RATIO(1, 8),
     FX_FROM_RATIO(1, 8)
);
```


## Novedades del salto de colisiones

### Triangle shape
Ahora existe `SICOL_SHAPE_TRIANGLE`, útil como primitiva directa para pruebas exactas, raycasts y fallback de contacto contra escenario triangulado.

```c
sicol_shape_t tri;
fx a[3], b[3], c[3];

fx_set3(a, 0, 0, 0);
fx_set3(b, 0, FX_FROM_INT(2), 0);
fx_set3(c, 0, 0, FX_FROM_INT(2));
sicol_shape_make_triangle(&tri, a, b, c);
```

### Static mesh con BVH
La ruta nueva usa `sicol_mesh_t` con triángulos locales y un BVH fijo para acelerar raycasts, overlap queries y shape-casts contra escenario estático.

```c
sicol_mesh_t mesh;
sicol_triangle_verts_t tris[2];
sicol_shape_t mesh_shape;
fx pos[3];

/* llenar tris[...] */
sicol_mesh_build(&mesh, tris, 2);
fx_zero3(pos);
sicol_shape_make_mesh(&mesh_shape, pos, &mesh);
```

### Qué pega ahora
- ray vs triangle
- ray vs mesh
- sphere vs triangle
- shape-cast de escena contra `SICOL_SHAPE_MESH`
- overlap world contra `SICOL_SHAPE_MESH` usando BVH + subtests por triángulo

## Novedades del siguiente mordisco

### Triangle vs convex más serio
Ahora `triangle vs convex/support-mapped` entra por una ruta explícita de narrowphase exacta antes de caer al fallback genérico. Esto deja mejor marcados los contactos cuando el triángulo es la cara dura del escenario.

### Shape-cast refinado contra triángulos y mesh
La ruta de cast contra `SICOL_SHAPE_MESH` ya no se queda solo con la bisección genérica:

- usa candidatos por triángulo del BVH
- intenta una ruta refinada por triángulo
- valida mejor el impacto para cápsulas y spheres contra caras trianguladas
- conserva fallback conservador cuando el caso no entra en la ruta fina

```c
sicol_shape_cast_result_t hit;
fx delta[3];

fx_set3(delta, FX_FROM_INT(10), 0, 0);
sicol_shape_cast_pair(&capsule, delta, &mesh_shape, 0, &hit);
```

### Refit de BVH para mesh ligera dinámica
Puedes mover vértices/triángulos y luego refrescar bounds del BVH sin reconstruir toda la topología.

```c
sicol_triangle_verts_t tri;

sicol_mesh_get_triangle(&mesh, 0, &tri);
tri.v0[0] = FX_FROM_INT(1);
tri.v1[0] = FX_FROM_INT(1);
tri.v2[0] = FX_FROM_INT(1);
sicol_mesh_set_triangle(&mesh, 0, &tri);

/* o después de varios cambios directos */
sicol_mesh_refit(&mesh);
```

## Validación actual

La suite integrada ahora cubre también:

- spring joint dedicado
- slider alignment + límites + motor
- cone swing correction
- twist correction

Resultado actual verificado: **16/16 tests OK**.


## Latest collision evolution

This revision adds three major collision upgrades:

- exact **triangle vs capsule** overlap
- **oriented mesh** support (basis-aware raycast, BVH query and cast candidate collection)
- richer **persistent manifolds** for mesh contact pairs

The current verification baseline is **18/18 tests passing** and a clean manual strict compile with `-std=c89 -Wall -Wextra -Wpedantic`.
