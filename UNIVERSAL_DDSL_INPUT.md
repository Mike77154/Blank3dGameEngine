# Blank3D v3.12.0 — input universal para DDSL2

Blank3D integra ahora el catálogo y las capas de input proporcionadas en
`input_keys89`, `polls89`, `input_hook89`, `input_scanner89` y `scanemu89` para que los scripts DDSL2
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

Los nombres y aliases canónicos vienen de `vendor/input_keys89`, usando identidad USB HID. `polls89/key_pc` se conserva como ABI legacy y cruza al vocabulario canónico mediante un bridge explícito:

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
input_keys89       nombre / alias -> identidad HID canónica
        |
        v
     polls89         adquisición nativa Win/Linux/macOS
        |
        v
  input_hook89       snapshot/captura física normalizada HID
        |
        v
input_scanner89      hold / pressed / released / repeat / tap
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

El runner selecciona en `blank3d_input_platform.*` un provider `polls89` nativo
para Windows, Linux o macOS y lo adapta a `input_hook89`. El core
`blank3d_input.*` solo ve `ihk_backend` y no posee un backend Win32.
`input_hook89` conserva además sus providers directos (SDL2, X11, evdev,
Win32 async/LL hook, Allegro y EventTap) para hosts que quieran inyectarlos.
Al cambiar de provider/SO, la sintaxis DDSL2 y la identidad de teclas no cambian.

## Restricciones

- C89 estricto.
- buffers estáticos/caller-owned.
- sin `malloc`, `realloc` ni `free` en la integración activa.
- `scanemu89` usa Q16 en lugar del `float` del paquete de referencia.
- el input se actualiza antes del tick DDSL2 de cada frame.
