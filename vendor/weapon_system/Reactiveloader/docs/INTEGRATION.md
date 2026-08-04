# Integración con un engine

## 1. Al presionar reload por primera vez

```c
RLD_Begin(&reload_ctx, profile, current_mag_ammo, ammo_needed);
```

## 2. Cada tick

```c
int input = 0;
if (pressed_reload) input |= RLD_INPUT_RELOAD_PRESS;
if (pressed_fire)   input |= RLD_INPUT_FIRE_PRESS;
if (pressed_cancel) input |= RLD_INPUT_CANCEL_PRESS;

RLD_Tick(&reload_ctx, profile, input, &callbacks);
```

## 3. Al terminar

Cuando llegue `RLD_EVENT_DONE`, el engine puede tomar:

```c
reload_ctx.ammo_after
reload_ctx.result
reload_ctx.bonus_ticks_left
```

Y aplicar cambios reales al arma.

## 4. HUD recomendado

Dibuja la barra cerca del centro si quieres que el jugador la use como parte del combate, o en esquina si quieres sabor clásico.

```text
[ good window        ]
        [perfect]
-----------|-----------------
          cursor
```

## 5. Modos recomendados

```text
OFF:
  No uses RLD_FLAG_ENABLED.

SAFE:
  RLD_FLAG_ENABLED | RLD_FLAG_SAFE_FAIL | RLD_FLAG_KEEP_NORMAL_IF_NO_PRESS

GEARS_LIKE:
  RLD_FLAG_ENABLED | RLD_FLAG_KEEP_NORMAL_IF_NO_PRESS

HARDCORE:
  RLD_FLAG_ENABLED | RLD_FLAG_HARDCORE_JAM

ACCESSIBLE:
  RLD_FLAG_ENABLED | RLD_FLAG_ALLOW_AUTO | RLD_FLAG_SAFE_FAIL
```

## 6. Puente a weapon system

Al terminar:

```c
weapon->ammo_in_mag = reload_ctx.ammo_after;
weapon->reload_result = reload_ctx.result;
weapon->bonus_ticks = reload_ctx.bonus_ticks_left;
```

## 7. Puente a shotgun / shell-by-shell

Para armas de cartucho individual:

```c
profile.flags |= RLD_FLAG_SHELL_BY_SHELL;
profile.flags |= RLD_FLAG_ALLOW_INTERRUPT;
profile.shell_ticks = 16;
profile.shells_per_step = 1;
```

Si el jugador dispara durante `FINISHING`, manda `RLD_INPUT_FIRE_PRESS` y la librería deja `ammo_after` con lo cargado hasta ese momento.
