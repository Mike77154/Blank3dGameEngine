# Blank3D v3.10.0 — inventarios de armas por NPC

Cada NPC armado posee ahora un inventario privado conectado al catálogo global
`config/weapons/weapons.ini`. El NPC no depende de enums de pistola, escopeta o
ametralladora: guarda y equipa el `weapon_id` del perfil resuelto por nombre.
Por eso una arma nueva creada completamente en INI puede asignarse a un NPC sin
agregar un caso especial en `monika_blank3d.c`.

## Flujo de datos

```text
config/weapons/weapons.ini
          |
          v
perfil de arma INI registrado por nombre/ID
          |
          +-----------------------------+
          |                             |
          v                             v
config/npc_loadouts/<npc>.ini      RPYL / FPIL
propiedad + clips + munición       give/equip/cycle/reload/fire
          |                             |
          +--------------+--------------+
                         v
             Blank3DNpcInventoryBank
             32 actores, memoria estática
                         |
                         v
                    gweapon89
           cadencia / clip / recarga / eventos
```

## 1. Crear un arma completamente por INI

Crea, por ejemplo, `config/weapons/plasma_carbine.ini` con las mismas secciones
`[weapon]`, `[modules]` y `[audio]` de los perfiles existentes. Después regístrala
exclusivamente en la sección `[weapons]` del manifiesto global:

```ini
[weapons]
plasma_carbine=config/weapons/plasma_carbine.ini
```

El nombre izquierdo, `plasma_carbine`, es el nombre que usan los loadouts y los
scripts. El perfil puede utilizar un ID no consecutivo; el inventario resuelve
por búsqueda y nunca usa el ID como índice de arreglo.

## 2. Darle un inventario propio a un NPC

Ejemplo: `config/npc_loadouts/elite_gunner.ini`.

```ini
[loadout]
equipped=plasma_carbine

[weapons]
plasma_carbine=1
shotgun=1
arc_thrower=1

[ammo]
plasma_carbine=180
shotgun=28
arc_thrower=12

[clips]
plasma_carbine=30
shotgun=7
arc_thrower=2
```

- `[weapons]` define propiedad, no el arma activa.
- `[loadout] equipped` elige el arma inicial.
- `[ammo]` acepta el nombre de un arma o `ammo_<id>`.
- `[clips]` conserva un cargador independiente por arma.
- Dos NPC con el mismo archivo siguen teniendo reservas y cargadores separados.

Si no se proporciona una ruta explícita, el motor busca automáticamente:

```text
config/npc_loadouts/<archetype>.ini
```

## 3. Asignarlo al aparecer desde RPYL

```text
gunner_enemy 0 pos 0 0 18 hp 30 loadout config/npc_loadouts/elite_gunner.ini
```

También se puede forzar el arma inicial. Si la arma está registrada pero no
estaba en el loadout, `weapon` la agrega al inventario, llena su cargador y la
equipa:

```text
gunner_enemy 0 pos 0 0 18 hp 30 loadout config/npc_loadouts/elite_gunner.ini weapon arc_thrower
```

Los pares `loadout/inventory` y `weapon/equip` son alias.

## 4. Ordenar cambios desde FPIL

```text
:state=0:equipweapon=plasma_carbine,setstate=1
:state=1,plrdistwithin=8:equipweapon=shotgun,setstate=2
:state=2,plrdistfurther=11:equipweapon=plasma_carbine,setstate=1
:state=1:lookatplayer,fireplayer
:state=2:lookatplayer,fireplayer
```

Acciones disponibles:

```text
equipweapon=<nombre>   useweapon=<nombre>   selectweapon=<nombre>
giveweapon=<nombre>
nextweapon             prevweapon
reloadweapon           reload
fireplayer             shootplayer
```

Condiciones disponibles:

```text
weaponis=<nombre>      usingweapon=<nombre>
hasweapon=<nombre>
```

`equipweapon` solo funciona si el NPC posee el arma. `giveweapon` otorga una
arma registrada; no la equipa automáticamente, para que la decisión siga siendo
explícita.

## 5. Ordenar cambios directos desde RPYL

```text
npc_weapon 0 give arc_thrower
npc_weapon 0 equip arc_thrower
npc_weapon 0 next
npc_weapon 0 prev
npc_weapon 0 reload
```

`npc_inventory` es alias de `npc_weapon`. El primer argumento es el índice del
NPC en la escena.

## Estado aislado por actor

Cada inventario conserva:

```text
actor_id
arma equipada
lista de armas poseídas
clip por arma
reserva por ammo_id
```

Cambiar de `plasma_carbine` a `shotgun` y regresar restaura el cargador previo de
cada una. Otro NPC puede usar los mismos perfiles sin compartir munición ni clip.
El jugador permanece en su manager e inventario independientes.

## Límites fijos actuales

La implementación mantiene el protocolo sin heap:

- hasta 32 NPC armados simultáneos;
- hasta 16 perfiles registrados en el catálogo global;
- hasta 16 armas poseídas por cada NPC;
- hasta 16 tipos de munición;
- IDs de arma arbitrarios positivos dentro de los perfiles registrados.

Las capacidades pueden aumentarse mediante las macros de compilación sin cambiar
el formato de los INI ni los scripts.

## Validación

`tests/test_npc_weapon_inventory.c` crea dos armas ficticias enteramente desde
INI, con IDs 42 y 77, y comprueba:

- resolución por nombre y por ID no consecutivo;
- propiedad y rechazo de armas no poseídas;
- cambio de arma durante ejecución;
- cargador persistente por arma;
- munición y cargadores aislados entre dos NPC;
- integración con el provider de inventario de `gweapon89`.

Ejecutar:

```text
make test-npc-weapon-inventory
make test-languages
make test
```
