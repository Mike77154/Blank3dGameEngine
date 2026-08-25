# Migración vector-only

Este paquete conserva los 192 IDs y la ABI 2, pero todos los presets ahora se resuelven como `GC89_DRAW_VECTOR`.

## Garantías

- `gcb89_preset_requires_image(id)` devuelve 0 para todo ID válido.
- `gcb89_preset_asset_id(id)` devuelve 0 para todo ID válido.
- `GC89_Variant.image_id` queda en 0 en normal, aim, fire y hit.
- Los 42 IDs y nombres de assets históricos siguen disponibles mediante `gcb89_asset_filename()` para no romper consumidores externos que los consulten directamente.
- No cambia `GCB89_PRESET_COUNT`, ningún enum ni `GC89_TYPES_ABI_VERSION`.

Las antiguas composiciones image/hybrid se aproximan mediante su definición de figura ABI 2, sus segmentos, direcciones, rotación, cortes y brazos vectoriales existentes.
