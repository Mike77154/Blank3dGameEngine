# BigVaderHudder como orquestador de HUD de gameplay

## Arquitectura

```text
Gameplay real
├── player.health / player.health_max
├── weapon.loaded / weapon.capacity / weapon.reserve
├── gameplay.threat
├── damage.flash_ms
└── ecg.bpm
          │
          ▼
BigVaderHudder (`gameplay.bighud`)
├── crea nodos
├── resuelve anclas, tamaño y orden Z
├── enlaza variables del gameplay
├── entrega propiedades ECG → `ecg_bighud`
├── entrega propiedades BAR → `gbar89_bighud`
└── configura COUNTER → backend 5x7 OpenGL
          │
          ▼
Renderer Blank3D
├── ECG embebido
├── GBar89 rectángulos/líneas/triángulos/clip
└── texto 5x7
```

El documento se carga durante `blank3d_hud_init()`. Si falta o contiene un
error, se instala un HUD seguro de respaldo con ECG, barra de munición y
contador; el juego no se queda sin interfaz.

## Reglas de un nodo

`type` debe ser la primera propiedad especializada del nodo.

```text
node nombre -=)
type bar | ecg | counter
z 10
visible true
pos top_left + 20,18
size 320,24
```

Anclas disponibles:

```text
top_left       top_center       top_right
center
bottom_left    bottom_center    bottom_right
```

En anclas de borde derecho o inferior, offsets positivos se mueven hacia
dentro de la pantalla. Por ejemplo:

```text
pos bottom_right + 28,16
```

`pos` y `size` son la colocación de BigVaderHudder y tienen precedencia sobre
`x/y/w/h` internos de una barra. En un ECG, `pos/size` controlan la imagen
final en pantalla; `x/y/scale_*` del ECG siguen controlando su composición
interna sobre la superficie embebida.

## Bindings disponibles

```text
player.health
player.health_max
player.dead
weapon.loaded
weapon.capacity
weapon.reserve
weapon.empty
gameplay.threat
ecg.bpm
damage.flash_ms
camera.first_person
weapon.muzzle_flash
zero
one
```

También se acepta un entero literal como binding:

```text
min_bind 0
max_bind 100
```

## Nodo BAR

```text
node health -=)
type bar
bind player.health
min_bind zero
max_bind player.health_max
pos top_right + 28,24
size 108,108
kind radial_ring
radial_start -90
radial_sweep 360
radial_inner 70
radial_steps 64
```

Tipos:

```text
linear
segmented
sprite_clip
nineslice
radial_pie
radial_ring
```

Direcciones:

```text
left_to_right
right_to_left
top_to_bottom
bottom_to_top
center_horizontal
center_vertical
```

Bindings especiales de barra:

```text
bind             valor principal
min_bind         mínimo dinámico
max_bind         máximo dinámico
overlay_bind     valor de overlay/escudo
segments_bind    cantidad dinámica de segmentos
```

### Parámetros GBar89 expuestos

El bridge acepta los campos públicos de `GBar89_Meter` y `GBar89_Style`.
Las categorías principales son:

```text
kind, direction, flags
min, max, value, visual_value, lag_value
smooth_speed, lag_speed
range

overlay_min, overlay_max, overlay_value, overlay_visual_value
overlay_speed, overlay_direction, overlay_range

x, y, width, height, rect
segments, segment_gap
radial_start, radial_sweep, radial_inner, radial_steps
layer_count, layer_size
marker_count, markers, marker0..marker15
state, low_ratio, critical_ratio, state_timer, blink_period
mask_kind, mask_amount, mask_steps, mask_slice_count
mask_slice0..mask_slice15
ease_kind, ease_duration, ease_elapsed
ease_start_value, ease_target_value, ease_active
```

Flags individuales, editables como booleanos:

```text
clamp_value
draw_bg
draw_border
damage_lag
smooth_value
draw_markers
draw_overlay
use_visual
invert_ratio
layered
draw_value_overlay
draw_pattern
use_mask
auto_state
state_blink
ease_value
draw_layer_pips
```

Colores y ornamentación:

```text
color / color_fill
color_bg
color_lag
color_border
color_overlay
color_empty
color_marker
color_pattern

color_state_fill0..9
color_state_border0..9
color_layer_fill0..7
color_layer_lag0..7

border_size
margin_left, margin_top, margin_right, margin_bottom
padding_left, padding_top, padding_right, padding_bottom
marker_size
pattern_kind, pattern_step, pattern_size
```

Patrones:

```text
none
vertical_stripes
horizontal_stripes
crosshatch
dots
ticks
```

Máscaras:

```text
none
slant_right
slant_left
hexagon
diamond
custom_slices
```

Easing:

```text
linear
in_quad
out_quad
in_out_quad
smoothstep
out_cubic
```

También quedan expuestos los IDs y rectángulos de sprites:

```text
sprite_bg, sprite_fill, sprite_lag, sprite_overlay
sprite_empty, sprite_full, sprite_partial
src_bg, src_fill, src_lag, src_overlay
src_empty, src_full, src_partial
```

El backend integrado cubre rectángulos, líneas, triángulos radiales y clip.
Los IDs de sprite quedan en el contrato para un atlas futuro; el Blank3D
actual no enlaza todavía un proveedor de texturas al HUD.

## Nodo ECG

```text
node vitals -=)
type ecg
pos top_left + 20,18
size 360,160
visible_cols 32
fixed_offset 58
active.signal_style bars
active.x_step 3
active.y_step 3
active.glow false

content_alpha 255
background_alpha 76
clear_alpha 0
```

### Dinámica de gameplay expuesta

```text
dynamic_state
dynamic_bpm
dynamic_offset
dynamic_text
dynamic_damage_overlay
phase_direction
phase
phase_accum_ms

danger_below
orange_below
caution_below

bpm / fixed_bpm
bpm_base
bpm_threat_span
bpm_injury_span
bpm_min
bpm_max

interval_numerator
interval_min_ms
interval_max_ms
damage_interval_ms
damage_flash_period_ms
fixed_offset
clear_color
```

La velocidad de desplazamiento se calcula a partir de
`interval_numerator / bpm`, limitada por `interval_min_ms` y
`interval_max_ms`. Un numerador menor produce un barrido más rápido.

### Alfa del nodo ECG en pantalla

```text
content_alpha      # trazo, textos, rejilla y marco; 0..255
background_alpha   # píxeles de la caja background_color; 0..255
clear_alpha        # zona limpia alrededor de la caja; 0..255
```

Estos tres parámetros pertenecen al nodo BigVaderHudder. El bridge convierte
la superficie RGB del ECG a RGBA usando un buffer estático, de modo que el
fondo puede ser translúcido sin volver translúcido el pulso.

### Composición ECG expuesta

```text
x, y
scale, scale_x, scale_y
state
offset
visible_cols
draw_flags

active_x, active_y
overview_x, overview_y
state_text_x, state_text_y, state_text_scale
custom_text, custom_text_x, custom_text_y
custom_text_scale, custom_text_color

background_x, background_y
background_width, background_height
background_filled, background_color

overlay_x, overlay_y
overlay_width, overlay_height
overlay_filled, overlay_color
```

Flags legibles:

```text
show_active
show_overview
show_state_text
show_custom_text
show_background_box
show_overlay_box
```

Parámetros del renderer, con prefijo `active.`, `overview.` o `render.`:

```text
show_grid, show_axis, show_frame, show_signal, show_window
draw_flags
x_step / x_step_q8
y_step / y_step_q8
bar_width
grid_x_units, grid_y_units
axis_y_units, waveform_y_units
dot_on, dot_off
signal_style = continuous | dotted | bars
glow, glow_radius, glow_intensity
use_profile_glow_color
grid_color, axis_color, frame_color, window_color, glow_color
```

`render.` aplica la misma propiedad a vista activa y overview. Las variantes
sin prefijo de colores y estilo también se aplican a ambas vistas.

Estados editables o añadibles por nombre:

```text
state_color.NOMBRE
state_gradient.NOMBRE
state_glow.NOMBRE
state_profile.NOMBRE
```

Ejemplo:

```text
state_color.DANGER #FF241C
state_gradient.DANGER #120202
state_glow.DANGER #FF0000
state_profile.DANGER 3
```

## Nodo COUNTER

```text
node ammo_counter -=)
type counter
bind weapon.loaded
bind2 weapon.reserve
mode pair
pos bottom_right + 28,16
label AMMO
prefix " "
separator /
suffix ""
pad 2
pad2 3
scale 2
glyph_gap 1
color #F4DA68FF
shadow true
shadow_color #000000D0
background false
background_color #000000A0
border false
border_color #FFFFFFFF
```

`mode value` dibuja un valor; `mode pair` dibuja `bind + separator + bind2`.
La fuente integrada acepta letras latinas mayúsculas, números y los signos
`: - / . % +`.

## Presets rápidos

### Barra horizontal de vida

```text
node hp_horizontal -=)
type bar
bind player.health
min_bind zero
max_bind player.health_max
pos top_left + 24,24
size 280,20
kind linear
direction left_to_right
draw_bg true
draw_border true
color_fill #38E878FF
color_bg #08100CD8
color_border #B8FFD0FF
```

### Barra vertical

```text
node hp_vertical -=)
type bar
bind player.health
min_bind zero
max_bind player.health_max
pos top_right + 24,24
size 18,120
kind linear
direction bottom_to_top
```

### Munición segmentada por capacidad real

```text
node ammo -=)
type bar
bind weapon.loaded
min_bind zero
max_bind weapon.capacity
segments_bind weapon.capacity
pos bottom_right + 24,40
size 300,18
kind segmented
segment_gap 2
```

### Aro circular

```text
node hp_ring -=)
type bar
bind player.health
min_bind zero
max_bind player.health_max
kind radial_ring
radial_start -90
radial_sweep 360
radial_inner 72
radial_steps 64
pos top_right + 24,24
size 112,112
```

## Archivos añadidos

```text
src/blank3d_bighud.h
src/blank3d_bighud.c
vendor/gbar89/include/gbar89_bighud.h
vendor/gbar89/src/gbar89_bighud.c
vendor/ecg_embedded_c89/include/ecg_bighud.h
vendor/ecg_embedded_c89/src/ecg_bighud.c
config/hud/gameplay.bighud
tests/test_bighud.c
```

## Actualización GBar89 v0.4 — superficie completa

Blank3D v3.23.0 sustituye el vendor anterior por GBar89 v0.4. BVH ahora puede
orquestar frames y fondos procedurales, compositor 2D, banda intermedia,
pixelado, unidades vectoriales, paridad vertical y la superficie radial
completa. También añade bindings dinámicos para overlay range, middle value,
estados, fase radial, unidades y layers.

La referencia exhaustiva, incluidas todas las claves, aliases, formas
vectoriales, sprites y nine-slice, está en:

```text
GBAR89_V04_BIGHUD_FULL_SURFACE.md
```

La prueba exhaustiva es:

```bash
make test-gbar-v04-bighud
```
