# Blank3D v3.13.1 — cadencia separada de velocidad de proyectil

La cadencia y la velocidad física de una bala son propiedades distintas del
perfil. El runtime mantiene dos rutas independientes:

```text
trigger + fire_rate/cooldown -> reloj de disparo del actor
projectile_speed             -> velocidad inicial del proyectil
```

Cambiar `projectile_speed` no modifica `cooldown_ms`, y cambiar la cadencia no
altera el vector de velocidad de las balas ya emitidas.

## Claves recomendadas

```ini
[weapon]
fire_mode=auto
fire_rate_bps=13.333
projectile_speed=46.0
```

Cadencia puede expresarse de tres formas equivalentes:

```ini
fire_rate_bps=13.333     ; disparos por segundo
fire_rate_rpm=800        ; disparos por minuto
cooldown_ms=75           ; intervalo exacto entre disparos
```

También se aceptan `shots_per_second`, `rounds_per_second`,
`rounds_per_minute`, `shot_interval_ms` y `fire_interval_ms`.

`speed` continúa como alias legado de `projectile_speed`, pero los perfiles
incluidos ya usan el nombre explícito para evitar confundirlo con cadencia.
`action_speed`, dentro de `[audio]`, solo modifica la velocidad del mecanismo
sintetizado y nunca el reloj de disparo.

## Limpieza del host

Se retiró el comando RPYL legado:

```text
weapon bullet_speed 28 cooldown 0.16
```

Esos valores globales no gobernaban el catálogo INI y creaban una segunda
fuente aparente de configuración. Cada arma queda ahora gobernada únicamente
por su perfil activo.

## Regresión

La prueba crea dos armas automáticas a 20 disparos por segundo: una con
`projectile_speed=5` y otra con `projectile_speed=200`. Ambas resuelven un
intervalo de 50 ms. El test del runtime cambia la velocidad de la ametralladora
durante su cooldown y comprueba que el siguiente disparo continúa ocurriendo a
los 75 ms configurados.
