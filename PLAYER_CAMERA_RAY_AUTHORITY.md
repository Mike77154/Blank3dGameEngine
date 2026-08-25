# Blank3D v3.25.5 — Player Camera Ray Authority

## Decisión

Las armas de fuego convencionales controladas por el jugador ya no dependen de
la orientación persistente de un proyectil físico para decidir el impacto.
La cámara activa y la retícula son la única autoridad de selección.

El cambio es exclusivo del jugador. Los NPC conservan su ruta de arma,
apuntado, proyectiles y predicción existentes.

## Por qué se reemplazó la ruta anterior

La ruta v3.25.2–v3.25.4 intentaba mantener sincronizados:

- cámara;
- cuerpo del actor;
- socket visual del arma;
- muzzle balístico;
- zeroing;
- recoil;
- proyectil físico;
- trazador persistente.

Aunque cada fotografía del frame podía ser internamente coherente, al caminar,
rotar y mantener fuego automático coexistían proyectiles nacidos en muchos
frames y orientaciones anteriores. Un trazador físico viejo podía parecer una
bala nueva que se desviaba de la cruz. Además, la ruta del jugador seguía
reinterpretando el mismo disparo en más de una capa.

v3.25.5 deja de resolver ese problema por sincronización de ocho autoridades y
reduce el jugador a una sola consulta de intención: el rayo de pantalla.

## Ruta FPS

```text
camera.eye
    + camera.forward * range
          |
          v
  raycast de retícula
          |
          +-- primer bloqueo = impacto y daño inmediato
          |
          `-- sin bloqueo = punto máximo del rayo
```

- El origen del disparo lógico es la cámara.
- El centro del viewport determina la dirección.
- El muzzle no puede desviar el daño.
- El trazador es sólo una línea visual corta.

## Ruta TPS y OTS

TPS y OTS usan dos consultas deliberadas:

```text
RAYO A — selección
camera.eye -> centro de retícula -> aim_point

RAYO B — validación física
muzzle -> aim_point -> primer bloqueo
```

El rayo A responde «qué está mirando el jugador». El rayo B responde «puede el
cañón alcanzar ese punto sin atravesar una pared cercana». De esta manera la
cámara puede ver por encima o a un costado de una cobertura, pero el arma no
puede disparar a través de ella.

## Proyectiles falsos

Para pistola, ametralladora, Gatling, escopeta, Magnum y rifle de precisión del
jugador:

- el daño es hitscan inmediato;
- el resultado se decide una sola vez;
- se crea un trazador cosmético de 48 ms;
- el trazador no tiene velocidad, colisión ni daño;
- no se dibuja una malla de bala física;
- rotar la cámara después del disparo no puede cambiar el resultado.

Granada, cohete y munición Bolt3D continúan como proyectiles reales porque su
caída, viaje, rebote o explosión forman parte de la mecánica.

## Aislamiento de NPC

La compuerta se activa solamente cuando:

```text
actor_id == B3D_PLAYER_ACTOR_ID
AND physics_backend == B3D_PHYSICS_LINEAR
AND weapon_id != ROCKET_LAUNCHER
```

Los eventos de NPC pasan directamente a la ruta física heredada. CameraNaku
también rechaza solicitudes de cámara de cualquier actor que no sea el jugador.

## Archivos

```text
src/blank3d_player_fire_ray.h
src/blank3d_player_fire_ray.c
src/monika_blank3d.c
tests/test_player_camera_fire_ray.c
```

## Pruebas

`make test-player-camera-fire-ray` comprueba:

1. FPS usa un solo rayo nacido en la cámara.
2. TPS selecciona con cámara y respeta una pared frente al muzzle.
3. OTS converge desde el muzzle al objetivo de retícula cuando está libre.
4. Los siete rayos de escopeta permanecen en el hemisferio delantero.

Resultado esperado:

```text
PASS: player-only camera ray authority (FPS/TPS/OTS)
```
