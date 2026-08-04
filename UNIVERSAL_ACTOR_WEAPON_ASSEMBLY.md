# Universal Actor Weapon Assembly — Blank3D v3.21.0

## Objetivo

Esta revisión elimina la noción de “arma del player” dentro de la capa visual y
mecánica. Un arma es ahora una **instancia de objeto equipable** que puede ser
montada en cualquier actor que publique un socket compatible: jugador,
enemigo, aliado o actor genérico.

```text
GWeapon89
  selecciona weapon_id para actor_id
        |
        v
Weapon Presentation Registry
  resuelve model/model_id + socket + mecanismo
        |
        +--------------------------+
        |                          |
        v                          v
GAttach89                    NationalMecanicanimal89
monta object_id              construye un rig por objeto
sobre actor/socket           y anima sus piezas internas
        |                          |
        +-------------+------------+
                      v
             objeto armado en mundo
                      |
                      v
        muzzle_socket invisible -> sistemas muzzle externos
```

## Autoridad de cada sistema

### GWeapon89

Es la autoridad de gameplay. Mantiene por actor:

- arma activa;
- cargador y reserva;
- recarga;
- cooldown;
- eventos `FIRE_ACCEPTED`, `RELOAD_BEGIN` y `RELOAD_END`.

No posiciona ni dibuja modelos.

### Weapon Presentation Registry

Carga la sección `[presentation]` del INI del arma y conecta el `weapon_id` con
su presentación:

```ini
[presentation]
model=machine_gun_model
model_id=2
socket=weapon_r
attachment=equipped_weapon
mechanism=slide_magazine
```

`model` y `model_id` son selectores opacos. El sistema de equipamiento no
necesita conocer si el recurso final procede de OBJ, GFO, un mesh compuesto,
un provider procedural o cualquier otro backend.

La misma sección describe el mecanismo que recibirá el animador:

```ini
detachable_magazine=1
fire_ticks=5
recoil_z=0.08
recoil_pitch=-2
action_home_z=-0.10
action_fire_z=0.18
feed_home_y=-0.3125
feed_home_z=0.15
feed_out_y=-0.85
feed_out_z=0.25
barrel_home_z=-0.62
muzzle_y=0.02
muzzle_z=-1.12
```

Todos estos valores se conservan en Q20.12 mientras pertenecen a GWeapon89 y
GAttach89. La conversión a Q16.16 ocurre una sola vez en la frontera de
NationalMecanicanimal89.

### GAttach89

GAttach recibe:

```text
actor_id
object_id
socket_name
grip_offset
model selector
```

Su trabajo es resolver el mundo del objeto. No sabe si el actor es humano,
zombi, gunner, aliado o player.

Cada actor equipado obtiene un `object_id` distinto aunque porte el mismo
modelo. Esto evita que dos actores compartan accidentalmente corredera,
recarga, visibilidad de cargador o clip mecánico.

### NationalMecanicanimal89

Cada objeto equipado posee una instancia independiente del animador. El rig
recibe la transformación mundial resuelta por GAttach y anima:

- cuerpo;
- conjunto de retroceso;
- corredera, cerrojo, bomba, cilindro o rotor;
- cargador o alimentación;
- cañón;
- `muzzle_socket` invisible.

El `resource_id` de cada `nm89_geometry_packet` conserva el `model_id` elegido
por el sistema de armas. El `mesh_id` identifica la pieza mecánica del modelo.
Así un provider gráfico puede resolver `modelo × pieza` sin modificar GAttach,
GWeapon ni el animador.

## API universal

```c
blank3d_actor_equipment_define_socket(
    &equipment,
    actor_id,
    "weapon_r",
    B3D_ATTACH89_SOCKET_WEAPON_R,
    0
);

blank3d_actor_equipment_equip(
    &equipment,
    actor_id,
    B3D_EQUIPMENT_ACTOR_ENEMY,
    weapon_id
);

blank3d_actor_equipment_sync_actor(
    &equipment,
    &weapon_manager,
    actor_id,
    B3D_EQUIPMENT_ACTOR_ENEMY,
    frame_ms
);
```

Los tipos de actor sólo son metadatos:

```text
B3D_EQUIPMENT_ACTOR_GENERIC
B3D_EQUIPMENT_ACTOR_PLAYER
B3D_EQUIPMENT_ACTOR_ENEMY
B3D_EQUIPMENT_ACTOR_ALLY
```

No existen rutas de montaje diferentes para cada uno.

## Gunner Enemy

`gunner_enemy` es la primera prueba integrada en el runner:

1. su inventario NPC selecciona el arma activa en GWeapon89;
2. el registry obtiene `machine_gun_model` y `slide_magazine`;
3. GAttach monta una instancia del objeto en `weapon_r` del gunner;
4. NationalMecanicanimal89 actualiza su rig particular;
5. `FIRE_ACCEPTED` dispara sólo el clip mecánico de ese actor;
6. el proyectil obtiene la posición del `muzzle_socket` animado;
7. cambiar su arma reemplaza modelo y rig sin código especial del gunner.

El player usa exactamente la misma API. La prueba portable añade además un
aliado para demostrar que el contrato no depende de la facción.

## Muzzle separado

El rig no contiene geometría de fogonazo. Sólo publica:

```text
muzzle_socket.position
muzzle_socket.basis
```

Las bibliotecas independientes de muzzle, humo, luz, gas, audio, casquillos y
proyectiles pueden consumir ese socket. `FIRE_ACCEPTED` se bifurca sin crear una
dependencia entre el animador y los efectos.

## Backend gráfico actual

El runner incluido conserva un mesh procedural multipartes como fallback para
las cuatro piezas visibles. La frontera ya entrega `model_id` y `model_name`,
pero este paquete no incluye archivos OBJ nuevos ni un cargador de assets de
armas adicional. Para mostrar modelos artísticos distintos, el renderer debe
resolver `packet.resource_id` y `packet.mesh_id` mediante el provider gráfico
elegido. Esa sustitución no requiere modificar la lógica de equipamiento.

## Archivos principales

```text
src/blank3d_weapon_presentation.h
src/blank3d_weapon_presentation.c
src/blank3d_actor_equipment.h
src/blank3d_actor_equipment.c
src/blank3d_mechanical_weapon.h
src/blank3d_mechanical_weapon.c
config/weapons/*.ini
tests/test_actor_equipment.c
```
