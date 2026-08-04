# Arquitectura de Reactiveloader

## Núcleo

El núcleo es un solver por contexto:

```text
RLD_Profile  = reglas del arma
RLD_Context  = estado vivo por entidad/arma
RLD_Tick     = avanza un frame/tick determinista
RLD_Callbacks = puente hacia engine
```

## Fixed point

Todo porcentaje usa Q16:

```text
0%   = 0
50%  = 32768
100% = 65536
```

Usa `RLD_FromPercent(x)` para convertir valores enteros.

## Memoria

La librería no pide memoria al sistema. Opciones:

1. Stack/local:

```c
RLD_Context ctx;
RLD_Profile profile;
```

2. Static/global:

```c
static RLD_Context g_reload_ctx[64];
```

3. Arena del caller:

```c
static unsigned char bytes[4096];
RLD_Arena arena;
RLD_ProfileBank bank;
RLD_ArenaInit(&arena, bytes, sizeof(bytes));
RLD_ProfileBankInitArena(&bank, &arena, 16);
```

## Filosofía de integración

Reactiveloader no debe modificar directamente:

- inventario,
- munición global,
- animación,
- sonido,
- cámara,
- recoil,
- HUD.

Sólo informa eventos y deja que el engine decida.

## Eventos principales

```text
RLD_EVENT_RELOAD_BEGIN
RLD_EVENT_SWEEP_BEGIN
RLD_EVENT_GOOD
RLD_EVENT_PERFECT
RLD_EVENT_FAIL
RLD_EVENT_JAM
RLD_EVENT_SHELL_LOADED
RLD_EVENT_INTERRUPT
RLD_EVENT_DONE
```

## Bonus

Los bonus son sólo datos:

```text
bonus_damage_q16
bonus_accuracy_q16
bonus_stability_q16
bonus_fire_rate_q16
bonus_spread_q16
bonus_ticks_left
```

Tu weapon system decide cómo aplicar esos números.
