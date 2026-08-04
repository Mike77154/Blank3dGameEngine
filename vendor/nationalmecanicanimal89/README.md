# NationalMecanicanimal89 v0.1.0

**NationalMecanicanimal89** es una librería agnóstica de animación mecánica rígida para C89 estricto. Está diseñada para objetos construidos por piezas: armas, puertas, máquinas, escuadras articuladas, vehículos, rompecabezas, utilería animada y personajes rígidos.

No es un renderer ni un cargador de modelos. Mantiene la mecánica, resuelve límites, anima canales y entrega matrices/visibilidad mediante providers.

## Protocolo

- C89 estricto.
- Q16.16 fixed-point.
- Sin `malloc`, `realloc`, `free` ni heap.
- Sin `float` ni `double`.
- Almacenamiento estático configurable por macros.
- Sin dependencia de OpenGL, Direct3D, SDL, OBJ, GLTF o un engine concreto.
- Licencia CC0-1.0.

## Idea central

```text
código + clips + providers
            |
            v
      pose solicitada
            |
            v
 constraints estáticos/dinámicos
            |
            v
       pose resuelta
            |
            v
 pivot + socket + jerarquía + mundo externo
            |
            v
 alignment por binding
            |
            v
 provider de salida
```

Un provider puede proponer datos, pero la resolución mecánica conserva la última palabra.

## Qué salió de las dos bases

De **NRPA3D** toma:

- Q16.16.
- Clips deterministas por ticks.
- API explícita de partes y límites.
- Providers agnósticos.

De **Mechanimer89** toma:

- Jerarquía interna real.
- Matrices mundiales.
- Pivotes.
- Sockets externos.
- Transform providers.
- Múltiples bindings por pieza.
- Selectores de objeto/grupo/submesh/mesh del engine.

NationalMecanicanimal89 reorganiza esos conceptos en bancos compartidos y un registro de providers multipropósito.

## Providers disponibles

Cada `nm89_provider` puede implementar cualquier combinación de callbacks. Se pueden registrar varios providers simultáneamente.

| Capacidad | Callback | Uso |
|---|---|---|
| Transform | `sample_transform` | Pose externa por pieza y source ID |
| Socket | `sample_socket` | Anclar una pieza a mano, hueso, hardpoint o entidad |
| Pivot | `sample_pivot` | Pivote dinámico |
| World | `sample_world` | Matriz mundial externa, reemplazada o premultiplicada |
| Visibility | `sample_visibility` | Visibilidad de pieza y/o binding |
| Constraint | `sample_constraint` | Límites dinámicos que intersectan o reemplazan los estáticos |
| Geometry resolver | `resolve_geometry` | Traducir resource/selectores a handles del engine |
| Geometry output | `apply_geometry` | Entregar cada binding resuelto al renderer/scene graph |
| Events | `emit_event` | Sonidos, marcadores y eventos de gameplay |
| Clock | `sample_delta_ticks` | Proporcionar el delta temporal |
| Logging | `log` | Diagnóstico sin `stdio` obligatorio dentro del core |
| Matrix identity | `matrix_identity` | Matemáticas del engine |
| Matrix multiply | `matrix_multiply` | Convención matricial externa |
| Matrix compose | `matrix_from_transform` | Composición TRS/pivote externa |
| Point transform | `matrix_transform_point` | Transformación de puntos externa |
| Easing | `ease` | Interpolación personalizada |

Los callbacks no usados se dejan en `NULL`.

## Providers por pieza

Cada pieza puede enlazarse independientemente a:

```text
part
├── transform provider
├── socket provider
├── pivot provider
├── world provider
├── visibility provider
└── dynamic constraint provider
```

Los providers se seleccionan por `provider_id` y cada enlace conserva su propio `source_id`.

## Geometría

Una pieza lógica puede controlar cero, una o varias geometrías:

```text
pieza lógica: slide
├── pistol.obj / group slide
├── sight.obj / whole resource
└── engine mesh 417
```

Los bindings aceptan:

- recurso completo;
- objeto;
- grupo;
- submesh;
- mesh propio del engine;
- geometría procedural.

La alineación permanente se almacena en un banco compartido de `nm89_alignment`.

## Constraints

Cada eje de movimiento, rotación y escala puede usar:

- `NM89_CONSTRAINT_LOCKED`
- `NM89_CONSTRAINT_FREE`
- `NM89_CONSTRAINT_CLAMPED`
- `NM89_CONSTRAINT_WRAPPED`
- `NM89_CONSTRAINT_STEPPED`

Los constraints estáticos viven en un banco reutilizable. Un provider puede suministrar constraints dinámicos:

```text
INTERSECT:
requested -> static constraint -> dynamic constraint -> resolved

REPLACE:
requested -> dynamic constraint -> resolved
```

`INTERSECT` es el modo recomendado para impedir que un provider sobrepase la mecánica base.

## Espacios de socket

`NM89_SOCKET_PARENT_SPACE`

```text
parent_world * socket_local * part_local
```

`NM89_SOCKET_WORLD_SPACE`

```text
socket_world * part_local
```

El modo mundial sirve para fijar el root de un arma a una mano o un objeto completo a una entidad externa.

## World provider

`NM89_WORLD_PREMULTIPLY`

```text
external_world * internally_resolved_world
```

`NM89_WORLD_REPLACE`

```text
world = external_world
```

El modo replace es útil cuando el engine ya resolvió completamente la jerarquía o la física.

## Animación

Los clips usan tracks por canal, no eventos que dupliquen transformaciones completas.

Canales:

```text
MOVE_X/Y/Z
ROTATE_X/Y/Z
SCALE_X/Y/Z
VISIBLE
```

Interpolaciones:

- step;
- linear;
- smooth;
- ease-in;
- ease-out;
- custom provider.

Ejemplo conceptual:

```text
slide / MOVE_Z
  tick 0 ->  0
  tick 2 -> -4
  tick 8 ->  0
```

## Acciones

Una acción semántica puede apuntar a un clip:

```c
nm89_action_bind(&rig, ACTION_RELOAD, reload_clip,
                 NM89_ACTION_RESTART);
nm89_trigger_action(&rig, ACTION_RELOAD);
```

Políticas:

- restart;
- ignore if playing;
- reverse;
- queue.

## Eventos discretos

Los clips pueden emitir:

- marker;
- visibility;
- sound;
- user event.

La librería no reproduce sonido ni ejecuta gameplay directamente; emite el `nm89_clip_event` a todos los providers con callback `emit_event`.

## Orden de evaluación

```text
1. manual pose
2. clip activo
3. transform provider
4. constraint estático
5. constraint dinámico
6. visibility provider
7. pivot estático/dinámico
8. socket
9. jerarquía
10. world provider
11. alignment del binding
12. geometry resolver
13. visibility por binding
14. geometry output providers
```

## Configuración de memoria

Antes de incluir el header se pueden cambiar las capacidades:

```c
#define NM89_MAX_PARTS       32
#define NM89_MAX_CONSTRAINTS 16
#define NM89_MAX_PIVOTS      16
#define NM89_MAX_ALIGNMENTS  32
#define NM89_MAX_BINDINGS    48
#define NM89_MAX_CLIPS        8
#define NM89_MAX_TRACKS      64
#define NM89_MAX_KEYS       192
#define NM89_MAX_EVENTS      32
#define NM89_MAX_ACTIONS     16
#define NM89_MAX_PROVIDERS    4
#include "nationalmecanicanimal89.h"
```

Con la configuración por defecto, medida en GCC de 64 bits durante las pruebas:

```text
sizeof(nm89_part) = 296 bytes
sizeof(nm89_rig)  = 37016 bytes
```

El tamaño exacto puede variar según ABI y alineación. En MinGW32 debe medirse nuevamente en el target real.

## Integración mínima

```c
nm89_rig rig;
nm89_provider provider;
nm89_i16 provider_id;

memset(&provider, 0, sizeof(provider));
provider.user = engine;
provider.sample_transform = engine_sample_transform;
provider.sample_socket = engine_sample_socket;
provider.apply_geometry = engine_apply_geometry;

nm89_rig_init(&rig, 1);
nm89_provider_add(&rig, &provider, &provider_id);
```

Después se crean piezas, constraints y bindings. El ciclo normal es:

```c
nm89_step(&rig, delta_ticks, 1U);
```

Esto avanza clips, evalúa poses, resuelve mundo y emite geometría.

## Compilación GCC / MSYS2

```sh
make test
make demo
```

Compilación directa:

```sh
gcc -std=c89 -pedantic -Wall -Wextra -Werror \
    -Iinclude -Isrc \
    src/nationalmecanicanimal89.c \
    tests/test_nm89.c \
    -o test_nm89
```

En una consola **MSYS2 MinGW32**, `build_mingw32.bat` genera el test y el demo con el GCC de 32 bits de esa consola.

## Archivos principales

```text
include/nationalmecanicanimal89.h
src/nationalmecanicanimal89.c
src/nm89_sin_table.h
tests/test_nm89.c
examples/pistol_provider_demo.c
```

## Estado de v0.1.0

Incluido y probado:

- C89 estricto con warnings como errores.
- Q16.16.
- Constraints locked/free/clamped/wrapped/stepped.
- Constraints dinámicos.
- Jerarquía y detección de ciclos.
- Pivotes estáticos y provistos.
- Transform, socket y world providers.
- Visibilidad por pieza y binding.
- Bindings múltiples y alignment.
- Geometry resolver y output.
- Clips, tracks, keys, easing y acciones.
- Eventos y reloj provistos.
- Math provider completo.

No incluido deliberadamente:

- loader OBJ;
- renderer;
- memoria dinámica;
- física;
- IK deformable;
- skinning por huesos.

Esos sistemas se conectan mediante providers o viven en módulos externos.
