# gcrosshair_params89

Biblioteca C89 de autoria y resolucion de parametros para miras vectoriales,
por imagen o hibridas. No dibuja: convierte `GC89_Style + GC89_InputState`
en un `GC89_DrawSpec` plano para `gcrosshair_core89` o cualquier renderer.

## Familias vectoriales

- cruz clasica de cuatro brazos;
- circulo o elipse;
- cuadrado;
- rombo;
- chevrons direccionales;
- hexagono;
- brackets direccionales;
- triangulo abierto o incompleto.

Todas conservan:

- variantes `NORMAL`, `AIM`, `FIRE` y `HIT`;
- prioridad `HIT > FIRE > AIM > NORMAL`;
- color fijo o por variante;
- grosor;
- punto central y tamano del punto;
- spread/bloom;
- offset respecto al centro;
- modo vectorial, imagen o hibrido;
- dimensiones y tinte de imagen;
- mascara de brazos, direcciones o segmentos;
- corte central opcional en cada trazo;
- rotacion vectorial en grados enteros almacenados como Q16.16;
- outline vectorial configurable por variante, con ancho y color propios.

`GC89_STATE_DISABLED` apaga el resultado.

## Compatibilidad

Las funciones anteriores siguen disponibles. Esta llamada continua creando la
cruz clasica y selecciona automaticamente `GC89_SHAPE_CROSS`:

```c
gcp89_variant_set_vector_px(&style.normal, 9, 13, 1, 1, 3,
                            GC89_ARM_ALL,
                            GC89_RGBA(209,235,255,255));
```

El header compartido ahora declara `GC89_TYPES_ABI_VERSION 3`. ABI 2 introdujo
las figuras vectoriales y ABI 3 agrega, al final de `GC89_Variant` y
`GC89_DrawSpec`, los campos de outline. Todo consumidor debe recompilarse con
el nuevo `gcrosshair89_types.h`.

## Ejemplos rapidos

```c
/* Circulo blanco que crece con el bloom. */
gcp89_variant_set_circle_px(&style.normal,
                            14, 2, 1, 3,
                            GC89_RGBA(255,255,255,255));

/* Brackets izquierdo y derecho al apuntar. */
gcp89_variant_set_brackets_px(&style.aim,
                              22, 16, 6, 2,
                              GC89_DIRECTION_LEFT |
                              GC89_DIRECTION_RIGHT,
                              1, 3,
                              GC89_RGBA(80,255,120,255));
style.use_aim_variant = 1;

/* Triangulo sin el lado inferior y rotado hacia abajo. */
gcp89_variant_set_open_triangle_px(&style.hit,
                                   18, 16, 3,
                                   GC89_SEGMENT_0 |
                                   GC89_SEGMENT_2,
                                   1, 4,
                                   GC89_RGBA(255,80,80,255));
gcp89_variant_set_shape_rotation_deg(&style.hit, 180);
gcp89_variant_set_shape_break_px(&style.hit, 5);
style.use_hit_variant = 1;
```

## Outline vectorial

El outline se configura por variante y queda apagado por defecto para conservar
el aspecto de estilos existentes:

```c
gcp89_variant_set_outline_px(&style.normal,
                                 1,
                                 1,
                                 GC89_RGBA(0,0,0,255));
```

`width_px` representa el borde visible a cada lado del trazo. El resolver copia
la configuracion al `GC89_DrawSpec`; el core hace primero el pase exterior y
despues el dibujo normal. El outline integrado aplica a vectores y punto central,
no a imagenes.

## Medidas

`shape_radius_x_fx` y `shape_radius_y_fx` son semiejes: distancia desde el
centro hasta el borde exterior. `shape_depth_fx` controla la profundidad de
las alas de un chevron o de los ganchos de un bracket. El grosor sigue usando
`thickness_fx`.

El renderer debe obedecer `shape_segment_mask` para arcos o lados,
`shape_direction_mask` para chevrons y brackets, y `shape_break_fx` para
abrir un corte centrado en cada trazo activo. La tabla completa esta en
`SHAPE_RENDER_CONTRACT.md`.

## Spread

Cada variante decide donde se aplica el bloom:

- `GC89_SPREAD_NONE`;
- `GC89_SPREAD_GAP`;
- `GC89_SPREAD_SHAPE_SIZE`;
- `GC89_SPREAD_BOTH`.

La cruz clasica usa `GC89_SPREAD_GAP`. Las figuras nuevas usan
`GC89_SPREAD_SHAPE_SIZE` de forma predeterminada. Puede cambiarse con
`gcp89_variant_set_spread_mode`.

## Restricciones

C89 estricto, Q16.16, sin heap, sin tipos reales y sin dependencia grafica.

## Compilar

```sh
make
make test
```
