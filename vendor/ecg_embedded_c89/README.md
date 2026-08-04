# ECG Embedded C89 Library 1.2.0

Procedural ECG renderer plus a fully configurable HUD layer for strict C89
engines.

## Guarantees

- Strict C89 source.
- Caller-owned framebuffer.
- No dynamic allocation.
- Q8.8 fixed-point geometry and scaling.
- Independent X/Y scaling.
- No mandatory platform API.
- Optional PNG writer remains outside the core library.
- User-defined HUD states with user-defined names and colors.
- Continuous, dotted and legacy bar traces.
- Configurable software glow.

## What changed in 1.2.0

The original five names remain available as defaults, but they are no longer a
closed enum. A game may define any names it needs:

```text
STABLE
BLEEDING
FROZEN
RADIATION
BOSS-PHASE-3
PLAYER-2-CRITICAL
```

The same canonical name is used by code, HUD text and key/value configuration.
Names are stored uppercase and matched case-insensitively.

Each state owns:

```text
name
main trace color
per-age fade amount
illumination color
waveform/profile index
```

## State capacity without a heap

`ECG_HudConfig` includes room for 16 states by default. The first five slots are
initialized as `FINE`, `CAUTION`, `ORANGE`, `DANGER` and `POISON` for backward
compatibility.

For any other capacity, provide a static array owned by the host:

```c
#define MY_STATE_CAPACITY 128u

static ECG_HudStateDef my_states[MY_STATE_CAPACITY];
static ECG_HudConfig hud;

void hud_init(void)
{
    ecg_hud_config_default(&hud);

    ecg_hud_config_bind_state_storage(&hud,
                                      my_states,
                                      MY_STATE_CAPACITY,
                                      0u);
}
```

The fourth argument selects whether existing definitions are copied into the
new storage. Use `1u` to preserve the defaults or `0u` to start empty.

The practical state count is therefore selected by the engine, not fixed by the
ECG library.

## Add states from C

```c
ECG_HudState frozen_state;

static ECG_HudStateDef states[64];

void define_states(ECG_HudConfig *hud)
{
    ecg_hud_config_bind_state_storage(hud, states, 64u, 0u);

    ecg_hud_config_add_state(
        hud,
        "FROZEN",
        ecg_color_make(70u, 205u, 255u),
        ecg_color_make(2u, 6u, 8u),
        ecg_color_make(20u, 120u, 255u),
        ECG_PROFILE_ORANGE,
        &frozen_state
    );

    ecg_hud_config_set_state_name(hud, "FROZEN");
}
```

`profile_index` chooses the waveform. `ecg_draw_hud_ex()` accepts any
caller-provided profile array and count, so custom states are not restricted to
the five bundled waveforms.

## Add states from configuration

A named state color automatically creates that state if it does not exist:

```ini
hud.states.clear=1

hud.state.STABLE=30,255,110
hud.gradient.STABLE=1,7,3
hud.glow_color.STABLE=20,255,80
hud.profile.STABLE=0

hud.state.BLEEDING=255,35,50
hud.gradient.BLEEDING=7,1,1
hud.glow_color.BLEEDING=255,0,0
hud.profile.BLEEDING=3

hud.state.FROZEN=70,205,255
hud.gradient.FROZEN=2,6,8
hud.glow_color.FROZEN=20,120,255
hud.profile.FROZEN=2

hud.state=FROZEN
```

The host remains responsible for reading the file and separating each
`key=value` pair. Pass each pair to:

```c
ecg_hud_config_apply_pair(&hud, key, value);
```

## Change colors at runtime

Change the currently selected state:

```c
ecg_hud_config_apply_pair(&hud, "hud.color", "255,40,40");
ecg_hud_config_apply_pair(&hud, "hud.gradient", "7,1,1");
ecg_hud_config_apply_pair(&hud, "hud.glow_color", "255,0,0");
```

Change a named state without selecting it:

```c
ecg_hud_config_apply_pair(&hud,
                          "hud.color.BLEEDING",
                          "220,15,25");
```

Renderer colors are configurable too:

```ini
hud.grid.color=18,28,30
hud.axis.color=42,72,65
hud.frame.color=50,80,75
hud.window.color=230,230,230
hud.custom_text.color=220,235,225
hud.background_box.color=35,50,50
hud.overlay_box.color=255,40,40
```

## Continuous line and optional dots

The renderer now stores the original 80 ECG samples and connects adjacent
samples with an integer Bresenham line.

Solid trace:

```ini
hud.signal.style=continuous
```

Dotted trace:

```ini
hud.signal.style=dotted
hud.signal.dot_on=2
hud.signal.dot_off=3
```

Legacy vertical columns remain available:

```ini
hud.signal.style=bars
```

A boolean alias is also accepted:

```ini
hud.signal.dotted=1
```

Set it to `0` to return to a continuous trace.

C equivalents:

```c
hud.active_render.signal_style = ECG_SIGNAL_STYLE_CONTINUOUS;
hud.overview_render.signal_style = ECG_SIGNAL_STYLE_DOTTED;

hud.active_render.dot_on_px = 2u;
hud.active_render.dot_off_px = 3u;
```

Trace thickness:

```ini
hud.signal.line_width=3
```

## Illumination / glow

Glow is drawn beneath the core trace. It uses a component-lighten blend, so
repeated points do not wash the trace into white.

```ini
hud.signal.glow=1
hud.signal.glow_radius=3
hud.signal.glow_intensity=92
```

`glow_intensity` accepts `0` through `255`. Each state may use a separate glow
color:

```ini
hud.glow_color.FROZEN=20,120,255
hud.glow_color.BLEEDING=255,0,0
```

C configuration:

```c
hud.active_render.glow_enabled = 1u;
hud.active_render.glow_radius_px = 3u;
hud.active_render.glow_intensity = 92u;
```

## Position and arbitrary scale

The HUD has one absolute origin:

```c
hud.x = 40;
hud.y = 24;
```

Every child position is relative to it. Global scale affects child offsets,
boxes, text and both ECG renderers:

```ini
hud.scale.x=1.5
hud.scale.y=0.75
```

Accepted scale formats:

```text
2
1.5
0.75
3/2
3/4
```

Uniform scaling:

```ini
hud.scale=1.25
```

The active and overview graphs can also have their own base sample scale:

```ini
hud.active.scale.x=3.5
hud.active.scale.y=2
hud.overview.scale.x=2
hud.overview.scale.y=1.5
```

C equivalents:

```c
hud.scale_x_q8 = ecg_fixed_from_ratio(3, 2);
hud.scale_y_q8 = ecg_fixed_from_ratio(3, 4);

hud.active_render.x_step_q8 = ecg_fixed_from_ratio(7, 2);
hud.active_render.y_step_q8 = ecg_fixed_from_int(2);
```

The framebuffer clips drawing automatically when a scaled HUD extends outside
the surface.

## Positioning children

```ini
hud.x=40
hud.y=24
hud.active.x=160
hud.active.y=0
hud.overview.x=670
hud.overview.y=0
hud.state_text.x=0
hud.state_text.y=18
hud.custom_text.x=0
hud.custom_text.y=44
```

Final child position:

```text
HUD origin + scaled child offset
```

## Show or hide parts

```ini
hud.show.active=1
hud.show.overview=1
hud.show.state_text=1
hud.show.custom_text=1
hud.show.background_box=1
hud.show.overlay_box=0
hud.show.grid=1
hud.show.axis=1
hud.show.frame=1
hud.show.signal=1
hud.show.window=1
```

Boolean values accept `0/1`, `true/false`, `yes/no` and `on/off`.

## Custom profiles

The old convenience function still accepts the bundled five-profile array:

```c
ecg_draw_hud(&surface, profiles, &hud);
```

For a larger caller-owned waveform library:

```c
ecg_draw_hud_ex(&surface,
                my_profiles,
                my_profile_count,
                &hud);
```

A state stores only the profile index, so many named states may share one
waveform while keeping separate colors and glow settings.

## Full minimal example

```c
#include "ecg.h"

#define W 640u
#define H 180u
#define STATE_CAPACITY 32u

static ECG_Color pixels[W * H];
static ECG_Profile profiles[ECG_PROFILE_COUNT];
static ECG_HudStateDef states[STATE_CAPACITY];

void draw_frame(void)
{
    ECG_Surface surface;
    ECG_HudConfig hud;

    surface.pixels = pixels;
    surface.width = W;
    surface.height = H;
    surface.stride = W;

    ecg_profiles_init(profiles);
    ecg_hud_config_default(&hud);
    ecg_hud_config_bind_state_storage(&hud,
                                      states,
                                      STATE_CAPACITY,
                                      0u);

    ecg_hud_config_apply_pair(&hud,
                              "hud.state.FROZEN",
                              "70,205,255");
    ecg_hud_config_apply_pair(&hud,
                              "hud.glow_color.FROZEN",
                              "20,120,255");
    ecg_hud_config_apply_pair(&hud,
                              "hud.profile.FROZEN",
                              "2");
    ecg_hud_config_apply_pair(&hud, "hud.state", "FROZEN");
    ecg_hud_config_apply_pair(&hud,
                              "hud.signal.style",
                              "continuous");
    ecg_hud_config_apply_pair(&hud, "hud.signal.glow", "1");
    ecg_hud_config_apply_pair(&hud, "hud.scale", "1.25");

    ecg_surface_clear(&surface, ecg_color_make(0u, 0u, 0u));
    ecg_draw_hud(&surface, profiles, &hud);
}
```

## Key/value fields added in 1.2.0

```text
hud.states.clear
hud.state.add
hud.state.<NAME>
hud.color
hud.color.<NAME>
hud.gradient
hud.gradient.<NAME>
hud.glow_color
hud.glow_color.<NAME>
hud.profile.<NAME>

hud.scale
hud.scale.x
hud.scale.y
hud.active.scale.x
hud.active.scale.y
hud.overview.scale.x
hud.overview.scale.y

hud.signal.style
hud.signal.dotted
hud.signal.line_width
hud.signal.dot_on
hud.signal.dot_off
hud.signal.glow
hud.signal.glow_radius
hud.signal.glow_intensity

hud.grid.color
hud.axis.color
hud.frame.color
hud.window.color
hud.custom_text.color
hud.background_box.color
hud.overlay_box.color
```

All 1.1 position, visibility, text and box fields remain available.

## Build

```sh
make all
make demo
./build/ecg_demo
make test
make check
```

Default validation flags:

```sh
-std=c89 -pedantic -Wall -Wextra -Werror -Iinclude
```

The generated preview is:

```text
previews/ecg_hud_v1_2_demo.png
```
