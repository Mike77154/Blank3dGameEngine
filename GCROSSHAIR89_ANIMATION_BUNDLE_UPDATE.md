# GCROSSHAIR89 animation-bundle update

## Qué se hizo

- Se reemplazó `vendor/weapon_system/gcrosshair89` por la versión nueva del bundle
  `gcrosshair89_ini_provider_runtime_animation_bundle`.
- Se sincronizó `config/crosshair/` con la nueva receta base:
  - `animations/`
  - `assets/`
  - `examples/`
  - `lists/base192.ini`
  - `parts/`
  - `presets/000..191`
- Se conservaron las extensiones locales de Blank3D:
  - `config/crosshair/gcrosshair.ini`
  - `config/crosshair/lists/blank3d_compat.ini`
  - `config/crosshair/presets/192_blank3d_pistol_first_person.ini`
  - `config/crosshair/presets/193_blank3d_pistol_third_person.ini`
- Se añadió `gcrosshair_anim89` al `Makefile` (include path + source file).
- `src/blank3d_crosshair.c` ahora usa el runtime nuevo para animación declarativa y
  expone dos puentes ligeros para el engine:
  - `blank3d_crosshair_trigger_event()`
  - `blank3d_crosshair_modifier()`
- Se mantuvo el fallback de primitivas OpenGL de `blank3d_hud.c`, así que si no hay
  provider especializado el motor sigue dibujando líneas/dot/imágenes con el backend
  de Blank3D.

## Validación

Se ejecutó:

```sh
make test-crosshair-runtime
```

Resultado:

```txt
Blank3D gcrosshair89 runtime integration: OK (95 lines, 8 dots, 0 images)
```

## Efecto práctico

- Los presets base ahora cargan también sus perfiles de animación reutilizables desde
  INI (`config/crosshair/animations/*.ini`).
- El runtime del crosshair quedó más animable sin romper las dos recetas legacy
  `blank3d_pistol_first_person` y `blank3d_pistol_third_person`.
- Si algún día quieres disparar eventos manuales desde gameplay/UI, ya quedó el hook.
