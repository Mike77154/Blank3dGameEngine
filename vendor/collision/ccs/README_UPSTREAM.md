# CoolCollege Collision System (CCS) — C89 / PS1-friendly ✅

Esta versión está pensada para ser **C89/C90 estricta**, sin `malloc`, sin `stdio`, sin `math.h` y sin depender de `stdint.h`.

✅ **Objetivo:** una librería modular de colisiones 3D (fixed-point Q16.16) compilable en toolchains retro (PS1/MIPS) y también en PC.

---

## Qué trae (versión “supercompleta”)

### Núcleo

- **Fixed-point Q16.16** (`ccs_fixed`) + utilidades (mul/div/sqrt/abs/clamp)
- **Math 3D** (`ccs_vec3`) + dot/cross/normalize/lerp/etc.

### Shapes

- `SPHERE`
- `BOX (AABB)`
- `CAPSULE` (segmento + radio)
- `OBB` (oriented box con ejes ortonormales)

### Narrow phase (colisiones “clásicas”)

- Sphere–Sphere
- Sphere–Box (AABB)
- Box–Box (AABB)
- Capsule–Sphere
- Capsule–Capsule
- Capsule–OBB
- Sphere–OBB
- OBB–OBB (SAT completo)
- Box(AABB) vs OBB (vía OBB identidad)
- Capsule vs Box(AABB) (vía OBB identidad)

### SAT OBB vs OBB (completo)

- **15 ejes**:
  - 3 normales de cara de A
  - 3 normales de cara de B
  - **9 ejes cross** `Ai x Bj`
- Selección de normal por **mínima penetración** entre los 15 ejes.
- **Manifold estable** (hasta 4 puntos) para casos face–face
- Caso edge–edge (cross-axis): **1 contacto** vía closest-segments

### Manifold + Warm start

- `ccs_manifold` con `feature_id` por punto
- Impulsos cacheables por punto:
  - `normal_impulse`
  - `tangent_impulse1`
  - `tangent_impulse2`
- Helper: `ccs_manifold_warm_start(new, old)`

> Nota: CCS no impone un solver físico completo; pero deja el manifold listo para que tu solver use warm-start.

### Raycast

- Ray vs Sphere
- Ray vs AABB
- Ray vs OBB
- Ray vs Capsule
- Raycast contra `ccs_shape*` (dispatcher)

### Mundo + broadphase

- **World** (lista estática de cuerpos) con broadphase seleccionable:
  - Sweep & Prune (generalista)
  - Grid3D (rápido/PS1-friendly) con overflow-safe

### Extras

- `ccs_trace_box` (swept box vs planes)
- `ccs_aabbtree` (BVH AABB)

---

## Compilación

### Makefile (rápido)

```bash
make
make ccs_selftest
make ccs_api_test

# Core-only (solo Sphere/Box/Capsule/OBB)
make ENABLE_PLANE=0 ENABLE_TRIMESH=0 ENABLE_HEIGHTFIELD=0 ENABLE_CONVEX=0 ENABLE_COMPOUND=0
```

### CMake

```bash
cmake -S . -B build \
  -DCCS_WERROR=ON \
  -DCCS_BUILD_SELFTEST=ON \
  -DCCS_BUILD_API_TEST=ON

cmake --build build

./build/ccs_selftest
./build/ccs_compile_api_test
```

### GCC/Clang (manual)

```bash
gcc -std=c89 -Wall -Wextra -pedantic -Werror -I. -c ccs_*.c
ar rcs libcollision.a ccs_*.o
```

### Smoke test (sin stdio)

```bash
gcc -std=c89 -Wall -Wextra -pedantic -Werror -I. \
  -o ccs_api_test _compile_api_test.c collision_api.c libcollision.a

./ccs_api_test
echo $?
```

### Self-test (sin stdio)

```bash
gcc -std=c89 -Wall -Wextra -pedantic -Werror -I. \
  -o ccs_selftest _ccs_selftest.c libcollision.a

./ccs_selftest
echo $?
```

### PS1 (idea general)



- Compila los `.c` como parte de tu proyecto (sin CRT extra).
- **No hay malloc**: ajusta límites en `ccs_config.h`.
- Asegúrate que tu toolchain trate `int` como 32-bit (normal en PS1).

---

## Configuración

Todo se controla por macros en `ccs_config.h` (puedes sobreescribir antes de incluir headers).

### Feature toggles (compilar módulos opcionales)

- `CCS_ENABLE_PLANE` (1/0) — `PLANE` + `HALFSPACE`
- `CCS_ENABLE_TRIMESH` (1/0) — `TRIMESH` + BVH (`ccs_aabbtree`)
- `CCS_ENABLE_HEIGHTFIELD` (1/0) — `HEIGHTFIELD`
- `CCS_ENABLE_CONVEX` (1/0) — `CONVEX`
- `CCS_ENABLE_COMPOUND` (1/0) — `COMPOUND`


- `CCS_MAX_BODIES` (default 256)
- Broadphase default del world:
  - `CCS_WORLD_DEFAULT_BROADPHASE` (`CCS_WORLDBP_SWEEP` por defecto)
- Grid3D:
  - `CCS_BG3D_CELL_SIZE` (default 64, en **world-units enteras**, no fixed)
  - `CCS_BG3D_X / Y / Z`
  - `CCS_BG3D_BIAS_X / Y / Z`
  - `CCS_BG3D_NEIGHBOR_RANGE` (default 1)
- Sweep:
  - `CCS_SWEEP_MAX_OBJECTS` (default = `CCS_MAX_BODIES`)
  - `CCS_SWEEP_MAX_PAIRS` (default 1024)

---

## Uso rápido

### 1) Crear shapes

#### Box (AABB)

```c
ccs_shape_box box;
box.header.type = CCS_SHAPE_BOX;
box.center = pos;                 /* ccs_vec3 */
box.half_extents = half;          /* ccs_vec3 */
```

#### Sphere

```c
ccs_shape_sphere s;
s.header.type = CCS_SHAPE_SPHERE;
s.center = pos;
s.radius = r;
```

#### Capsule

```c
ccs_shape_capsule cap;
cap.header.type = CCS_SHAPE_CAPSULE;
cap.center = pos;
cap.axis = axis_unit;       /* importante: normalizada */
cap.half_height = hh;
cap.radius = r;
```

#### OBB

```c
ccs_shape_obb o;
o.header.type = CCS_SHAPE_OBB;
o.center = pos;
o.half_extents = half;
o.axis[0] = ax; /* unit */
o.axis[1] = ay; /* unit */
o.axis[2] = az; /* unit */
```

### 2) Meterlos al world

```c
ccs_world_clear();

static ccs_vec3 p0;

ccs_world_set_broadphase(CCS_WORLDBP_SWEEP); /* o CCS_WORLDBP_GRID3D */
ccs_world_add(&p0, (ccs_shape*)&box);

ccs_world_step();
```

---

## Nota sobre C89 estricto

Se evitó:

- `inline`
- `for (int i=...)`
- compound literals `(type){...}`
- inicializadores de structs con expresiones no-const
- `stdint.h`

---

## Notas prácticas

- Para **OBB**, procura que `axis[0..2]` sean ortonormales (unit length y perpendiculares).
- Para **Capsule**, `axis` debe estar normalizada. Si no, CCS la normaliza “safe”.
- El **manifold estable + feature_id** está pensado para que un solver externo pueda hacer warm-start y fricción.

