# INTEGRACION_BLANK3D

## Recompilacion requerida

Esta version usa `GC89_TYPES_ABI_VERSION 3`. Copie el nuevo
`include/gcrosshair89_types.h` en las tres bibliotecas y recompile params,
base/presets y core. Un core ABI 1 puede seguir dibujando la cruz antigua,
pero no conoce los nuevos campos de figura.

## Una sola vez

```c
GC89_Core crosshair_core;
GC89_Style crosshair_style;
GCB89_AnimationPreset crosshair_anim;

gc89_core_init(&crosshair_core);
gcb89_make_blank3d_original_style(&crosshair_style);
gcb89_make_blank3d_animation(&crosshair_anim);
gc89_core_set_ranges(&crosshair_core,
                     crosshair_anim.micro_scale_fx,
                     crosshair_anim.neutral_scale_fx,
                     crosshair_anim.maxi_scale_fx,
                     crosshair_anim.speed_fx_per_tick);
```

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

## Seleccion de figura

```c
gcp89_variant_set_hexagon_px(&crosshair_style.normal,
                             16, 14, 2, 1, 3,
                             GC89_RGBA(255,255,255,255));
```

El core decide la rutina vectorial mediante `spec.shape_type`. Consulte
`SHAPE_RENDER_CONTRACT.md` para el significado de cada campo y mascara.

## Eventos de escala

```c
gc89_core_trigger_micro(&crosshair_core, GC89_ANIM_RETURN);
gc89_core_trigger_maxi(&crosshair_core, GC89_ANIM_RETURN);
```

El adapter OpenGL vive en el engine y traduce callbacks a lineas, vertices,
arcos, textura o al presentador correspondiente. Las bibliotecas permanecen
puras.
