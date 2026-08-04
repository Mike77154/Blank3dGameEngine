# gcrosshair_core89

Nucleo renderer-agnostic del crosshair. Su trabajo es:

- calcular el centro exacto de la pantalla;
- aplicar offset opcional;
- animar microescala, escala neutral y maxiescala en Q16.16;
- emitir figuras vectoriales, punto central y/o una imagen mediante callbacks;
- ejecutar un pase exterior opcional para outline vectorial;
- no saber nada de OpenGL, DirectX, GDI, SDL ni del engine anfitrion.

## Restricciones

C89 estricto, sin `malloc`, `realloc`, `free`, heap, `float` ni `double`. Todo estado lo entrega el usuario y todos los valores fraccionales usan fixed-point Q16.16.

## Flujo

```text
GC89_DrawSpec resuelto
        |
        v
gc89_core_update() ---- micro / neutral / maxi
        |
        v
gc89_core_draw(screen_w, screen_h)
        |
        +--> draw_line callback
        +--> draw_dot callback
        +--> draw_image callback
```

## Animacion

```c
gc89_core_trigger_micro(&core, GC89_ANIM_RETURN);
gc89_core_trigger_maxi(&core, GC89_ANIM_RETURN);
gc89_core_trigger_neutral(&core);
gc89_core_update(&core, frame_ticks);
```

`GC89_ANIM_RETURN` hace que al tocar el extremo vuelva por si solo a escala neutral.

## Integracion

El callback OpenGL puede convertir `0xRRGGBBAA` a `glColor4ub`, el callback GDI puede usar `MoveToEx/LineTo`, y uno de DirectX puede llenar su propio vertex buffer. La libreria no impone backend.

## Compilar

```sh
make
make test
```

## ABI 3: vector shapes and outline

This build uses `GC89_TYPES_ABI_VERSION 3` and `GC89_CORE_ABI_VERSION 3`.
It consumes the ABI 2 shape fields plus the ABI 3 outline fields resolved by
`gcrosshair_params89`:

- cross, circle/ellipse, square/rectangle, diamond;
- directional chevrons and brackets;
- hexagon and open triangle;
- segment and direction masks;
- fixed-point rotation and shape breaks;
- image and hybrid modes remain available;
- vector and center-dot outlines use a renderer-agnostic double pass.

All libraries sharing `GC89_DrawSpec` must be rebuilt against the included
`gcrosshair89_types.h`.

## Outline order

When `outline_enabled` is non-zero and `outline_width_fx` is positive, the core
emits the same vector geometry first with `2 * outline_width_fx` added to line
thickness and dot diameter, using `outline_color_rgba`. It then emits the normal
geometry over it. Image outlines remain the responsibility of the host renderer.
