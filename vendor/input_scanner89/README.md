# input_scanner v2

Biblioteca de input digital en C89 con semántica más estricta, contadores de duración y backends incluidos orientados a portabilidad.

## Qué cambió

- `valid_mask` y control explícito de cuántos botones existen.
- `input_scanner_update()` ahora devuelve códigos de error.
- `down_frames`, `up_frames` y `last_down_frames` para heurísticas útiles.
- helpers nuevos: `repeat`, `long_press`, `long_hold`, `tapped`.
- dispatcher configurable con flags y soporte opcional de `REPEAT`.
- backends incluidos:
  - bitmask simple
  - teclado mapeable
  - mouse con posición, delta y wheel
  - gamepad genérico con presets de muchos mandos/layouts

## Semántica importante

- `input_button_pressed()` = un frame, borde `UP -> DOWN`.
- `input_button_released()` = un frame, borde `DOWN -> UP`.
- `input_button_hold()` = alias de `down`, mantiene compatibilidad.
- `input_dispatch_events()` ya no emite `HOLD` el mismo frame del `PRESS`.

## Backends incluidos

Los backends incluidos son **agnósticos de plataforma**. No hablan directo con SDL, XInput, DirectInput o hardware propietario; en lugar de eso, reciben snapshots/estados crudos desde tu capa de plataforma y te devuelven una semántica uniforme. Eso conserva la portabilidad del core.

### Gamepads / layouts cubiertos

- Atari 2600
- Atari 7800
- Master System
- PC Engine / TurboGrafx 2 botones
- PC Engine / TurboGrafx 6 botones
- NES
- Game Boy
- SNES
- Genesis 3 botones
- Genesis 6 botones
- Neo Geo 4 botones
- Saturn 6 botones
- Dreamcast
- Nintendo 64
- GameCube
- Switch Pro / Joy-Con par
- Wii Classic
- Wii U Pro
- PlayStation digital
- DualShock
- PSP
- PS Vita
- XInput / Xbox moderno
- Xbox Elite
- Arcade 2 / 4 / 6 / 8 botones

## Uso mínimo

```c
InputMaskBackend raw;
InputScanner scanner;

input_mask_backend_init(&raw, input_mask_from_button_count(12));
input_scanner_init(&scanner, input_mask_backend_poll, &raw);
input_scanner_set_button_count(&scanner, 12);

input_mask_backend_set_bits(&raw, input_button_bit(0) | input_button_bit(3));
input_scanner_update(&scanner);

if (input_button_pressed(&scanner, 0)) {
    /* ... */
}
```

## Mouse

```c
InputMouseBackend mouse;
InputMouseSnapshot snap;
InputScanner mouse_buttons;

input_mouse_backend_init(&mouse);
input_scanner_init(&mouse_buttons, input_mouse_backend_poll, &mouse);
input_scanner_set_valid_mask(&mouse_buttons, mouse.valid_mask);

snap.x = 320;
snap.y = 200;
snap.wheel_x = 0;
snap.wheel_y = 1;
snap.buttons_down = input_button_bit(INPUT_MOUSE_LEFT);

input_mouse_backend_commit(&mouse, &snap);
input_scanner_update(&mouse_buttons);
```

## Gamepad moderno

```c
InputPadBackend pad;
InputPadSnapshot snap;
InputScanner pad_scanner;

input_pad_backend_init_xinput(&pad);
input_scanner_init(&pad_scanner, input_pad_backend_poll, &pad);
input_scanner_set_valid_mask(&pad_scanner, pad.valid_mask);

snap.buttons_down = input_button_bit(INPUT_XBOX_A);
snap.left_x = 0;
snap.left_y = 0;
snap.right_x = 0;
snap.right_y = 0;
snap.left_trigger = 0;
snap.right_trigger = 42000;
snap.hat_x = 0;
snap.hat_y = 0;

input_pad_backend_set_snapshot(&pad, &snap);
input_scanner_update(&pad_scanner);
```

## Compilación rápida

```sh
cc -std=c89 -pedantic -Wall -Wextra -Werror \
    input_scanner.c input_ev_handler.c \
    input_backend_mask.c input_backend_keyboard.c \
    input_backend_mouse.c input_backend_pad.c \
    test_compile.c -o input_scanner_smoke_test
```
