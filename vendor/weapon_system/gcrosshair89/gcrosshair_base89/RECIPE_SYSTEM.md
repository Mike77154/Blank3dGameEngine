# gcrosshair89 INI recipe system

The preset catalog is no longer compiled into `gcrosshair_base89.c`.

## Chain

```text
recipes/gcrosshair.ini
  -> recipes/lists/base192.ini
      -> recipes/presets/000_blank3d_default.ini
      -> ...
      -> recipes/presets/191_sci_fi_lock_vector.ini
```

A preset can itself be assembled from reusable INI fragments:

```text
preset.ini
  -> include part A
  -> include part B
  -> local sections override included values
```

Includes are resolved relative to the INI that contains them. Includes are
loaded first, then the local file is applied, so local keys always win. The
maximum recursive include depth is fixed (`GCB89_RECIPE_MAX_INCLUDE_DEPTH`).
No dynamic allocation is used.

## Root INI

```ini
[catalog]
format_version=1
assets=assets/assets.ini

[lists]
list_000=lists/base192.ini
list_001=lists/my_mods.ini
```

## List INI

```ini
[presets]
preset_000=../presets/000_blank3d_default.ini
preset_001=../presets/001_precision_pixel.ini
```

Preset IDs in a loaded root must be contiguous from zero. The stock root has
IDs 0..191. Additional roots may append IDs up to the fixed registry capacity.

## Preset INI

Supported sections:

- `[include]`: `include_000=relative/path.ini`
- `[preset]`: `id`, `name`, `category`
- `[style]`: state enable flags, color-change flag, spread multiplier, center offsets
- `[animation]`: micro/neutral/max scale and speed
- `[normal]`, `[aim]`, `[fire]`, `[hit]`: complete `GC89_Variant` fields

State sections support `inherit=normal`, `inherit=aim`, etc. This makes a state
recipe an override instead of a full duplicate.

Human-readable enum values are accepted:

```ini
draw_mode=vector        ; vector | image | hybrid
shape=circle            ; cross | circle | square | diamond | chevrons |
                        ; hexagon | brackets | open_triangle
spread_mode=shape_size  ; none | gap | shape_size | both
```

Pixel keys such as `gap_px=9` are converted to Q16.16 without floating point.
Raw fixed-point forms such as `gap_fx=589824` are also accepted. Percentage
keys (`spread_multiplier_percent`, animation `*_percent`) use integer math.
Colors are `0xRRGGBBAA`.

## Runtime loading

Existing catalog calls lazy-load `recipes/gcrosshair.ini` (then
`gcrosshair.ini` as a fallback). An application with a different working
directory should load explicitly:

```c
if (!gcb89_recipe_load_root("data/hud/gcrosshair.ini")) {
    puts(gcb89_recipe_last_error());
}
```

`gcb89_recipe_reset()` drops the static registry so another root can be loaded.

## Adding a preset without recompiling

See `recipes/examples/gcrosshair_modded.ini`. It loads the stock 192 presets
and appends ID 192 from a recipe assembled from reusable parts.

```sh
./example_recipe_root recipes/examples/gcrosshair_modded.ini
```

The engine, ABI3 structures, drawing core, and parameter helpers remain C89;
only catalog/preset data moved out to INI.

## Equivalence

The migration validator dumps every field of every stock preset from the old
compiled tables and from the new INI loader. The generated stock recipes were
accepted only after both 192-line dumps compared byte-for-byte as identical
text representations of all style and animation fields.

## Optional recipe I/O provider

The recipe tree can now be read through a host filesystem instead of direct stdio.

Bind `GCB89_RecipeIoProvider` with:

- `open_read(path)`
- `read_line(handle, buffer, capacity)`
- `close(handle)`

This is suitable for PAK/VFS/ROM/archive/embedded-resource systems. If no provider is installed, the exact same loader falls back to normal `fopen/fgets/fclose`, so desktop tools remain plug-and-play.

Provider configuration survives `gcb89_recipe_reset()` and can be cleared explicitly with `gcb89_recipe_clear_io_provider()`.
