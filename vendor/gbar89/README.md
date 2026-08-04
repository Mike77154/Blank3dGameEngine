# GBAR89 v0.4 - General Bar / Meter Compositor for C89

GBAR89 is a backend-agnostic C89 support library and procedural 2D compositor for visual meters in games.
It is designed for health bars, stamina bars, shield bars, boss bars, segmented cells,
cooldowns, radial meters, sprite-clipped bars, nine-slice bars, masked bars, and HUD debug meters.

## Hard constraints

- C89 style.
- No `malloc`, `free`, `realloc`.
- No heap ownership.
- No `float`, no `double`.
- Fixed-point helpers only, using 16.16 `long` fixed point.
- No PNG loading, no file I/O, no platform dependency.
- You provide drawing callbacks.

## Build

```sh
make
make test
```

Or directly:

```sh
cc -std=c89 -Wall -Wextra -pedantic -Iinclude src/gbar89.c examples/ascii_demo.c -o ascii_demo
cc -std=c89 -Wall -Wextra -pedantic -Iinclude src/gbar89.c examples/v02_feature_demo.c -o v02_feature_demo
```

## Mental model

```text
game value
   |
   v
clamp + normalize to 16.16 ratio
   |
   v
layout / direction / segmentation / layer / mask / radial conversion
   |
   v
abstract drawing callbacks or fixed command buffer
   |
   v
SDL / OpenGL / Allegro / software renderer / console / your engine
```

## Render callbacks

GBAR89 does not draw to a concrete API. It emits calls to your backend:

- `draw_rect`: simple bars, patterns, borders, markers, fallback path.
- `draw_sprite`: source-rect to dest-rect texture copy; useful for sprite clipping.
- `draw_triangles`: radial pie/ring meters and polygon masks.
- `push_clip` / `pop_clip`: nine-slice fills and masked fills.
- `draw_line`: optional utility.


## v0.4 radial and vertical parity

v0.4 applies the procedural compositor to radial pie/ring meters and fixes the remaining vertical asymmetries.
The radial path now follows the same order as a linear bar:

```text
background -> lag -> primary fill -> middle band -> overlay
           -> surface details -> vector units / markers -> frame
```

### Radial faux-3D, outline and frames

The existing frame enum now has radial interpretations: concentric simple/double frames,
outward/inward bevel, arc brackets, rail with ticks, and a pixel/segmented rim.
Drop shadow, extrusion, outer outline, gloss and inner shadow are generated with triangles and lines only.

```c
gbar89_set_kind(&bar, GBAR89_KIND_RADIAL_RING);
gbar89_set_radial(&bar, -90, 300, 62, 64);
gbar89_set_frame(&bar, GBAR89_FRAME_RAIL, 3, 2, 18);
gbar89_set_outline(&bar, 2, 0x02060CFFUL);

gbar89_set_fx_flags(&bar,
    GBAR89_FX_OUTER_OUTLINE |
    GBAR89_FX_DROP_SHADOW |
    GBAR89_FX_EXTRUDE |
    GBAR89_FX_GLOSS |
    GBAR89_FX_INNER_SHADOW |
    GBAR89_FX_RADIAL_TICKS);
```

### Segmented arcs and cap styles

```c
gbar89_set_radial_style(&bar,
    18,                        /* angular segments */
    4,                         /* gap in degrees */
    GBAR89_RADIAL_CAP_ROUND,   /* butt / round / square */
    3,                         /* concentric detail rings */
    82);                       /* vector-unit radius */
```

A one-segment ring is continuous. Higher counts produce dashed/cell rings without textures.
`gbar89_set_radial_phase()` offsets procedural details and can be changed every frame.

### Radial backgrounds and fill surfaces

The existing procedural backgrounds are reinterpreted in polar space:

```text
grid      -> concentric rings + spokes
checker   -> alternating angular/radial cells
scanlines -> concentric bands
diagonal  -> slanted spoke links
dither    -> sparse angular cells
```

The fill supports grid, scanlines, angular pixel cells, radial rings/spokes,
leading-sweep highlight and patterns. Damage lag, overlay and middle/ghost values work as arcs.
Markers become radial ticks, vector units are distributed around the circumference,
and layered boss pips become small outer arc cells.

### Vertical parity

Horizontal styling already used rectangles, but partial segmented fills were still hardwired to
left-to-right. v0.4 passes the actual direction through every segment, so top-to-bottom,
bottom-to-top and center-vertical fills now preserve partial cells, lag, middle bands,
overlays, pixel quantization, markers, vector units, frame effects and patterns.
Gloss is orientation-aware: horizontal meters receive a top highlight, vertical meters a side highlight.

### v0.4 preview

```sh
make radial_preview
```

`examples/radial_vertical_preview_ppm.c` renders a full radial/vertical showcase and 48 animation frames
through the real GBAR89 callbacks. No platform renderer or image library is required by the example itself.

## v0.3 graphic operations

v0.3 keeps the v0.2 renderer intact and adds a style/compositor layer. All effects are emitted through the same backend callbacks; no platform API, heap allocation, floating point, texture loader, or shader is required.

### Procedural frame kinds

```text
simple, double, bevel_out, bevel_in, brackets, rail, pixel, none
```

```c
gbar89_set_frame(&bar, GBAR89_FRAME_DOUBLE, 2, 2, 10);
```

### Composable faux-3D and surface effects

```text
outer outline, drop shadow, extrusion, inner shadow,
gloss, fill grid, fill scanlines, pixel cells
```

```c
gbar89_set_fx_flags(&bar,
    GBAR89_FX_OUTER_OUTLINE |
    GBAR89_FX_EXTRUDE |
    GBAR89_FX_GLOSS |
    GBAR89_FX_INNER_SHADOW);
```

These are ordinary 2D rectangles, lines, clips, and triangles. A backend may alpha-blend the RGBA colors, but no depth buffer or 3D API is needed.

### Procedural backgrounds

```text
solid, grid, checker, scanlines, diagonal, dither
```

```c
gbar89_set_background(&bar, GBAR89_BG_GRID, 8, 1, 0x20809080UL);
```

### Pixel-quantized bars

```c
bar.flags |= GBAR89_FLAG_PIXEL_QUANTIZE;
gbar89_set_pixel_size(&bar, 8);
```

The visible fill length snaps to integer cells while the game value remains exact.

### Independent middle / ghost value

The middle value is a separately animated value in the main meter range. GBAR89 draws the interval between the principal value and the middle value using `style.color_mid`.

```c
bar.flags |= GBAR89_FLAG_DRAW_MID_VALUE;
gbar89_set_mid_value(&bar, 78);
gbar89_set_mid_speed(&bar, 240);
bar.style.color_mid = 0x60D8FFFFUL;
```

Typical uses: recoverable health, cost preview, temporary damage, predicted value, shield break residue, or a second timing cursor.

### Vector units

A meter can repeat a user-owned vector glyph. Coordinates use a normalized integer box from `0..1000`, so no floating point or heap ownership is involved. Filled convex glyphs use `draw_triangles`; any glyph can use `draw_line` as an outline.

```c
static const GBar89_VectorPoint diamond[4] = {
    {500, 0}, {1000, 500}, {500, 1000}, {0, 500}
};

bar.flags |= GBAR89_FLAG_DRAW_VECTOR_UNITS;
gbar89_set_unit_vector(&bar, diamond, 4, 1, 1, 8, 3, 76);
```

This can represent hearts, bullets, batteries, diamonds, armor plates, spell charges, or any small convex icon supplied by the caller.

### Preview renderer

`examples/super_preview_ppm.c` is a complete fixed-storage software backend. It implements rectangle, line, triangle, and clip callbacks, then renders a showcase and animation frames to PPM.

```sh
make preview
```

The example is intentionally independent of SDL, OpenGL, GDI, or Allegro, so it also serves as a backend reference.

## v0.2 features

### 1. Fixed command buffer

Use this when you want GBAR89 to produce a fixed list of commands first and draw them later.
No heap is used; storage is inside `GBar89_CommandBuffer`.

```c
GBar89_CommandBuffer cb;
GBar89_RenderOps ops;

gbar89_command_buffer_init(&cb);
ops = gbar89_command_buffer_make_ops(&cb);

gbar89_draw(&bar, &ops);

/* Later: replay into a real renderer. */
gbar89_command_buffer_replay(&cb, &real_ops);
```

### 2. Real layered / boss bars

Layered bars map a big value into a visible per-layer chunk.
Example: `2450 / 3000` with 3 layers of 1000 draws layer 3 at `450 / 1000`.

```c
gbar89_set_range(&boss, 0, 3000);
gbar89_set_layers(&boss, 3, 1000);
boss.flags |= GBAR89_FLAG_LAYERED | GBAR89_FLAG_DRAW_LAYER_PIPS;
gbar89_set_value(&boss, 2450);
```

Useful helpers:

```c
gbar89_layer_index_from_value(&boss, boss.value);
gbar89_layer_value_from_value(&boss, boss.value);
gbar89_layer_ratio_from_value(&boss, boss.value);
gbar89_layer_count_filled(&boss, boss.value);
```

### 3. Overlay / shield bars

A numeric overlay can be drawn over the main meter. Good for shields, armor overlays,
infection, stagger, temporary HP, or absorbed damage.

```c
bar.flags |= GBAR89_FLAG_DRAW_VALUE_OVERLAY;
gbar89_set_overlay_range(&bar, 0, 600);
gbar89_set_overlay_value(&bar, 250);
gbar89_set_overlay_direction(&bar, GBAR89_DIR_LEFT_TO_RIGHT);
```

### 4. Visual states

GBAR89 includes state slots for normal, low, critical, poisoned, regenerating,
shielded, overheat, locked, broken, and custom.

```c
bar.flags |= GBAR89_FLAG_AUTO_STATE;
gbar89_set_state_thresholds(&bar, GBAR89_FIX_ONE / 3, GBAR89_FIX_ONE / 6);
gbar89_set_state_color(&bar, GBAR89_STATE_POISONED, 0xB040D0FFUL, 0xFFFFFFFFUL);
```

### 5. Accessible patterns

Patterns let the bar communicate status without relying on color alone.

```c
bar.flags |= GBAR89_FLAG_DRAW_PATTERN;
gbar89_set_pattern(&bar, GBAR89_PATTERN_CROSSHATCH, 8, 1);
```

Available patterns:

```text
none, vertical stripes, horizontal stripes, crosshatch, dots, ticks
```

### 6. Advanced masks

Masked fills allow slanted, hexagonal, diamond, or custom-slice bars.
The best backend path is `push_clip` + `draw_triangles`.

```c
bar.flags |= GBAR89_FLAG_USE_MASK;
gbar89_set_mask(&bar, GBAR89_MASK_HEXAGON, 12, 8);
```

Custom slices are fixed records inside the meter:

```c
gbar89_set_mask(&bar, GBAR89_MASK_CUSTOM_SLICES, 0, 8);
gbar89_clear_mask_slices(&bar);
gbar89_add_mask_slice(&bar,  0, 20, 8, 0);
gbar89_add_mask_slice(&bar, 20, 20, 4, 2);
gbar89_add_mask_slice(&bar, 40, 20, 0, 4);
```

### 7. Fixed-point easing

Easing is implemented without floats.

```c
gbar89_set_easing(&bar, GBAR89_EASE_SMOOTHSTEP, GBAR89_FIX_ONE);
gbar89_set_value(&bar, 20);
gbar89_tick(&bar, GBAR89_FIX_ONE / 4);
```

Available easing modes:

```text
linear, in_quad, out_quad, in_out_quad, smoothstep, out_cubic
```

You can also call the easing functions directly:

```c
GBar89_Fix t;
GBar89_Fix eased;

t = GBAR89_FIX_ONE / 2;
eased = gbar89_ease(GBAR89_EASE_OUT_QUAD, t);
```

## Tiny usage

```c
#include "gbar89.h"

static void my_rect(void *u, int x, int y, int w, int h, unsigned long rgba)
{
    /* draw in your engine */
    (void)u;
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    (void)rgba;
}

void draw_hp(long hp)
{
    GBar89_Meter bar;
    GBar89_RenderOps ops;

    gbar89_init(&bar);
    gbar89_set_rect(&bar, 16, 16, 180, 12);
    gbar89_set_range(&bar, 0, 100);
    gbar89_set_value(&bar, hp);
    gbar89_tick(&bar, 0);

    ops.user = 0;
    ops.draw_rect = my_rect;
    ops.draw_line = 0;
    ops.draw_sprite = 0;
    ops.draw_triangles = 0;
    ops.push_clip = 0;
    ops.pop_clip = 0;

    gbar89_draw(&bar, &ops);
}
```

## Design references

The design follows common meter ideas seen in engine documentation:

- Unity UI Filled Image: 0..1 fill amount and fill method for linear/radial status displays.
- Godot TextureProgressBar: under/progress/over textures, horizontal/vertical/radial modes, and nine-patch stretching.
- Unreal UProgressBar: percent value, fill direction, fill style, padding, and restyling.
- SDL render APIs: source/destination sprite copies, clipping rectangles, and triangle geometry.
- WCAG guidance: do not communicate information by color alone.

