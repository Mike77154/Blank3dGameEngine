# Reactiveloader

**Reactiveloader** es una librería C89 para recarga activa tipo timing bar. Es agnóstica: no sabe de engine, cámara, HUD, inventario, animación ni armas concretas. Sólo resuelve estados, ventanas de timing, resultados y callbacks.

## Restricciones cumplidas

- C89 estricto.
- Sin `malloc`, `realloc`, `free`.
- Sin heap interno.
- Sin `float` ni `double`.
- Fixed-point Q16 para porcentajes/posición de cursor.
- Arena opcional sobre buffer entregado por el caller.
- Perfiles declarativos convertibles a DSL o banco de armas.
- Callbacks para conectar HUD, FX, animación, sonido, inventario y weapon system.

## Modelo de diseño

El diseño está inspirado en la recarga activa: primer input inicia reload, segundo input durante una barra de timing decide `PERFECT`, `GOOD`, `FAIL` o `NORMAL`. El resultado puede acelerar la recarga, dar bonus temporal, o castigar con atasco.

Estados:

```text
IDLE
  -> RELOAD_BEGIN
  -> ACTIVE_SWEEP
       -> PERFECT / GOOD / FAIL / NORMAL
  -> FINISHING
  -> DONE
```

Con jam hardcore:

```text
ACTIVE_SWEEP -> FAIL -> JAMMED -> FINISHING -> DONE
```

Con escopetas o armas shell-by-shell:

```text
FINISHING -> SHELL_LOADED -> SHELL_LOADED -> interrupt/done
```

## Integración mínima

```c
#include "reactiveloader.h"

RLD_Profile profile;
RLD_Context ctx;

RLD_MakePistolProfile(&profile, 1);
RLD_ContextInit(&ctx, entity_id, weapon_id);
RLD_Begin(&ctx, &profile, ammo_in_mag, ammo_needed);

while (RLD_IsBusy(&ctx)) {
    int input_flags = 0;

    if (reload_pressed_this_frame) {
        input_flags |= RLD_INPUT_RELOAD_PRESS;
    }
    if (fire_pressed_this_frame) {
        input_flags |= RLD_INPUT_FIRE_PRESS;
    }

    RLD_Tick(&ctx, &profile, input_flags, 0);
}
```

## HUD

Pide un `RLD_HudSample` y dibuja en tu engine:

```c
RLD_HudSample hud;
RLD_GetHudSample(&ctx, &profile, &hud);
if (hud.visible) {
    /* Dibuja barra, good window, perfect window y cursor. */
}
```

## Resultados

- `NORMAL`: no intentó la ventana o auto reload seguro.
- `GOOD`: acierto aceptable, acelera la recarga.
- `PERFECT`: acierto perfecto, acelera más y activa bonus temporal.
- `FAIL`: input fuera de ventana.
- `JAM`: fallo hardcore con atasco temporal.
- `INTERRUPTED`: arma interrumpió reload, útil para shotguns.

## Compilar

MSYS2/MinGW o Linux:

```sh
make
make run
```

Manual:

```sh
gcc -std=c89 -pedantic -Wall -Wextra -Iinclude -c src/reactiveloader.c -o reactiveloader.o
ar rcs libreactiveloader.a reactiveloader.o
gcc -std=c89 -pedantic -Wall -Wextra -Iinclude demo/demo_reactiveloader.c libreactiveloader.a -o demo_reactiveloader
```

## Archivos

```text
Reactiveloader/
├─ include/reactiveloader.h
├─ src/reactiveloader.c
├─ demo/demo_reactiveloader.c
├─ profiles/reactiveloader_profiles.txt
├─ docs/README.md
├─ docs/ARCHITECTURE.md
├─ docs/INTEGRATION.md
└─ Makefile
```
