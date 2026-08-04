# INTEGRACION_BLANK3D

## Elegir preset una sola vez

```c
GC89_Core crosshair_core;
GC89_Style crosshair_style;
GCB89_AnimationPreset crosshair_anim;
int crosshair_preset;

crosshair_preset = GCB89_PRESET_BLANK3D_DEFAULT;

gc89_core_init(&crosshair_core);
gcb89_make_preset(crosshair_preset, &crosshair_style);
gcb89_make_preset_animation(crosshair_preset, &crosshair_anim);
gc89_core_set_ranges(&crosshair_core,
                     crosshair_anim.micro_scale_fx,
                     crosshair_anim.neutral_scale_fx,
                     crosshair_anim.maxi_scale_fx,
                     crosshair_anim.speed_fx_per_tick);
```

Para cambiarlo desde un menu:

```c
if (gcb89_preset_is_valid(selected_id)) {
    crosshair_preset = selected_id;
    gcb89_make_preset(crosshair_preset, &crosshair_style);
    gcb89_make_preset_animation(crosshair_preset, &crosshair_anim);
    gc89_core_set_ranges(&crosshair_core,
                         crosshair_anim.micro_scale_fx,
                         crosshair_anim.neutral_scale_fx,
                         crosshair_anim.maxi_scale_fx,
                         crosshair_anim.speed_fx_per_tick);
}
```

## ABI 2 y carga de assets

El pack actualizado usa `GC89_TYPES_ABI_VERSION 3`. Recompila las tres bibliotecas y
el engine con el mismo `gcrosshair89_types.h`. Las figuras vectoriales nuevas llegan en
los campos `shape_*` de `GC89_DrawSpec`; consulta `SHAPE_RENDER_CONTRACT.md`.

Los presets que todavía necesitan textura usan el `image_id` de `GC89_Variant`.

```c
int i;
int asset_id;
const char *asset_path;

for (i = 0; i < gcb89_preset_count(); ++i) {
    if (gcb89_preset_requires_image(i)) {
        asset_id = gcb89_preset_asset_id(i);
        asset_path = gcb89_asset_filename(asset_id);

        /* Tu loader registra asset_path bajo asset_id. */
        blank3d_register_crosshair_texture(asset_id, asset_path);
    }
}
```

Varios presets reutilizan el mismo asset con tamanos, colores o brazos vectoriales distintos.
El loader debe ignorar IDs ya cargadas.

## Cada frame

```c
GC89_InputState input;
GC89_DrawSpec spec;

gcb89_make_blank3d_input(&input,
                         e->mouse_right,
                         fired_this_frame,
                         hit_this_frame,
                         camera_recoil_x1000);
gcp89_resolve(&crosshair_style, &input, &spec);
gc89_core_update(&crosshair_core, frame_ticks);
gc89_core_draw(&crosshair_core,
               e->width, e->height,
               &spec, &callbacks, e, 0);
```

## Eventos de escala

```c
/* Contraccion breve. */
gc89_core_trigger_micro(&crosshair_core, GC89_ANIM_RETURN);

/* Expansion breve al disparar, recibir bloom o confirmar hit. */
gc89_core_trigger_maxi(&crosshair_core, GC89_ANIM_RETURN);
```

El adapter OpenGL debe vivir en el engine y traducir los callbacks a líneas, arcos,
polígonos abiertos, textura o al presentador correspondiente. La biblioteca base sigue sin
renderer, loader ni asignacion dinamica.
