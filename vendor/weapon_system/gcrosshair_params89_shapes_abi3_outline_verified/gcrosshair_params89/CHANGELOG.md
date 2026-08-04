# Changelog

## ABI 3 - vector outline

- Agrega `outline_enabled`, `outline_width_fx` y `outline_color_rgba`.
- Agrega `gcp89_variant_set_outline_px()`.
- Propaga el outline de `GC89_Variant` a `GC89_DrawSpec`.
- Mantiene el outline apagado por defecto para conservar estilos existentes.
- Conserva C89 estricto, Q16.16 y cero asignacion dinamica.


## ABI 2 - shapes

- Agrega circulo/elipse, cuadrado, rombo, chevrons, hexagono, brackets y
  triangulo abierto.
- Agrega mascaras de hasta ocho segmentos.
- Agrega direcciones independientes para chevrons y brackets.
- Agrega semiejes X/Y, profundidad, rotacion y corte central de trazos.
- Agrega politica de spread por gap, tamano de figura, ambos o ninguno.
- Conserva toda la API anterior y el comportamiento de la cruz clasica.
- Agrega setters de conveniencia, ejemplo y pruebas unitarias para figuras.
