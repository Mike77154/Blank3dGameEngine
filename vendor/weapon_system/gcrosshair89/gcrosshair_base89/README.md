# gcrosshair_base89

Biblioteca C89 de presets para el sistema `gcrosshair89`.

## Qué trae

- **192 presets** con ID estable.
- Los IDs **0–95** conservan el pack anterior sin renumeración.
- Los IDs **96–191** agregan 96 presets nuevos inspirados en retículas de shooters competitivos, arena FPS, hero shooters, scopes, armas de dispersión y modos de accesibilidad.
- **192 presets enteramente vectoriales**; ninguno exige imágenes o texturas.
- **57 presets** usan figuras no-cross introducidas en ABI 2: círculo/elipse, cuadrado, rombo, chevrons, hexágono, brackets y triángulo abierto.
- Los 42 assets TGA históricos se conservan únicamente como compatibilidad de recursos; ningún preset los referencia.
- Metadatos por nombre, categoría, modo, asset, figura y modo de spread.
- Animación recomendada por preset.
- C89 estricto, Q16.16, sin `malloc`, `realloc`, `calloc`, `free`, `float` ni `double`.

Consulta `PRESET_CATALOG.md`, `PRESET_CATALOG.csv` y `PRESET_CATALOG.png`.

## ABI 3

Este paquete usa `GC89_TYPES_ABI_VERSION 3`, igual que `gcrosshair_params89` y
`gcrosshair_core89`. Conserva las figuras de ABI 2 y agrega campos de outline a
`GC89_Variant` y `GC89_DrawSpec`. Los 192 presets dejan el outline apagado por
defecto para no cambiar su apariencia; puede activarse despues de construir el
preset mediante `gcp89_variant_set_outline_px()`.

**Recompila juntos** `gcrosshair_base89`, `gcrosshair_params89`,
`gcrosshair_core89` y cualquier consumidor que incluya `gcrosshair89_types.h`.

El renderer debe implementar el contrato descrito en
`SHAPE_RENDER_CONTRACT.md`.

## Uso

```c
GC89_Style style;
GCB89_AnimationPreset animation;
int preset_id;

preset_id = GCB89_PRESET_BRACKET_DYNAMIC_LOCK;

gcb89_make_preset(preset_id, &style);
gcb89_make_preset_animation(preset_id, &animation);

/* En este pack vector-only siempre devuelve 0. */
if (gcb89_preset_requires_image(preset_id)) {
    /* Reservado para packs externos o compatibilidad futura. */
}
```

## Consultar la figura

```c
int shape_type;
int spread_mode;

shape_type = gcb89_preset_shape_type(preset_id);
spread_mode = gcb89_preset_spread_mode(preset_id);
```

## Recorrer el catálogo

```c
int i;

for (i = 0; i < gcb89_preset_count(); ++i) {
    printf("%d %s %s shape=%d spread=%d\n",
           i,
           gcb89_preset_name(i),
           gcb89_preset_category(i),
           gcb89_preset_shape_type(i),
           gcb89_preset_spread_mode(i));
}
```

## Compatibilidad de IDs

Los presets existentes mantienen sus números. Los nuevos IDs son append-only,
por lo que una configuración serializada con valores 0–95 sigue seleccionando
el mismo preset.

## Compilar

```sh
make clean
make
make test
```

## Licencia

CC0-1.0.

## INI recipe catalog (deshardcoded presets)

Preset data is now external. `recipes/gcrosshair.ini` calls list INIs, list
INIs call preset INIs, and preset INIs may include reusable INI parts. The
stock 192-preset output is field-for-field identical to the previous compiled
tables. See `RECIPE_SYSTEM.md` and `recipes/examples/gcrosshair_modded.ini`.
