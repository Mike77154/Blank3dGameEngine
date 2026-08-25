# Assets tintables

Los `.tga` son RGBA sin compresión, 64x64, con figura blanca y borde negro semitransparente.

- El blanco se puede modular con `image_tint_rgba`.
- El borde negro permanece negro al modular por color y mejora la lectura.
- `gcb89_asset_filename(image_id)` entrega la ruta relativa estable.
- No hay loader ni dependencia de renderer dentro de la biblioteca.

El engine decide cómo mapear `image_id` a su textura. Una estrategia simple es cargar todos
los archivos reportados por el catálogo al iniciar y registrar cada textura con el ID indicado.
