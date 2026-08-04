# Blank3D v3.12.0 — input universal para DDSL2

Blank3D integra ahora el catálogo y las capas de input proporcionadas en
`input_hook`, `input_scanner`, `polls` y `scanemu` para que los scripts DDSL2
puedan declarar controles mediante nombres de tecla, sin mantener una tabla
manual dentro del engine.

## Sintaxis directa

```text
If key_hold W then move_forward
If key_hold Up then move_forward
If key_pressed Space then jump
If key_released Escape then pause
If key_repeat Down then menu_down
If key_tapped E then interact
If key_long_hold LeftShift then charge
```

La escritura es insensible a mayúsculas/minúsculas. Una acción sola después de
`then` equivale a `acción = 1`; la forma explícita sigue siendo válida. También
se conserva la forma histórica `key-w` para scripts antiguos.

## Estados disponibles

- `key_hold` / `key_down`: activo mientras el control permanece abajo.
- `key_pressed` / `key_press`: un pulso al bajar.
- `key_released` / `key_release`: un pulso al soltar.
- `key_repeat`: repetición temporizada para menús y navegación.
- `key_tapped` / `key_tap`: pulsación breve.
- `key_long_hold`: pulsación sostenida.

## Nombres de controles

Los nombres vienen de `vendor/polls89/key_pc.*`, basado en usages USB HID:

- `A` a `Z`, `0` a `9`.
- `Up`, `Down`, `Left`, `Right`.
- `Space`, `Enter`, `Escape`, `Tab`, `Backspace`.
- `Home`, `End`, `Insert`, `Delete`, `PageUp`, `PageDown`.
- `F1` a `F24`.
- `LeftShift`, `RightShift`, `LeftCtrl`, `RightCtrl`, `LeftAlt`, `RightAlt`.
- teclado numérico, locks, teclas internacionales y multimedia cuando el
  backend de la plataforma puede reportarlas.
- `MouseLeft`, `MouseRight` y aliases `mouse1`, `mouse2`, `lmb`, `rmb`.

La resolución compacta acepta variantes como `page_up`, `Page-Up` y `PageUp`.

## Flujo

```text
backend de plataforma
        |
        v
    input_hook       estado físico normalizado HID
        |
        v
  input_scanner      hold / pressed / released / repeat / tap
        |
        +------> scanemu89 Q16 para captura y rebinding
        |
        v
blank3d_input        resolución nombre -> control
        |
        v
blank3d_ddsl_input   registra solo teclas usadas por el script
        |
        v
       DDSL2          acciones de gameplay
```

El script no crea cientos de variables. Durante la recarga se detectan solo los
controles realmente mencionados y se generan referencias internas estáticas.
El límite actual es `B3D_DDSL_INPUT_MAX_BINDINGS` (128 por script).

## Bindings alternativos sin doble ejecución

```text
If key_hold W then move_forward
If key_hold Up then move_forward
```

Si W y Up están abajo simultáneamente, el host reúne ambas solicitudes y aplica
`move_forward` una sola vez durante ese frame. También se conserva
`move_foward` como alias compatible con el typo del ejemplo original.

## Backends

El runner Win32 actual activa `input_hook_backend_win32_async`. Los fuentes
vendorizados incluyen además SDL2, X11, Linux evdev, Allegro 5 y Event Tap para
macOS. Al portar el runner se cambia el backend físico; la sintaxis DDSL2 y los
nombres de controles permanecen iguales.

## Restricciones

- C89 estricto.
- buffers estáticos/caller-owned.
- sin `malloc`, `realloc` ni `free` en la integración activa.
- `scanemu89` usa Q16 en lugar del `float` del paquete de referencia.
- el input se actualiza antes del tick DDSL2 de cada frame.
