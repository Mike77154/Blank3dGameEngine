# VPhysics Total Provider Stack — Blank3D v3.22.1

## Objetivo

Integrar `vphysics_total_solver_c89` sin convertirlo en dueño absoluto del
engine. Blank3D conserva sus sistemas de escena, CCS/SICOL, GAttach,
NationalMecanicanimal89 y Soquete3D; VPhysics recibe providers y resuelve sólo
los cuerpos registrados en su servicio.

```text
Blank3D scene / gameplay
        │
        ├── transform provider
        │   ├── readWorld(external_id)
        │   └── writeWorld(external_id)
        │
        └── collision provider
            ├── generateContacts(world)
            └── bodySweepTOI(...)
                    │
                    ▼
          vpTotalSolver + vpWorld
```

Sólo existe **un mundo VPhysics compartido**. No se crea una arena por actor.

## Archivos

```text
vendor/vphysics_total_solver_c89/   solver vendorizado + fixes de restitución/sleep
src/blank3d_vphysics.h             contrato público del engine
src/blank3d_vphysics.c             transform/collision bridge
tests/test_vphysics_provider_stack.c
```

## Memoria fija

El perfil integrado reserva dentro de `Blank3DVPhysics`:

```text
world arena: 524288 bytes
Total Solver arena: 49152 bytes
objects: 144 slots
bodies: 144
contacts: 256
joints: 32
solver iterations: 10
transform iterations: 2
```

Las arenas viven en almacenamiento estático/caller-owned. No hay
`malloc/realloc/free`. En un PE normal la mayor parte cae en `.bss`, por lo que
la RAM reservada no se traduce uno-a-uno en tamaño de archivo.

## Transform provider

El provider utiliza IDs externos estables y conserva el TRS completo:

```c
const vpTransformProvider *blank3d_vphysics_get_transform_provider(
    const Blank3DVPhysics *physics);
```

Callbacks activos:

```text
readWorld  -> lee Blank3DVPhysicsObject.world
writeWorld -> recibe la pose física final
```

API host Q20.12:

```c
blank3d_vphysics_move_q12(...);
blank3d_vphysics_rotate_y_q12(...);
blank3d_vphysics_scale_q12(...);
blank3d_vphysics_set_position_q12(...);
blank3d_vphysics_get_transform(...);
```

La frontera convierte una sola vez de Q20.12 a Q16.16. La matriz de draw se
entrega en Q16.16 al renderer OpenGL.

### Autoridad

```text
VP_TRANSFORM_AUTH_PHYSICS
    body -> scene; posición y rotación provienen del solver.

VP_TRANSFORM_AUTH_EXTERNAL
    scene -> body cinemático; move/rotate/scale vienen del host.
```

El Total Solver mantiene `scale` en el nodo aunque el cuerpo rígido sólo posea
posición y quaternion.

## Collision provider

```c
const vpCollisionProvider *blank3d_vphysics_get_collision_provider(
    const Blank3DVPhysics *physics);
```

### Escenario

Para cada cuerpo dinámico se lanza un raycast hacia abajo contra
`B3D_COLLISION_LAYER_WORLD`. El resultado CCS/SICOL se convierte en contacto
VPhysics con punto, normal, penetración, fricción y restitución.

### CCD

`bodySweepTOI` llama:

```c
blank3d_collision_sweep_bullet_mask(...,
    B3D_COLLISION_LAYER_WORLD, ...);
```

El TOI Q20.12 se convierte a Q16.16 y se devuelve al Total Solver. La prueba
portable confirma que el callback es consultado y produce hits.

### Cuerpos VPhysics entre sí

La primera integración utiliza un radio envolvente por objeto para generar
contactos dinámico-dinámico. Esto permite probar estabilidad, warm starting y
separación sin duplicar todavía el registro de shapes de CCS/SICOL. Es un proxy
conservador, no una narrowphase exacta de caja orientada.

## Demo del runner

Al iniciar se crean:

```text
70001 dynamic box
70002 dynamic sphere
70003 dynamic box
70004 dynamic sphere
70005 external kinematic beacon
```

El beacon prueba cada frame:

```text
move   -> posición sinusoidal X
rotate -> incremento Y
scale  -> escala vertical pulsante
```

Los cuatro cuerpos dinámicos prueban gravedad, contactos, writeback y CCD.

Controles:

```text
B pause/resume
P reset completo del único mundo y sus caches
O impulse a 70001
```

El reset reinicializa las arenas en lugar de reciclar manualmente manifolds o
pair-cache antiguos.

## Uso por otros sistemas

```c
Blank3DVPhysics *service;
const vpTransformProvider *transforms;
const vpCollisionProvider *collisions;

transforms = blank3d_vphysics_get_transform_provider(service);
collisions = blank3d_vphysics_get_collision_provider(service);
```

Esto deja abierta la conexión con:

```text
GAttach / Soquete3D -> nodos EXTERNAL para objetos sostenidos
NationalMecanicanimal89 -> constraints pre/post physics
objetos soltados -> autoridad PHYSICS
doors/platforms -> autoridad EXTERNAL
ragdolls -> varios bodies + joints en el mismo world
```

## Límites conscientes de esta versión

- La demostración no reemplaza todavía el controlador cinemático del player.
- Los enemigos existentes continúan bajo Motion89/FPIL; no se volvieron
  rigid bodies por sorpresa.
- La narrowphase cuerpo-cuerpo usa proxy esférico.
- No se ejecutó la ventana Win32 en el host Linux; sí se validó el source
  completo con declarations Win32/OpenGL y pruebas portables.

## Props ligeros y casquillos

`Blank3DVPhysicsObject` permite configurar por objeto fricción, restitución,
damping lineal/angular, colisión entre pares, CCD y debug draw. Los casquillos
usan cuerpos caja orientados con inercia física, contacto en el punto de soporte
y colisión contra el mundo, pero desactivan shell-shell para evitar una nube
O(n²).

La integración de shells está documentada en
`VPHYSICS_CASING_BOUNCE_FIX.md`.
