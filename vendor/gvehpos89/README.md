# gvehpos89

`gvehpos89` es una libreria C89 para **posesion de vehiculos**: jugador o NPC pueden entrar, reservar asiento, montar, pasar control al vehiculo y salir.

No es una fisica de coche. Es el pegamento jugable que conecta actores, vehiculos, asientos, input, camaras, animaciones y sistemas externos.

## Reglas

- C89 estricto.
- Sin `malloc`, `realloc`, `free` ni ownership de heap.
- Sin `float` ni `double`.
- Fixed-point 16.16 con `GVPos_FP`.
- Storage por arrays estaticos o arena lineal provista por el usuario.
- Agnostica: no depende de render, fisica, IA, camara, animacion ni ECS.

## Que resuelve

- Player o NPC puede entrar a un carro/camion.
- Asientos por slot: driver, passenger, bloqueado, player-only, npc-only.
- Reserva de asiento durante animacion de entrada.
- Timers de enter/exit sin animacion obligatoria.
- Transferencia de control al vehiculo cuando el actor ocupa el asiento driver.
- Input del actor driver se reenvia por callback al sistema real de vehiculo.
- Ocultar/desocultar actor al entrar/salir.
- Attach/detach conceptual del actor al asiento.
- Verificacion de distancia y salida despejada por callbacks.
- Cola de eventos sin heap.

## Estructura

```text
gvehpos89/
├─ include/
│  └─ gvpos.h
├─ src/
│  └─ gvpos.c
├─ demo/
│  └─ demo_gvpos.c
├─ tests/
│  └─ test_basic.c
├─ docs/
│  └─ DESIGN.md
└─ Makefile
```

## Flujo recomendado

```text
ACTOR ON FOOT
  │ interact / AI decision
  ▼
REQUEST_MOUNT
  │ checks: vehicle usable, seat free, distance, callbacks
  ▼
ENTERING
  │ timer / animation hook
  ▼
IN_VEHICLE
  │ driver input => vehicle callback
  ▼
REQUEST_EXIT
  │ checks: can_exit, exit clear
  ▼
EXITING
  │ timer / animation hook
  ▼
ACTOR ON FOOT
```

## Ejemplo minimo

```c
GVPos_Context ctx;
GVPos_Config cfg;
GVPos_Actor actors[8];
GVPos_Vehicle vehicles[4];
GVPos_Seat seats[16];
GVPos_Event events[32];
GVPos_Vec3 p;

p.x = GVPOS_FROM_INT(0);
p.y = GVPOS_FROM_INT(0);
p.z = GVPOS_FROM_INT(0);

gvpos_default_config(&cfg);
gvpos_init(&ctx, &cfg);
gvpos_bind_storage(&ctx, actors, 8, vehicles, 4, seats, 16, events, 32);

gvpos_add_actor(&ctx, 1, GVPOS_ACTOR_FLAG_PLAYER | GVPOS_ACTOR_FLAG_ALLOW_DRIVE, p);
gvpos_add_vehicle(&ctx, 100, GVPOS_VEHICLE_FLAG_USABLE, 0, p);
gvpos_add_seat(&ctx, 100, 0,
               GVPOS_SEAT_FLAG_DRIVER | GVPOS_SEAT_FLAG_ALLOW_PLAYER,
               GVPOS_FROM_INT(3), p, p);

gvpos_request_mount(&ctx, 1, 100, 0, GVPOS_REQ_CHECK_DISTANCE);
gvpos_update(&ctx);
```

## Build

```sh
make
make check
```

En MSYS2/MinGW32:

```sh
gcc -std=c89 -pedantic -Wall -Wextra -Iinclude -c src/gvpos.c -o src/gvpos.o
ar rcs libgvpos89.a src/gvpos.o
gcc -std=c89 -pedantic -Wall -Wextra -Iinclude demo/demo_gvpos.c libgvpos89.a -o demo_gvpos.exe
```

## Integracion con tu engine

Conecta callbacks:

- `can_enter`: llaves, puertas, faccion, estado del vehiculo.
- `can_exit`: no salir si esta volcado, encerrado, muerto, cinemática, etc.
- `test_exit_clear`: consulta a collision/world probe.
- `set_actor_hidden`: apagar mesh/collider/controlador humano.
- `set_actor_attached`: enganchar a asiento/hueso/socket.
- `apply_vehicle_input`: pasar throttle/brake/steer al subengine real de vehiculos.
- `get_actor_pos` y `get_vehicle_pos`: usar transform real de tu engine.

## Notas de diseno

La libreria evita calcular distancia euclidiana para no necesitar multiplicaciones grandes ni `long long`. Usa una prueba AABB de radio por eje para entrada. Si quieres una distancia mas fina, hazla en tu engine y responde desde `can_enter`.
