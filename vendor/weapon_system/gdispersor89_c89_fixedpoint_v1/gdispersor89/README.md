# gdispersor89

Biblioteca independiente que toma **un disparo lógico** y lo convierte en **N solicitudes de proyectil**.

## Responsabilidad exacta

- `GDP89_MODE_SINGLE`: siempre produce un proyectil.
- `GDP89_MODE_SPREAD`: produce `projectile_count` proyectiles.
- Calcula una dirección distinta para cada proyectil usando `forward/right/up`.
- Puede mantener el daño completo por perdigón o dividir un daño total entre todos.
- Copia velocidad, vida y alcance como metadatos, pero **no mueve** proyectiles.

No administra armas, munición, colisiones, render, trails ni tiempo de vida.

## Ejemplo de escopeta

```c
GDP89_Request shot;
gdp89_request_defaults(&shot);
shot.mode = GDP89_MODE_SPREAD;
shot.projectile_count = 8;
shot.spread_fx = gdp89_spread_from_degrees(gdp89_fx_from_int(7));
shot.damage_mode = GDP89_DAMAGE_SPLIT_TOTAL;
shot.damage_fx = gdp89_fx_from_int(80);
shot.life_ms = 700UL;
shot.max_distance_fx = gdp89_fx_from_int(24);

gdp89_emit(&shot, my_spawn_pellet, my_engine);
```

## Reglas

- C89.
- Fixed point 20.12.
- Sin `malloc`, `realloc`, `free`, heap, `float`, `double`, `stdio`, `stdlib` ni `math.h` en la librería.
- Capacidad máxima configurable con `GDP89_MAX_PROJECTILES`.
