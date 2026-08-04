# Universal Weapon Stack v3.4

## Regla de diseño

Una arma no se identifica por ser “de fuego”. Se describe como un objeto que:

1. recibe una intención del jugador;
2. transforma esa intención en energía/velocidad;
3. consume o no un recurso;
4. crea uno o más proyectiles o efectos;
5. elige física, colisión, representación, cámara y audio.

`Blank3DWeaponModules` contiene esa composición sin transferir al runner la
autoridad de munición/cadencia que ya pertenece a `gweapon89`.

## Ejes de composición

```text
trigger model
├── standard
├── spin-up
└── charge-release

physics backend
├── linear
├── gravity
└── Bolt3D

presentation
├── projectile mesh / primitive
├── casing mesh o ninguno
├── muzzle o ninguno
├── Aoi trail profile o ninguno
└── optic/sway/HUD/preset
```

## Descriptor actual

```c
typedef struct Blank3DWeaponModulesTag {
    int weapon_id;
    const char *name;
    int trigger_model;
    int physics_backend;
    int trail_profile;
    int optic_profile;
    int sway_enabled;
    int aim_query_enabled;
    int emit_muzzle;
    int emit_casing;
    int emit_trail;
    int projectile_mesh_id;
    int casing_mesh_id;
    const char *scope_preset_name;

    unsigned short charge_time_ms;
    long charge_min_speed_q16;
    long charge_max_speed_q16;
    long gravity_q16;
    long bounce_q16;
    long drag_q16;
    long mass_q16;
} Blank3DWeaponModules;
```

## Resortera

```text
trigger_model     = CHARGE_RELEASE
physics_backend   = BOLT3D
projectile mesh   = primitive stone (id 9)
casing            = none
muzzle             = none
trail              = AOI_ARC
charge             = 900 ms
speed              = 14..46 Q16.16
gravity            = -13 Q16.16
bounce             = 0.35 Q16.16
drag               = 0.02 Q16.16
mass               = 0.18 Q16.16
```

El input mantiene carga; al soltar, el adapter escribe una velocidad absoluta de
un solo disparo en el numeric provider. El manager consume la piedra y emite el
`PROJECTILE_REQUEST`; el provider Bolt3D toma desde ahí la trayectoria física.

## Extensión esperada

Para añadir una nueva familia no hace falta alterar la colisión, el inventario o
el event bus. Se agregan:

1. perfil `GWP89_WeaponProfile` para reglas de munición/cadencia;
2. receta `Blank3DWeaponModules` para providers;
3. mesh/primitiva y, opcionalmente, casing/trail/óptica/audio.

Esto permite que arco, ballesta, jabalina, disco, boomerang o railgun entren en
el mismo sistema sin fingir que todos son pistolas.
