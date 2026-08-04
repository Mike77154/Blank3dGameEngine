# Changelog

## 3.0.0-outline

- Sincroniza el header compartido con `GC89_TYPES_ABI_VERSION 3`.
- Agrega campos de outline a todas las variantes de preset.
- Conserva el outline apagado por defecto en los 192 presets.
- Mantiene IDs, formas, nombres y comportamiento del paquete vector-only.


## 2.1.0-vector-only

- Convierte los 192 presets a `GC89_DRAW_VECTOR`.
- Elimina todas las referencias de preset a `image_id`; `gcb89_preset_requires_image()` devuelve 0 para todo el catálogo.
- Conserva IDs, nombres, categorías, ABI 2 y la tabla histórica de assets para compatibilidad.
- Refuerza las pruebas para exigir 192/192 presets vectoriales.

## preset-pack-192-abi2

- Conserva los 96 IDs anteriores sin renumerarlos.
- Agrega 96 presets nuevos, IDs 96–191.
- Actualiza el header compartido a `GC89_TYPES_ABI_VERSION 2`.
- Agrega soporte de preset para círculo/elipse, cuadrado, rombo, chevrons,
  hexágono, brackets y triángulo abierto.
- Agrega máscaras de segmentos/direcciones, rotación, cortes y modos de spread.
- Agrega `gcb89_preset_shape_type()` y `gcb89_preset_spread_mode()`.
- Mantiene los 42 assets TGA y los modos vector/image/hybrid.
- Actualiza catálogos CSV, Markdown y PNG a 192 presets.
- Amplía las pruebas para construir todos los presets y validar figuras ABI 2.
- Mantiene C89 estricto, fixed-point y cero asignación dinámica.

## preset-pack-96

- Conserva `blank3d_default` como preset 0 sin alterar sus valores.
- Agrega 95 presets nuevos.
- Agrega catálogo estable por ID, nombre, categoría, modo y asset.
- Agrega 42 assets TGA RGBA tintables.
- Agrega constructores y animación recomendada por preset.
- Agrega catálogos Markdown, CSV y PNG.
- Mantiene las restricciones C89/fixed-point/no-allocation.
