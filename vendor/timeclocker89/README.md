# timeclocker

`timeclocker` es una libreria C89 para timers de gameplay: ticks globales, frames globales, helpers de segundos/minutos, timers nombrados, cooldowns, pulsos `every_*`, condiciones, acciones, one-shot, loop, stopwatch, delays y alarmas.

## Reglas de diseno

- C89 / C90 friendly.
- Sin `malloc`, `free`, `realloc`.
- Sin `stdio.h`.
- Sin `float` ni `double`.
- Sin `stdint.h`, `stdbool.h`, comentarios `//`, `inline`, ni sintaxis C99.
- Estado completo en `TimeClocker`; no heap interno.
- Nombres fijos de 31 caracteres utiles + terminador.
- Maximo default: `TIME_CLOCKER_MAX_TIMERS = 128`.

Puedes cambiar el limite antes de incluir el header:

```c
#define TIME_CLOCKER_MAX_TIMERS 256
#include "timeclocker.h"
```

## Archivos

```txt
timeclocker/
|-- include/timeclocker.h
|-- src/timeclocker.c
|-- examples/example_game_loop.c
|-- tests/test_timeclocker.c
|-- Makefile
`-- README.md
```

## Compilar

```sh
make test
make example
```

Comando manual:

```sh
gcc -std=c89 -pedantic -Wall -Wextra -Werror -Iinclude src/timeclocker.c tests/test_timeclocker.c -o test_timeclocker
./test_timeclocker
```

El test no imprime nada; devuelve `0` si todo paso.

## Loop basico

Usa `timeclocker_update(&clock, delta_ticks, delta_frames)` una vez por frame del juego. Si tu engine va a 60 ticks por segundo y 60 FPS:

```c
TimeClocker tc;
timeclocker_init(&tc, 60, 60);

while (running) {
    timeclocker_update(&tc, 1, 1);
}
```

Si tienes un `rt_time`, lo ideal es alimentar asi:

```c
delta_ticks = rt_time_get_delta_ticks();
timeclocker_update(&tc, delta_ticks, 1);
```

## Global ticks y frames

```c
timeclocker_global_ticks(&tc);
timeclocker_global_frames(&tc);
timeclocker_global_seconds(&tc);
timeclocker_global_minutes(&tc);
```

## Helpers segundos/minutos/fixed point

```c
timeclocker_seconds_to_ticks(&tc, 2);   /* 2 segundos a ticks */
timeclocker_minutes_to_ticks(&tc, 1);   /* 1 minuto a ticks */
timeclocker_frames_to_ticks(&tc, 30);   /* frames a ticks */
timeclocker_ticks_to_frames(&tc, 120);  /* ticks a frames */
```

Tambien hay helpers Q16.16:

```c
tc_fixed half;
half = timeclocker_fixed_fraction(1, 2);          /* 0.5 en Q16.16 */
timeclocker_seconds_fixed_to_ticks(&tc, half);    /* medio segundo */
```

## Named timers

### One-shot

```c
timeclocker_oneshot_seconds(&tc, "door_delay", 2);

if (timeclocker_timer_done(&tc, "door_delay")) {
    open_door();
}
```

### Loop

```c
timeclocker_loop_seconds(&tc, "spawn", 5, 0); /* 0 = infinito */

if (timeclocker_timer_fired(&tc, "spawn")) {
    spawn_enemy();
}
```

Si el juego pega un salto de varios periodos en un solo update, puedes leer cuantos disparos se cruzaron:

```c
count = timeclocker_timer_fire_count(&tc, "spawn");
```

### Cooldown

```c
if (button_pressed && timeclocker_cooldown_ready(&tc, "dash")) {
    dash();
    timeclocker_cooldown_frames(&tc, "dash", 45);
}
```

### Stopwatch

```c
timeclocker_stopwatch_start(&tc, "life", TIME_CLOCKER_UNIT_TICKS);

elapsed = timeclocker_timer_elapsed_units(&tc, "life");
```

### Frame delay

```c
timeclocker_frame_delay(&tc, "blink_wait", 8);

if (timeclocker_timer_done(&tc, "blink_wait")) {
    blink_next();
}
```

### Tick delay

```c
timeclocker_tick_delay(&tc, "hitstop", 6);
```

### Alarm

Alarm guarda un payload de accion para que tu engine lo consuma sin callbacks ni heap.

```c
#define ACT_OPEN_DOOR 1

TimeClockerEvent ev;
timeclocker_alarm_seconds(&tc, "door_alarm", 2, ACT_OPEN_DOOR, "open_door", 0, 0, 0);

if (timeclocker_poll_alarm(&tc, &ev, 1)) {
    if (ev.action_id == ACT_OPEN_DOOR) {
        open_door();
    }
}
```

## every_ticks / every_frames / every_seconds

Estas funciones detectan si el update actual cruzo una frontera de periodo.

```c
if (timeclocker_every_seconds(&tc, 1)) {
    hud_clock_tick();
}

if (timeclocker_every_frames(&tc, 2)) {
    sprite_blink();
}

if (timeclocker_every_ticks(&tc, 30)) {
    ai_think();
}
```

## Bindings de condicion

La capa de condicion sirve para DSLs tipo:

```txt
if timer_done door_delay then open_door
if every_seconds 5 then spawn_enemy
if cooldown_ready dash then player_dash
```

En C:

```c
TimeClockerCondition cond;
timeclocker_condition_init(&cond, TIME_CLOCKER_COND_TIMER_DONE, "door_delay", 0, 0, 0);

if (timeclocker_condition_eval(&tc, &cond)) {
    open_door();
}
```

Condiciones incluidas:

```txt
TIME_CLOCKER_COND_ALWAYS
TIME_CLOCKER_COND_TIMER_EXISTS
TIME_CLOCKER_COND_TIMER_ACTIVE
TIME_CLOCKER_COND_TIMER_DONE
TIME_CLOCKER_COND_TIMER_FIRED
TIME_CLOCKER_COND_TIMER_READY
TIME_CLOCKER_COND_COOLDOWN_READY
TIME_CLOCKER_COND_EVERY_TICKS
TIME_CLOCKER_COND_EVERY_FRAMES
TIME_CLOCKER_COND_EVERY_SECONDS
TIME_CLOCKER_COND_EVERY_MINUTES
TIME_CLOCKER_COND_TIMER_ELAPSED_GTE
TIME_CLOCKER_COND_TIMER_REMAINING_LTE
TIME_CLOCKER_COND_ALARM_PENDING
TIME_CLOCKER_COND_GLOBAL_TICKS_GTE
TIME_CLOCKER_COND_GLOBAL_FRAMES_GTE
TIME_CLOCKER_COND_GLOBAL_SECONDS_GTE
TIME_CLOCKER_COND_GLOBAL_MINUTES_GTE
```

## Bindings de accion

La capa de accion sirve para DSLs tipo:

```txt
do timer_start door_delay seconds 2
do cooldown_set dash frames 45
do alarm_set door_alarm seconds 2 action open_door
```

En C:

```c
TimeClockerAction act;
timeclocker_action_init(&act, TIME_CLOCKER_ACT_COOLDOWN_FRAMES, "dash", 45, 0, 0);
timeclocker_action_run(&tc, &act);
```

Acciones incluidas:

```txt
TIME_CLOCKER_ACT_NONE
TIME_CLOCKER_ACT_TIMER_ONESHOT_TICKS
TIME_CLOCKER_ACT_TIMER_ONESHOT_SECONDS
TIME_CLOCKER_ACT_TIMER_ONESHOT_FRAMES
TIME_CLOCKER_ACT_TIMER_LOOP_TICKS
TIME_CLOCKER_ACT_TIMER_LOOP_SECONDS
TIME_CLOCKER_ACT_TIMER_LOOP_FRAMES
TIME_CLOCKER_ACT_COOLDOWN_TICKS
TIME_CLOCKER_ACT_COOLDOWN_SECONDS
TIME_CLOCKER_ACT_COOLDOWN_FRAMES
TIME_CLOCKER_ACT_STOPWATCH_START
TIME_CLOCKER_ACT_FRAME_DELAY
TIME_CLOCKER_ACT_TICK_DELAY
TIME_CLOCKER_ACT_ALARM_TICKS
TIME_CLOCKER_ACT_ALARM_SECONDS
TIME_CLOCKER_ACT_TIMER_STOP
TIME_CLOCKER_ACT_TIMER_PAUSE
TIME_CLOCKER_ACT_TIMER_RESUME
TIME_CLOCKER_ACT_TIMER_RESET
TIME_CLOCKER_ACT_TIMER_CLEAR
```

## Sugerencia de binding DSL

Mapeo sugerido:

```txt
Condiciones DSL                  C
-----------------------------------------------------------
timer_exists name                TIME_CLOCKER_COND_TIMER_EXISTS
timer_active name                TIME_CLOCKER_COND_TIMER_ACTIVE
timer_done name                  TIME_CLOCKER_COND_TIMER_DONE
timer_fired name                 TIME_CLOCKER_COND_TIMER_FIRED
timer_ready name                 TIME_CLOCKER_COND_TIMER_READY
cooldown_ready name              TIME_CLOCKER_COND_COOLDOWN_READY
every_ticks n                    TIME_CLOCKER_COND_EVERY_TICKS
every_frames n                   TIME_CLOCKER_COND_EVERY_FRAMES
every_seconds n                  TIME_CLOCKER_COND_EVERY_SECONDS
every_minutes n                  TIME_CLOCKER_COND_EVERY_MINUTES
alarm_pending name               TIME_CLOCKER_COND_ALARM_PENDING
```

```txt
Acciones DSL                     C
-----------------------------------------------------------
timer_start name ticks n         TIME_CLOCKER_ACT_TIMER_ONESHOT_TICKS
timer_start name seconds n       TIME_CLOCKER_ACT_TIMER_ONESHOT_SECONDS
timer_start name frames n        TIME_CLOCKER_ACT_TIMER_ONESHOT_FRAMES
timer_loop name ticks n          TIME_CLOCKER_ACT_TIMER_LOOP_TICKS
timer_loop name seconds n        TIME_CLOCKER_ACT_TIMER_LOOP_SECONDS
timer_loop name frames n         TIME_CLOCKER_ACT_TIMER_LOOP_FRAMES
cooldown_set name ticks n        TIME_CLOCKER_ACT_COOLDOWN_TICKS
cooldown_set name seconds n      TIME_CLOCKER_ACT_COOLDOWN_SECONDS
cooldown_set name frames n       TIME_CLOCKER_ACT_COOLDOWN_FRAMES
stopwatch_start name             TIME_CLOCKER_ACT_STOPWATCH_START
frame_delay name frames n        TIME_CLOCKER_ACT_FRAME_DELAY
tick_delay name ticks n          TIME_CLOCKER_ACT_TICK_DELAY
alarm_set name ticks n           TIME_CLOCKER_ACT_ALARM_TICKS
alarm_set name seconds n         TIME_CLOCKER_ACT_ALARM_SECONDS
timer_stop name                  TIME_CLOCKER_ACT_TIMER_STOP
timer_pause name                 TIME_CLOCKER_ACT_TIMER_PAUSE
timer_resume name                TIME_CLOCKER_ACT_TIMER_RESUME
timer_reset name                 TIME_CLOCKER_ACT_TIMER_RESET
timer_clear name                 TIME_CLOCKER_ACT_TIMER_CLEAR
```

## Semantica importante

- `timer_done`: estado estable; queda verdadero cuando un countdown termina.
- `timer_fired`: pulso del update actual; util para loops y eventos.
- `cooldown_ready`: verdadero si no existe timer o si ya no esta activo.
- `every_*`: verdadero si el update actual cruzo una frontera del periodo.
- `alarm_pending`: queda verdadero hasta que llames `timeclocker_poll_alarm(..., consume=1)`.

## Integracion mental

```txt
rt_time / reloj base
        |
        v
timeclocker_update(delta_ticks, delta_frames)
        |
        +--> global_ticks / global_frames
        +--> named timers
        +--> cooldowns
        +--> every_* pulses
        +--> condition bindings
        `--> action/alarm event bindings
```
