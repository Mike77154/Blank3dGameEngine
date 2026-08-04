# Blank3D v3.25.2 — Motion/Gatling Zeroing Lock

## Síntoma

En tercera persona, algunos disparos podían abandonar el centro del HUD al
caminar o durante una ráfaga sostenida de la Gatling. El fallo podía verse como
una bala demasiado vertical o, en el peor caso, orientada hacia atrás.

## Causas encontradas

El zeroing matemático de `g3dweaponzeroing89` es stateless; no conservaba basura
entre tiros. El estado incorrecto aparecía antes y después del solver:

1. `begin_locator_frame()` publicaba la raíz del actor antes del movimiento del
   frame. `sync_actor_equipment()` podía leer esa raíz vieja y colocar el arma
   visual un frame detrás del jugador.
2. `player.muzzle` era usado a la vez como socket visual y origen balístico. La
   animación mecánica de retroceso de la Gatling volvía a publicar ese socket,
   por lo que el siguiente proyectil heredaba desplazamientos de la animación.
3. El administrador de armas ya calculaba la pose contra la cámara del momento
   del disparo, pero `spawn_bullet_event()` volvía a hacer zeroing usando la
   cámara viva. Una rotación, desplazamiento o recoil entre ambos pasos podía
   reinterpretar el evento con otro frame de referencia.

## Corrección

### Snapshot inmutable del disparo

`GWP89_Event` conserva ahora:

```text
camera_origin
camera_forward
camera_right
camera_up
```

Los valores se copian desde `GWP89_FireInput` al crear cada evento. El runtime
usa ese snapshot para el segundo ajuste de HUD, incluidos los proyectiles
recuperados de Gatling y los siete perdigones de escopeta. Los eventos externos
antiguos que no traigan snapshot siguen usando Cameranaku como fallback.

### Socket balístico independiente

Se añadió `player.ballistic_muzzle`, un socket local estable que comparte la
posición nominal de la boca del arma, pero no recibe la animación mecánica de
retroceso o giro del cañón. `player.muzzle` queda reservado para presentación,
flash y ensamblaje visual.

```text
player root actual
├─ player.weapon            -> arma visual / attachment
├─ player.muzzle            -> muzzle visual animado
└─ player.ballistic_muzzle  -> origen físico estable
```

### Sincronización del mismo frame

Antes de leer sockets de equipamiento se vuelven a publicar las transformaciones
finales del jugador, enemigos y aliados. Así el arma visual tampoco arrastra un
frame viejo cuando el actor camina.

## Archivos principales

- `src/monika_blank3d.c`
- `tests/test_aim_convergence.c`
- `vendor/weapon_system/gweapon89_manager_v2_super_agnostic_provider_bus/gweapon89_manager/src/gweapon89.h`
- `vendor/weapon_system/gweapon89_manager_v2_super_agnostic_provider_bus/gweapon89_manager/src/gweapon89.c`

## Contrato resultante

```text
input del frame de fuego
  -> snapshot cámara + socket balístico estable
  -> pose GWeapon89
  -> evento inmutable
  -> zeroing TPS/FPS con el mismo snapshot
  -> launch físico
```

La animación puede moverse libremente, pero ya no modifica el origen ni el eje
de un disparo que ya fue aceptado.
