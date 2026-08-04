# gamlib3d — Núcleo 3D fixed-point C89 endurecido

**gamlib3d** es una caja negra de transformaciones 3D para motores retro, renderers software y runtimes portables.  
Esta versión del core queda enfocada en cuatro objetivos:

- **C89 real**
- **sin `malloc`**
- **sin `math.h`**
- **fixed-point Q20.12** consistente de punta a punta

---

## Qué endurece esta versión

- `g3d_fix_mul` y `g3d_fix_div` ya no dependen de `double`
- trigonometría sin `math.h`, usando **LUT de seno** + simetrías
- `sqrt` fixed por **Newton-Raphson**
- suma/resta/negación con **saturación** para evitar overflow UB
- `mat4_mul` ahora es **alias-safe**
- `lookat()` y `camera_build_view_matrix()` quedaron **alineadas**
- `transform_move_local()` ahora usa **ejes locales reales 6DOF**
- proyecciones con **guards** para parámetros degenerados
- `gamlib3d_scalar.c` dejó de romper el build C89

---

## Convención espacial

La librería usa una convención fija y explícita:

- sistema **diestro**
- **+Y** arriba
- **-Z** hacia adelante cuando la rotación es identidad
- matrices **4x4 columna-major** estilo OpenGL clásico
- `Transform.rotation` guarda:
  - `x = pitch`
  - `y = yaw`
  - `z = roll`

Esto permite que `Transform`, `Camera`, `LookAt` y el movimiento local compartan el mismo idioma matemático.

---

## Estructura principal

```text
./
├── gamlib3d_camera.c
├── gamlib3d_camera.h
├── gamlib3d_movement.c
├── gamlib3d_movement.h
├── gamlib3d_rotation.c
├── gamlib3d_rotation.h
├── gamlib3d_scalar.c
├── gamlib3d_scalar.h
├── gamlib3d_scale.c
├── gamlib3d_scale.h
├── gamlib3d_transform.c
├── gamlib3d_transform.h
├── gamlib_3_d_readme.md
├── math_helpers/
│   ├── gamlib3d_math.c
│   ├── gamlib3d_math.h
│   ├── gamlib3d_matrix.c
│   └── gamlib3d_matrix.h
└── tests/
    └── test_gamlib3d.c
```

---

## Módulos clave

### `gamlib3d_math`
Core fixed Q20.12:

- saturación a 32 bits con signo
- multiplicación/división fixed sin flotantes internos
- `sqrt` fixed
- trigonometría vía LUT
- `Vec3` con dot/cross/normalize

### `gamlib3d_matrix`
Matrices 4x4 columna-major:

- identidad, copia, multiplicación
- traslación, escala, rotaciones X/Y/Z
- proyecciones perspectiva y ortográfica
- inversión afín
- `lookat()` coherente con la cámara

### `gamlib3d_transform`
Transformación de objetos y cámaras:

- init identidad
- rotación acumulada con wrap angular
- extracción de ejes locales ortonormalizados
- movimiento local **6DOF**
- variante `transform_move_local_flat()` para estilo FPS
- matriz de modelo 4x4

### `gamlib3d_movement`
Integración basada en ejes:

- `movement_apply_axes()`
- `movement_apply_axes_ordered()` para elegir:
  - rotate-then-translate
  - translate-then-rotate

### `gamlib3d_camera`
Construcción de:

- view
- projection
- view-projection

con fallback a identidad cuando la inversión afín o los parámetros se vuelven degenerados.

---

## API añadida / endurecida

### Ejes locales

```c
void transform_get_local_axes(const Transform* t,
                              Vec3* out_right,
                              Vec3* out_up,
                              Vec3* out_forward);
```

### Movimiento local plano opcional

```c
void transform_move_local_flat(Transform* t,
                               g3d_fix dx,
                               g3d_fix dy,
                               g3d_fix dz);
```

### Orden explícito de integración

```c
typedef enum {
    MOVEMENT_ROTATE_THEN_TRANSLATE = 0,
    MOVEMENT_TRANSLATE_THEN_ROTATE = 1
} MovementIntegrationOrder;

void movement_apply_axes_ordered(Transform* t,
                                 const MovementAxes* axes,
                                 g3d_fix dt,
                                 const MovementParams* params,
                                 MovementIntegrationOrder order);
```

---

## Build C89

Ejemplo de compilación estricta:

```bash
gcc -std=c89 -Wall -Wextra -Wpedantic \
    -I. -Imath_helpers \
    gamlib3d_camera.c \
    gamlib3d_movement.c \
    gamlib3d_rotation.c \
    gamlib3d_scalar.c \
    gamlib3d_scale.c \
    gamlib3d_transform.c \
    math_helpers/gamlib3d_math.c \
    math_helpers/gamlib3d_matrix.c \
    tests/test_gamlib3d.c \
    -o test_gamlib3d
```

---

## Test mínimo

```bash
./test_gamlib3d
```

Los tests incluidos verifican:

- precisión angular básica
- coherencia entre movimiento local y basis del transform
- equivalencia entre `lookat()` y view matrix de cámara
- seguridad alias-safe en `mat4_mul`
- fallback a identidad en proyecciones inválidas

---

## Nota de diseño

El módulo `gamlib3d_scalar.h` conserva macros opcionales de conversión a `float` como borde-API, pero el **núcleo matemático no depende de punto flotante ni de `math.h`**.

Eso deja a gamlib3d lista como base de:

- motores retro
- render software
- simuladores pequeños
- runtimes embebidos
- engines portables estilo “una sola base matemática para todo”
