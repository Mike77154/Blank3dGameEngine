# GBAR89 v0.4 - Especificación en español

GBAR89 es una librería C89 para componer medidores visuales de videojuego.
No es un renderer completo ni un loader de imágenes. Su trabajo es convertir valores de juego en rectángulos, clips, sprites, segmentos, máscaras, anillos, pips y comandos abstractos.

## Restricciones duras

```text
C89
sin malloc
sin free
sin realloc
sin heap ownership
sin float
sin double
sin stdint.h obligatorio
sin bool C99
sin VLAs
sin dependencias externas
```

Usa fixed point 16.16:

```c
typedef long GBar89_Fix;

#define GBAR89_FIX_SHIFT 16
#define GBAR89_FIX_ONE   (1L << GBAR89_FIX_SHIFT)
```


## Extensión v0.4: compositor polar y paridad vertical

La barra radial ya no es solamente un abanico de triángulos. Usa el mismo pipeline visual de las barras rectangulares:

```text
pre-FX radial
  sombra / extrusión / outline
        ↓
fondo polar + detalle
        ↓
lag → fill → mid band → overlay
        ↓
gloss / inner shadow / grid / rings / spokes / pixel cells
        ↓
marcadores / unidades vectoriales / pips
        ↓
marco radial
```

### API radial de estilo

```c
gbar89_set_radial_style(&meter,
                         segments,
                         gap_degrees,
                         cap_kind,
                         detail_rings,
                         unit_radius_percent);

gbar89_set_radial_phase(&meter, phase_degrees);
```

`segments == 1` produce un arco continuo. Los demás valores dividen el barrido total en celdas angulares.
Los caps disponibles son `BUTT`, `ROUND` y `SQUARE`.

### Efectos polares

```text
GBAR89_FX_RADIAL_SPOKES
GBAR89_FX_RADIAL_RINGS
GBAR89_FX_RADIAL_TICKS
GBAR89_FX_RADIAL_SWEEP_HIGHLIGHT
```

Se combinan con los efectos existentes de outline, shadow, extrude, gloss, inner shadow, fill grid,
scanlines y pixel cells.

### Paridad vertical

Todas las direcciones verticales pasan ahora su dirección real al relleno parcial de cada segmento.
Esto corrige la última dependencia izquierda→derecha en barras segmentadas. El gloss usa el eje transversal:
arriba en barras horizontales y lateral en barras verticales.

---

## Modelo mental

```text
valor lógico del juego
   ↓
clamp / normalización / layer mapping
   ↓
ratio fixed-point 16.16
   ↓
forma visual: linear, segmentada, radial, sprite, nine-slice, máscara
   ↓
patrones, overlays, marcadores, estados
   ↓
callbacks de dibujo o command buffer fijo
   ↓
backend real: SDL, OpenGL, Allegro, software renderer, engine propio
```

---

# Paso 1 - Command buffer fijo

## Objetivo

Permitir que GBAR89 genere comandos primero y que el juego los dibuje después.
Esto sirve para:

```text
- batching
- debug
- replay visual
- inspeccionar lo que se va a dibujar
- separar update/render
- backend multipaso
```

## API

```c
GBar89_CommandBuffer cb;
GBar89_RenderOps ops;

gbar89_command_buffer_init(&cb);
ops = gbar89_command_buffer_make_ops(&cb);

gbar89_draw(&bar, &ops);

gbar89_command_buffer_replay(&cb, &real_ops);
```

## Almacenamiento

Todo está dentro de `GBar89_CommandBuffer`:

```text
commands[GBAR89_MAX_COMMANDS]
vertices[GBAR89_MAX_COMMAND_VERTICES]
indices[GBAR89_MAX_COMMAND_INDICES]
```

Si se llena, marca `overflow`, pero no intenta reservar memoria.

---

# Paso 2 - Boss bars multi-layer reales

## Problema

Un boss puede tener 3000 HP, pero visualmente quieres mostrarlo como 3 capas de 1000.

Ejemplo:

```text
2450 / 3000
↓
layer index: 2
layer value: 450 / 1000
filled layers: 3
```

## API

```c
gbar89_set_range(&boss, 0, 3000);
gbar89_set_layers(&boss, 3, 1000);
boss.flags |= GBAR89_FLAG_LAYERED;
gbar89_set_value(&boss, 2450);
```

## Helpers

```c
gbar89_layer_index_from_value(&boss, boss.value);
gbar89_layer_value_from_value(&boss, boss.value);
gbar89_layer_ratio_from_value(&boss, boss.value);
gbar89_layer_count_filled(&boss, boss.value);
```

## Pips de capa

```c
boss.flags |= GBAR89_FLAG_DRAW_LAYER_PIPS;
```

Dibuja indicadores pequeños para saber cuántas capas quedan.

---

# Paso 3 - Overlay / shield bars

## Objetivo

Dibujar una segunda cantidad sobre la barra principal.

Sirve para:

```text
- escudo
- armadura temporal
- daño absorbido
- stagger
- infección
- carga secundaria
- temporary HP
```

## API

```c
bar.flags |= GBAR89_FLAG_DRAW_VALUE_OVERLAY;

gbar89_set_overlay_range(&bar, 0, 600);
gbar89_set_overlay_value(&bar, 250);
gbar89_set_overlay_direction(&bar, GBAR89_DIR_LEFT_TO_RIGHT);
```

El overlay tiene su propio rango, valor visual, velocidad y dirección.

---

# Paso 4 - Estados visuales

## Estados incluidos

```text
normal
low
critical
poisoned
regenerating
shielded
overheat
locked
broken
custom
```

## API manual

```c
gbar89_set_state(&bar, GBAR89_STATE_POISONED);
gbar89_set_state_color(&bar, GBAR89_STATE_POISONED,
                       0xB040D0FFUL,
                       0xFFFFFFFFUL);
```

## Auto state

```c
bar.flags |= GBAR89_FLAG_AUTO_STATE;
gbar89_set_state_thresholds(&bar,
                            GBAR89_FIX_ONE / 3,
                            GBAR89_FIX_ONE / 6);
```

Con eso:

```text
ratio <= critical_ratio -> critical
ratio <= low_ratio      -> low
si no                   -> normal
```

## Blink opcional

```c
bar.flags |= GBAR89_FLAG_STATE_BLINK;
```

---

# Paso 5 - Patrones accesibles

## Motivo

Una barra no debería comunicar información solo por color. Los patrones permiten distinguir estados incluso si el jugador no percibe bien ciertos colores.

## API

```c
bar.flags |= GBAR89_FLAG_DRAW_PATTERN;
gbar89_set_pattern(&bar, GBAR89_PATTERN_CROSSHATCH, 8, 1);
```

## Patrones disponibles

```text
GBAR89_PATTERN_NONE
GBAR89_PATTERN_VERTICAL_STRIPES
GBAR89_PATTERN_HORIZONTAL_STRIPES
GBAR89_PATTERN_CROSSHATCH
GBAR89_PATTERN_DOTS
GBAR89_PATTERN_TICKS
```

---

# Paso 6 - Máscaras avanzadas

## Objetivo

Permitir barras que no sean solo rectángulos.

## Máscaras incluidas

```text
GBAR89_MASK_NONE
GBAR89_MASK_SLANT_RIGHT
GBAR89_MASK_SLANT_LEFT
GBAR89_MASK_HEXAGON
GBAR89_MASK_DIAMOND
GBAR89_MASK_CUSTOM_SLICES
```

## Uso básico

```c
bar.flags |= GBAR89_FLAG_USE_MASK;
gbar89_set_mask(&bar, GBAR89_MASK_HEXAGON, 12, 8);
```

## Backend ideal

```text
push_clip + draw_triangles + pop_clip
```

Si el backend no tiene triángulos o clipping, GBAR89 cae a rectángulos.

## Custom slices

Permite construir formas raras con rebanadas fijas.

```c
gbar89_set_mask(&bar, GBAR89_MASK_CUSTOM_SLICES, 0, 8);
gbar89_clear_mask_slices(&bar);
gbar89_add_mask_slice(&bar,  0, 20, 8, 0);
gbar89_add_mask_slice(&bar, 20, 20, 4, 2);
gbar89_add_mask_slice(&bar, 40, 20, 0, 4);
gbar89_add_mask_slice(&bar, 60, 20, 4, 2);
gbar89_add_mask_slice(&bar, 80, 20, 8, 0);
```

---

# Paso 7 - Easing fixed-point

## Objetivo

Animar valores visuales sin floats.

## API

```c
gbar89_set_easing(&bar, GBAR89_EASE_SMOOTHSTEP, GBAR89_FIX_ONE);
gbar89_set_value(&bar, 20);
gbar89_tick(&bar, GBAR89_FIX_ONE / 4);
```

## Modos

```text
GBAR89_EASE_LINEAR
GBAR89_EASE_IN_QUAD
GBAR89_EASE_OUT_QUAD
GBAR89_EASE_IN_OUT_QUAD
GBAR89_EASE_SMOOTHSTEP
GBAR89_EASE_OUT_CUBIC
```

También se puede usar directo:

```c
GBar89_Fix eased;
eased = gbar89_ease(GBAR89_EASE_OUT_QUAD, GBAR89_FIX_ONE / 2);
```

---

# Tipos de medidor soportados

```text
GBAR89_KIND_LINEAR
GBAR89_KIND_SEGMENTED
GBAR89_KIND_SPRITE_CLIP
GBAR89_KIND_NINESLICE
GBAR89_KIND_RADIAL_PIE
GBAR89_KIND_RADIAL_RING
```

Los flags nuevos son combinables con estos tipos cuando tiene sentido:

```text
layered + linear/nineslice/sprite/segmented/radial
value overlay + linear/nineslice/sprite/segmented/radial
patterns + rect based paths
masks + linear/nineslice fallback rect paths
states + todos los fills que usan color/tint
```

---

# Filosofía final

GBAR89 v0.2 no intenta ser una UI completa.

Su frase correcta sería:

> Compositor C89 de medidores visuales: valores -> ratios -> formas -> comandos.



---

## Extensión v0.3: compositor gráfico procedural

La versión 0.3 agrega una fase de estilizado que sigue emitiendo únicamente operaciones abstractas del backend. No introduce shaders, texturas obligatorias, memoria dinámica ni tipos flotantes.

### Orden conceptual

```text
sombra / extrusión exterior
        ↓
fondo base + detalle procedural
        ↓
lag de daño
        ↓
valor principal + patrones + efectos de superficie
        ↓
banda intermedia / ghost value
        ↓
overlay numérico
        ↓
unidades vectoriales + marcadores
        ↓
marco procedural
```

### Marcos

- `GBAR89_FRAME_SIMPLE`: borde rectangular clásico.
- `GBAR89_FRAME_DOUBLE`: borde exterior y línea interior.
- `GBAR89_FRAME_BEVEL_OUT`: luz arriba/izquierda y sombra abajo/derecha.
- `GBAR89_FRAME_BEVEL_IN`: bisel invertido o hundido.
- `GBAR89_FRAME_BRACKETS`: únicamente esquinas en forma de L.
- `GBAR89_FRAME_RAIL`: rieles horizontales y tapas laterales.
- `GBAR89_FRAME_PIXEL`: esquinas escalonadas sin antialiasing.
- `GBAR89_FRAME_NONE`: omite el marco procedural.

### Fondos

Los fondos `grid`, `checker`, `scanlines`, `diagonal` y `dither` se construyen con rectángulos pequeños y respetan el clip del backend cuando está disponible.

### Pixelización

`GBAR89_FLAG_PIXEL_QUANTIZE` cuantiza únicamente la longitud gráfica de relleno. El valor lógico y el cálculo del ratio conservan toda la precisión fixed-point.

### Banda intermedia

`mid_value` comparte el rango principal. `mid_visual_value` se aproxima a `mid_value` mediante `mid_speed`. El renderer colorea el intervalo entre el ratio principal y el ratio intermedio; por ello sirve tanto para valores futuros mayores como para costos o pérdidas previstas menores.

### Unidades vectoriales

- El arreglo de puntos pertenece al usuario y debe permanecer vivo durante el dibujo.
- Cada coordenada usa el rango entero `0..1000`.
- El relleno por abanico de triángulos está pensado para polígonos convexos.
- Los contornos aceptan cualquier polilínea razonable mediante `draw_line`.
- `GBAR89_MAX_VECTOR_POINTS` limita la copia temporal usada al emitir geometría.

### Degradación por capacidades

- Sin `draw_triangles`, los glifos rellenos conservan su contorno si existe `draw_line`.
- Sin `push_clip/pop_clip`, los efectos rectangulares siguen funcionando; algunas máscaras complejas degradan al rectángulo, igual que en v0.2.
- Sin blending alfa, los colores con alfa se interpretan según el backend. Para resultados deterministas se pueden usar colores opacos.
