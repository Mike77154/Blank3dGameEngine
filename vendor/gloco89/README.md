# gloco89

`gloco89` es una libreria C89 de locomocion 3D para caminar, correr, sprintar, esquivar y hacer slide-lite con una fisica agnostica de camara, renderer y motor.

## Restricciones cumplidas

- C89 / ANSI C.
- Sin `malloc`, `realloc`, `free`.
- Sin ownership de heap.
- Sin `float` ni `double`.
- Fixed point Q8.8 (`1.0 = 256`).
- Arena estatica: `GLOCO_Context` contiene slots fijos de actores y perfiles.

## Fisicas incluidas por nombre tecnico

- `acceleration envelope`: acelera hacia una velocidad objetivo.
- `ground braking`: frenado constante cuando no hay input.
- `ground friction`: amortiguacion proporcional a velocidad horizontal.
- `camera-relative basis locomotion`: movimiento relativo a camara, actor o mundo.
- `strafe/backpedal speed scaling`: modificadores laterales y hacia atras.
- `sprint stamina gate`: sprint limitado por stamina.
- `evade impulse window`: impulso lateral/frontal con ventana activa y recovery.
- `slide friction window`: deslizamiento por friccion mientras dura el estado.
- `slope/floor snap bridge`: callback externo para ground, slopes, steps y paredes.
- `stride telemetry`: fase de paso y eventos pie izquierdo/derecho para animacion.
- `movement provider`: horizontal/facing, vertical/gravedad, impulses e integracion pueden delegarse al host.
- `physics provider`: movimiento/resolucion y lifecycle de cuerpos pueden delegarse a un backend externo.

## Estructura

```text
gloco89/
├─ include/
│  ├─ gloco89.h
│  ├─ gloco89_bridge.h
│  └─ gloco89_profiles.h
├─ src/
│  ├─ gloco89.c
│  ├─ gloco89_bridge.c
│  └─ gloco89_profiles.c
├─ demo/
│  ├─ demo_gloco89.c
│  └─ demo_providers.c
├─ docs/
│  ├─ INTEGRATION.md
│  ├─ PHYSICS_NOTES.md
│  ├─ PROFILE_TUNING.md
│  └─ PROVIDERS.md
└─ Makefile
```

## Compilar

```sh
make
./demo_gloco89
```

En MinGW32/MSYS2:

```sh
gcc -ansi -pedantic -Wall -Wextra -Iinclude -c src/gloco89.c -o src/gloco89.o
gcc -ansi -pedantic -Wall -Wextra -Iinclude -c src/gloco89_profiles.c -o src/gloco89_profiles.o
gcc -ansi -pedantic -Wall -Wextra -Iinclude -c src/gloco89_bridge.c -o src/gloco89_bridge.o
ar rcs libgloco89.a src/gloco89.o src/gloco89_profiles.o src/gloco89_bridge.o
gcc -ansi -pedantic -Wall -Wextra -Iinclude demo/demo_gloco89.c libgloco89.a -o demo_gloco89
```

## Uso minimo

```c
GLOCO_Context ctx;
GLOCO_Input in;
GLOCO_Vec3 pos;
GLOCO_Vec3 fwd;
GLOCO_Vec3 right;
int actor;

gloco_init(&ctx);
gloco_load_default_profile_bank(&ctx);

pos = gloco_v3(0, 0, 0);
fwd = gloco_v3(0, 0, GLOCO_FX_ONE);
right = gloco_v3(GLOCO_FX_ONE, 0, 0);
actor = gloco_actor_create(&ctx, GLOCO_PROFILE_TACTICAL, &pos, &fwd);

in = gloco_bridge_make_camera_input(0, 256, GLOCO_INPUT_RUN | GLOCO_INPUT_SPRINT, fwd, right);
gloco_update_actor(&ctx, actor, &in, 16);
```

## Integracion real

La lib no resuelve geometria del mundo por si sola. Para eso existe `GLOCO_WorldProbeFn`. Tu motor le devuelve:

- posicion corregida,
- normal del suelo,
- flags: grounded, blocked, steep.

Asi `gloco89` queda lista para conectarse a tu motor 3D, a un capsule sweep, a un collision controller propio o a un physics backend.

## Provider mode

El comportamiento original sigue siendo el fallback. Opcionalmente puedes instalar `GLOCO_MovementProvider` y/o `GLOCO_PhysicsProvider` para conectar locomocion mecanica y fisicas del host. Cada callback decide por llamada si maneja la etapa (`GLOCO_PROVIDER_HANDLED`) o si deja correr la implementacion original (`GLOCO_PROVIDER_FALLBACK`).

Consulta `docs/PROVIDERS.md` y `demo/demo_providers.c`.

## v0.2 engine-ready

La rama v0.2 conserva todo el comportamiento standalone y anade puntos de integracion opcionales para hosts grandes:

- `GLOCO_MAX_ACTORS` y `GLOCO_MAX_PROFILES` son capacidades configurables por `-D`.
- `GLOCO_StatsProvider` permite delegar stamina a un sistema numerico externo; si el provider no maneja la llamada, GLOCO conserva su stamina interna.
- `GLOCO_Input.speed_override` permite que AI/pathing solicite una velocidad concreta sin mutar perfiles compartidos.
- `HARD_STOP` emite evento en el flanco de entrada; mantener el boton no produce spam de eventos.
- MovementProvider y PhysicsProvider siguen siendo opcionales y con fallback al comportamiento original.

Prueba estricta:

```sh
make test-engine-ready
```

El test se compila con C89 estricto, `-pedantic-errors -Wall -Wextra -Werror`.
