# gvehpos89 DESIGN

## Filosofia

La posesion de vehiculo no debe estar enterrada dentro de la fisica del coche. Debe ser un sistema pequeño que haga tres cosas:

1. Resolver si un actor puede ocupar un asiento.
2. Mover el control del actor al vehiculo mientras esta en driver seat.
3. Devolver al actor al mundo de forma segura al salir.

Eso permite que el mismo sistema funcione con jugador, NPC, aliados, enemigos, camiones, autos, tanques, lanchas, mechas o carritos de golf malditos.

## Estados del actor

```text
OFF          no registrado / slot muerto
ON_FOOT      actor normal en mundo
ENTERING     asiento reservado, esperando animacion/timer
IN_VEHICLE   actor montado, oculto y/o attached
EXITING      asiento liberado, esperando animacion/timer
```

## Asientos

Cada asiento pertenece a un vehiculo y tiene un `slot` numerico. El slot 0 suele ser driver, pero no es obligatorio.

Flags importantes:

```text
GVPOS_SEAT_FLAG_DRIVER
GVPOS_SEAT_FLAG_PASSENGER
GVPOS_SEAT_FLAG_LOCKED
GVPOS_SEAT_FLAG_ALLOW_PLAYER
GVPOS_SEAT_FLAG_ALLOW_NPC
GVPOS_SEAT_FLAG_NO_ANIM
```

## Politica de memoria

Hay dos rutas:

```c
/* Arrays estaticos directos */
gvpos_bind_storage(&ctx, actors, 8, vehicles, 4, seats, 16, events, 32);

/* Arena lineal */
gvpos_arena_init(&arena, memory, sizeof(memory));
gvpos_bind_arena(&ctx, &arena, 8, 4, 16, 32);
```

La arena es lineal y no libera. Si quieres resetear, reinicializa el contexto o la arena.

## Callbacks como puentes

La lib nunca toca tu motor directamente. Todo sale por callbacks.

```text
Physics / Collision  <── test_exit_clear
Entity visibility    <── set_actor_hidden
Attachment sockets   <── set_actor_attached
Vehicle engine       <── apply_vehicle_input
Rules / locks        <── can_enter / can_exit
Transforms           <── get_actor_pos / get_vehicle_pos
```

## Multiplayer / autoridad

La libreria no implementa red. Para multiplayer, usala asi:

```text
Cliente:
  - predice highlight / prompt
  - pide entrar/salir

Servidor:
  - ejecuta gvpos_request_mount / gvpos_request_exit
  - valida distancia, asiento, bloqueo y colision
  - replica eventos ENTER_BEGIN, MOUNTED, EXIT_BEGIN, EXITED
```

## Lo que NO hace

- No simula suspension, llantas, motor ni drift.
- No decide pathfinding de NPC.
- No renderiza puertas ni animaciones.
- No cambia camaras directamente.
- No usa heap ni ownership de recursos.

## Extension sugerida

Para una version v2:

```text
- seat groups: cabina, caja, torreta, exterior
- puertas con estado: open/closed/jammed/broken
- entrada por lado: left/right/back/top
- takeover hostile: robar asiento / sacar conductor
- despawn protection: no liberar actor si salida esta bloqueada
- pose tags: driver_car, passenger_truck, turret_stand
```
