# gautomotion89

Paquete modular de automatización de movimiento 3D para C89.

No es una IA completa. Es una capa reutilizable para ejecutar movimientos
dirigidos, patrones espaciales y secuencias básicas desde una IA, un DSL,
un sistema de eventos o código de gameplay.

## Restricciones

- C89.
- Q16.16 fixed point.
- Sin `malloc`, `calloc`, `realloc` ni `free`.
- Sin heap interno.
- Sin `float` ni `double`.
- Sin estado global mutable.
- Sin dependencia de render, físicas, ECS, cámara o formato de entidad.
- Providers opcionales para entidades y sockets.
- API manual y wrappers provider para no casar el código con un ECS.
- Estado propiedad del caller: puede almacenarse en pools estáticos.

## Módulos

### `gautmove89`

Primitivas equivalentes al vocabulario de `move_towards_point`,
`movefore`, `rotatetoplr` y movimientos de separación:

- mover hacia un punto;
- mover hacia entidad o socket mediante provider;
- moverse en una dirección;
- avanzar según `forward`;
- alejarse hasta una distancia segura;
- orientar gradualmente hacia un punto.

### `gmovepattern89`

Genera posiciones objetivo procedurales:

- `ZIGZAG_TO`;
- `HELIX_TO`;
- `PINGPONG`;
- `ORBIT`;
- `SINE_TO`.

La salida es `GMoveMotion89`. El engine puede:

- copiar `target_position` directamente;
- convertir `delta` en velocidad;
- alimentar un character controller;
- usar `desired_direction` para animación;
- aplicar su propia detección de colisiones.

### `gmovesequence89`

Encadena pasos estáticos:

- `MOVE_TO`;
- `MOVE_AWAY`;
- `PATTERN`;
- `WAIT`;
- `ROTATE_TO`;
- `END`.

Los arrays de pasos son propiedad del caller y pueden ser `static const`.

## Árbol

```text
gautomotion89/
├── include/
│   ├── gmove89_types.h
│   ├── gmove89_math.h
│   ├── gautmove89.h
│   ├── gmovepattern89.h
│   └── gmovesequence89.h
├── src/
│   ├── gmove89_types.c
│   ├── gmove89_math.c
│   ├── gautmove89.c
│   ├── gmovepattern89.c
│   └── gmovesequence89.c
├── examples/
│   ├── demo_basic.c
│   ├── demo_provider.c
│   └── demo_sequence.c
├── tests/
│   └── test_all.c
├── Makefile
├── build_mingw32.bat
└── LICENSE
```

## Salidas de compilación

```text
libgmove89_core.a       fixed point, vectores, targets y providers
libgautmove89.a         movimiento dirigido
libgmovepattern89.a     patrones procedurales
libgmovesequence89.a    secuenciador
libgautomotion89.a      paquete combinado de conveniencia
```

Dependencias al enlazar por separado:

```text
gautmove89      -> gmove89_core
gmovepattern89  -> gmove89_core
gmovesequence89 -> gautmove89 + gmovepattern89 + gmove89_core
```

## Integración manual

```c
GAutMoveConfig89 config;
GMoveMotion89 motion;

gautmove89_config_default(&config);
config.speed = GMOVE89_FX_FROM_INT(6);
config.stop_distance = GMOVE89_FX_FROM_INT(1);

gautmove89_step_to_point(
    enemy_position,
    player_position,
    &config,
    delta_time_q16,
    &motion
);

/* El engine decide cómo aplicar la intención. */
enemy_position = motion.target_position;
```

## Integración por provider

El provider resuelve posiciones de entidades y sockets sin que la librería
conozca el engine:

```c
GMoveProvider89 provider;
GMoveTarget89 target;

provider.user = world;
provider.get_position = world_get_position;
provider.get_forward = world_get_forward;
provider.get_socket_position = world_get_socket_position;
provider.apply_motion = world_apply_motion;

target.type = GMOVE89_TARGET_SOCKET;
target.entity_id = player_id;
target.socket_id = head_socket;

gautmove89_step_to_target(
    &provider,
    enemy_id,
    &target,
    &config,
    delta_time_q16,
    &motion
);
```

## Semántica de patrones

`gmovepattern89_step` devuelve la posición ideal de la trayectoria para el
tick actual. Esto permite dos modos:

1. **Cinemático:** aplicar `target_position`.
2. **Con físicas:** convertir `delta / dt` en velocidad deseada y dejar que
   el engine resuelva colisiones.

Para un objeto controlado por físicas, no conviene copiar la posición sin
pasar por el bridge del motor.

## Compilar

Linux, MSYS2 o MinGW:

```sh
make
make test
make examples
```

Compilación estricta usada:

```sh
gcc -std=c89 -pedantic -Wall -Wextra
```

En Windows también se incluye `build_mingw32.bat`.

## Límites intencionales

La longitud vectorial es una aproximación entera rápida. Es adecuada para
movimiento y steering básico, pero no pretende reemplazar un módulo de
geometría exacta.

La rotación gradual mezcla y normaliza vectores `forward`; no genera
cuaterniones. El bridge del engine puede convertir `desired_forward` a su
representación de orientación.

## Licencia

CC0 1.0 Universal.
