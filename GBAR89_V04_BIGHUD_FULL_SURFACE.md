# GBar89 v0.4 + BigVaderHudder Full Surface — Blank3D v3.23.0

## Objetivo

Blank3D ya no conserva la implementación GBar89 v0.2 anterior. El directorio
`vendor/gbar89/` contiene GBar89 v0.4 y el bridge de BigVaderHudder expone la
superficie completa necesaria para construir barras lineales, verticales,
segmentadas, radiales, sprite-clip y nine-slice desde `.bighud`.

```text
telemetría del gameplay
          │
          ▼
BigVaderHudder / gameplay.bighud
          │ propiedades + bindings dinámicos
          ▼
gbar89_bighud
          │ GBar89_Meter / GBar89_Style
          ▼
GBar89 v0.4
          │ rects + lines + triangles + sprites + clip
          ▼
Blank3D HUD renderer / sprite provider opcional
```

La librería continúa siendo C89, fixed-point Q16.16, sin heap y agnóstica de
backend.

## Tipos y direcciones

```text
kind:
  linear | segmented | sprite_clip | nineslice | radial_pie | radial_ring

direction:
  left_to_right | right_to_left
  top_to_bottom | bottom_to_top
  center_horizontal | center_vertical
```

La ruta vertical usa el mismo compositor que la horizontal: frames, fondos,
outline, extrusión, gloss, middle band, pixelado y unidades vectoriales no se
pierden al usar `bottom_to_top` o `top_to_bottom`.

## Bindings dinámicos de BVH

```text
bind                  valor principal
min_bind              mínimo
max_bind              máximo
overlay_bind          overlay / shield / amenaza
overlay_min_bind      mínimo del overlay
overlay_max_bind      máximo del overlay
mid_bind              banda intermedia / ghost
segments_bind         segmentos
state_bind            estado visual
radial_phase_bind     fase radial
unit_count_bind       cantidad de glyphs vectoriales
layer_count_bind      capas de boss bar
layer_size_bind       tamaño de cada capa
```

Bindings disponibles actualmente:

```text
player.health          player.health_max       player.dead
weapon.loaded          weapon.capacity          weapon.reserve
weapon.empty           gameplay.threat          ecg.bpm
damage.flash_ms        camera.first_person      weapon.muzzle_flash
zero                    one                      entero literal
```

`radial_phase_speed` anima la fase en grados por segundo usando fixed-point. Si
existe `radial_phase_bind`, el binding tiene autoridad y la velocidad automática
no se aplica.

## Flags normales

Cada flag se acepta como booleano independiente:

```text
clamp_value             draw_bg                 draw_border
damage_lag              smooth_value            draw_markers
draw_overlay            use_visual              invert_ratio
layered                 draw_value_overlay      draw_pattern
use_mask                auto_state              state_blink
ease_value              draw_layer_pips         draw_mid_value
draw_vector_units       pixel_quantize
```

También se acepta `flags` con una máscara numérica o una lista corta separada
por `|`, coma, espacio o `+`.

## Compositor v0.3/v0.4

Cada efecto puede activarse individualmente:

```text
outer_outline           drop_shadow
extrude                 inner_shadow
gloss                   fill_grid
fill_scanlines          pixel_cells
radial_spokes           radial_rings
radial_ticks            radial_sweep_highlight
```

Aliases `fx_*` están disponibles, por ejemplo `fx_gloss true`.

También existe:

```text
fx_flags / effects
```

para una máscara numérica o una lista corta. En documentos BVH es más legible
y no depende del límite de longitud de línea usar los booleanos individuales.

Parámetros del compositor:

```text
outline_size
shadow_offset_x / shadow_offset_y / shadow_offset x,y
extrude_depth
gloss_percent
pixel_size
bg_step / background_step
bg_size / background_size
```

Colores:

```text
color_outline           color_highlight
color_shadow            color_extrude
color_gloss             color_bg_detail
```

## Frames procedurales

```text
frame / frame_kind:
  simple | double | bevel_out | bevel_in
  brackets | rail | pixel | none

frame_depth
frame_gap
frame_corner
```

Forma compacta numérica:

```text
frame_style kind,depth,gap,corner
```

## Fondos procedurales

```text
background / background_kind / bg_kind:
  solid | grid | checker | scanlines | diagonal | dither

bg_step
bg_size
color_bg_detail
```

Forma compacta numérica:

```text
background_style kind,step,size
```

## Banda intermedia independiente

```text
draw_mid_value true
mid_value 60
mid_bind weapon.reserve
mid_visual_value 60
mid_speed 90
mid_direction left_to_right
color_mid #5090C0A0
```

No sustituye el damage-lag ni el overlay. Los tres valores pueden coexistir:

```text
valor principal + lag + mid/ghost + overlay
```

## Paridad radial completa

```text
radial_start / radial_start_deg
radial_sweep / radial_sweep_deg
radial_inner / radial_inner_percent
radial_steps

radial_segments
radial_gap / radial_gap_deg
radial_cap / radial_cap_kind: butt | round | square
radial_detail_rings
radial_unit_radius / radial_unit_radius_percent
radial_marker_length / radial_marker_length_percent
radial_phase / radial_phase_deg
radial_phase_speed
```

Forma compacta numérica:

```text
radial_style segments,gap_deg,cap_kind,detail_rings,unit_radius_percent
```

El aro puede combinar simultáneamente segmentos, huecos, caps, frames,
backgrounds, middle band, markers, vector units, spokes, rings, ticks y sweep
highlight.

## Unidades vectoriales

Activación:

```text
draw_vector_units true
unit_count 15
unit_count_bind weapon.capacity
unit_padding 1
unit_scale_percent 72
unit_closed true
unit_filled true
```

Colores:

```text
color_unit_fill
color_unit_empty
color_unit_outline
```

Formas incluidas por Blank3D:

```text
diamond
bullet / cartridge
heart
shield
hex / hexagon
chevron
battery
none
```

Ejemplo:

```text
unit_shape bullet
```

También se admite una figura arbitraria propiedad del nodo BVH. Las coordenadas
normalizadas van de 0 a 1000:

```text
unit_points "500,0|1000,500|500,1000|0,500"
```

El bridge copia los puntos a almacenamiento fijo dentro de cada
`Blank3DBigHudNode`. Después de ordenar los nodos por Z vuelve a enlazar el
puntero de GBar89, evitando referencias hacia la copia temporal usada por el
sort.

## Layers, estados, patrones, máscaras y easing

Se conserva toda la superficie anterior:

```text
layer_count, layer_size
color_layer_fill0..7
color_layer_lag0..7

state
low_ratio, critical_ratio
state_timer, blink_period
color_state_fill0..9
color_state_border0..9

pattern_kind
pattern_step, pattern_size
color_pattern

mask_kind, mask_amount, mask_steps
mask_slice_count
mask_slice0..15 = y_percent,h_percent,inset_left,inset_right

ease_kind, ease_duration, ease_elapsed
ease_start_value, ease_target_value, ease_active
```

Patrones:

```text
none | vertical_stripes | horizontal_stripes | crosshatch | dots | ticks
```

Máscaras:

```text
none | slant_right | slant_left | hexagon | diamond | custom_slices
```

Easing:

```text
linear | in_quad | out_quad | in_out_quad | smoothstep | out_cubic
```

## Sprites, clip y nine-slice

Campos:

```text
sprite_bg              src_bg x,y,w,h
sprite_fill            src_fill x,y,w,h
sprite_lag             src_lag x,y,w,h
sprite_overlay         src_overlay x,y,w,h
sprite_empty           src_empty x,y,w,h
sprite_full            src_full x,y,w,h
sprite_partial         src_partial x,y,w,h

margin_left / margin_top / margin_right / margin_bottom
margins left,top,right,bottom
padding_left / padding_top / padding_right / padding_bottom
padding left,top,right,bottom
```

El host puede instalar un renderer real de atlas:

```c
Blank3DHudSpriteProvider sprites;
sprites.user = atlas;
sprites.draw = my_atlas_draw;
blank3d_hud_set_sprite_provider(&hud, &sprites);
```

Si no hay provider, Blank3D dibuja un rectángulo tintado de respaldo para que
un nodo sprite no desaparezca silenciosamente durante las pruebas.

El renderer posee un clip stack fijo de 8 niveles e intersecta clips anidados;
esto evita que un `sprite_clip` o `nineslice` rompa el scissor de otro nodo.

## Colores base

```text
color / color_fill
color_bg
color_lag
color_border
color_overlay
color_empty
color_marker
color_pattern
color_mid
color_outline
color_highlight
color_shadow
color_extrude
color_gloss
color_bg_detail
color_unit_fill
color_unit_empty
color_unit_outline
```

Formato aceptado:

```text
#RRGGBB
#RRGGBBAA
entero RGBA
```

## Capacidades fijas de Blank3D

El backend de command buffer fue elevado para soportar un radial con todos los
efectos y unidades simultáneamente:

```text
commands   1024
vertices   8192
indices   32768
```

Son capacidades de almacenamiento fijo. No hay `malloc`. El renderer normal de
Blank3D dibuja mediante callbacks directos, por lo que sólo se paga la memoria
del command buffer cuando el host declara una instancia.

## Errores de configuración

El bridge ya no ignora una propiedad GBar desconocida. Un typo produce un error
que incluye la clave, y un valor inválido incluye clave y valor:

```text
unknown GBar89 property: glso
a bad GBar89 property radial_cap=banana
```

Esto evita que BVH parezca aceptar una opción que en realidad no llegó al
renderer.

## Ejemplo combinado

```text
node player_ring -=)
type bar
bind player.health
min_bind zero
max_bind player.health_max
overlay_bind gameplay.threat
mid_bind weapon.reserve
pos top_right + 28,24
size 120,120
kind radial_ring
radial_start -90
radial_sweep 360
radial_inner 70
radial_segments 30
radial_gap_deg 2
radial_cap round
radial_phase_speed 24
frame bevel_out
background grid
outer_outline true
drop_shadow true
extrude true
inner_shadow true
gloss true
radial_spokes true
radial_rings true
radial_ticks true
radial_sweep_highlight true
draw_mid_value true
draw_value_overlay true
draw_vector_units true
unit_shape shield
unit_count 12
color_fill #42E47CFF
color_mid #5090C0A0
color_overlay #FF503080
```

## Archivos de integración

```text
vendor/gbar89/include/gbar89.h
vendor/gbar89/include/gbar89_bighud.h
vendor/gbar89/src/gbar89.c
vendor/gbar89/src/gbar89_bighud.c
src/blank3d_bighud.h
src/blank3d_bighud.c
src/blank3d_hud.h
src/blank3d_hud.c
config/hud/gameplay.bighud
tests/data/hud/gbar_v04_full.bighud
tests/test_gbar_v04_bighud.c
```
