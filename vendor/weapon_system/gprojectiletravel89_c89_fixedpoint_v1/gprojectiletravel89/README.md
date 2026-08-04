# gprojectiletravel89

Controladora independiente del **viaje y fin de vida** de cada proyectil.

Cada proyectil puede terminar por la primera condición alcanzada:

1. `life_ms`: agotó su tiempo de vida.
2. `max_distance_fx`: recorrió su alcance máximo.
3. `destination`: llegó a un punto, usando `arrival_radius_fx`.
4. muerte manual.

## Escopeta

```c
GPT89_Desc pellet;
gpt89_desc_defaults(&pellet);
pellet.speed_fx = gpt89_fx_from_int(38);
pellet.life_ms = 700UL;
pellet.max_distance_fx = gpt89_fx_from_int(24);

slot = gpt89_spawn(&travel_pool, &pellet);
```

Aunque un perdigón no choque con nada, desaparece al superar 700 ms o 24 unidades.

## Dos modos

- `GPT89_MODE_INTERNAL_MOVE`: la biblioteca integra posición con dirección y velocidad.
- `GPT89_MODE_EXTERNAL_POSITION`: tu física mueve el proyectil y llama `gpt89_observe_position()`; la biblioteca solamente vigila tiempo, distancia y destino.

## Reglas

- C89.
- Fixed point 20.12.
- Sin `malloc`, `realloc`, `free`, heap, `float`, `double`, `stdio`, `stdlib` ni `math.h` en la librería.
- Pool estático configurable con `GPT89_MAX_PROJECTILES`.
