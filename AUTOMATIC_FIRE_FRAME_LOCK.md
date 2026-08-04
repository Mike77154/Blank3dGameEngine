# Blank3D v3.25.4 — Automatic Fire Frame Lock

## Síntoma reproducido

Al mantener movimiento del jugador, rotación de cámara y fuego automático al
mismo tiempo, la ametralladora y la Gatling podían terminar enviando el arma,
el trazador o el proyectil fuera de la cruz. El defecto no dependía del perfil
de una sola arma: aparecía en la frontera temporal entre CameraNaku, el gestor
de armas y el render del HUD.

## Causa

El frame se resolvía en dos edades de cámara:

```text
1. CameraNaku actualiza la vista A.
2. Arma, muzzle y proyectil capturan A.
3. FIRE_ACCEPTED aplica recoil inmediatamente.
4. CameraNaku pasa a A + recoil dentro del mismo frame.
5. Renderer y cruz consumen A + recoil.
```

Un disparo aislado podía esconder la diferencia. En una ráfaga sostenida, con
yaw/pitch cambiando mientras el actor avanza, la diferencia se volvía visible
y parecía una inversión o elevación errática del disparo.

## Corrección

Se añadió `Blank3DFireFrameSync`, sin heap y compatible con C89. Su contrato es:

```text
Inicio del frame N
  -> aplicar recoil pendiente de N-1
  -> actualizar CameraNaku
  -> capturar vista inmutable N
  -> mover/sincronizar actor y arma
  -> resolver muzzle, target, bala y trazador con vista N
  -> dibujar mundo y HUD con vista N
  -> FIRE_ACCEPTED sólo encola recoil para N+1
```

`build_view_vectors()` devuelve la captura del `frame_stamp` actual. Por eso
ningún consumidor puede observar una cámara posterior a la usada por la bala.
El recoil sigue existiendo, pero se confirma al inicio del frame siguiente,
antes de generar su nueva captura coherente.

## Archivos principales

- `src/blank3d_fire_frame_sync.h`
- `src/blank3d_fire_frame_sync.c`
- `src/monika_blank3d.c`
- `tests/test_automatic_fire_frame_lock.c`
- `Makefile`

## Alcance

La corrección se encuentra en el flujo común, no en una tabla especial para
ametralladora o Gatling. También protege pistola, escopeta, lanzacohetes y
demás armas que consumen la vista del frame. Las trayectorias con gravedad
conservan sus solvers especializados.
