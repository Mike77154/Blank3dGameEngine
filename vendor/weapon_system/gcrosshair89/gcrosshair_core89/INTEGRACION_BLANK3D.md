# INTEGRACION_BLANK3D

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

## Eventos de escala

```c
/* Contraccion breve. */
gc89_core_trigger_micro(&crosshair_core, GC89_ANIM_RETURN);

/* Expansion breve al disparar, recibir bloom o confirmar hit. */
gc89_core_trigger_maxi(&crosshair_core, GC89_ANIM_RETURN);
```

El adapter OpenGL debe vivir en el engine y traducir los callbacks a `glBegin`, `glVertex2i`, textura o al presentador correspondiente. Asi las tres bibliotecas permanecen puras.
