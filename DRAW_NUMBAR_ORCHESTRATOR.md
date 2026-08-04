# Blank3D v3.24.0 — `draw_numbar` Preset Orchestrator

## Purpose

`draw_numbar` is the generic HUD-meter operation used by a GFO actor. It is not
limited to health. The same operation can display ammunition, shields, stamina,
oxygen, boss phases, heat, fuel, reload progress, capture progress or any other
numeric binding exposed by the host.

The system deliberately separates three responsibilities:

```text
GFO operation                    INI orchestrator                 .bhud preset
when/owner                       where/data                       how it looks
-----------------------------    -----------------------------    -------------------------
draw_numbar("...ini")     ->    anchor/x/y/scale/z/bindings ->   GBar89/BVH visual recipe
```

The base `config/hud/gameplay.bighud` now contains the ECG only. The player GFO
calls `config/hud/gameplay.ini`, which instantiates the circular health meter,
the vertical segmented meter and the ammunition display. This prevents the ECG
from being duplicated and lets every meter be replaced independently.

## GFO operation

```gfo
*render
draw_mesh()
draw_numbar("config/hud/gameplay.ini")
```

The operation works from any GFO entity. The caller becomes the NumBar owner,
so `self.*` bindings resolve against that player, enemy, ally, boss or generic
actor.

Example for an enemy-owned meter:

```gfo
*render
draw_mesh()
draw_numbar("config/hud/examples/enemy_health.ini")
```

## Orchestrator INI

An INI may instantiate one or many meters:

```ini
[orchestrator]
enabled=true

[numbar player_health]
preset=presets/re5_radial_green_3d.bhud
space=screen
anchor=top_right
x=46
y=24
scale=100
z=20
bind.value=player.health
bind.min=zero
bind.max=player.health_max
bind.overlay=gameplay.threat
bind.overlay_min=zero
bind.overlay_max=100
visible_when=player.alive
```

Supported placement keys:

```ini
preset=path/to/style.bhud
enabled=true
space=screen
anchor=top_left|top_center|top_right|center|bottom_left|bottom_center|bottom_right
x=0
y=0
z=0
scale=100
scale_x=100
scale_y=100
canvas_width=0
canvas_height=0
```

`canvas_width` and `canvas_height` are optional overrides. Normally the natural
canvas declared by the preset is used.

Supported binding channels:

```ini
bind.value=...
bind.min=...
bind.max=...
bind.overlay=...
bind.overlay_min=...
bind.overlay_max=...
bind.mid=...
bind.segments=...
bind.state=...
bind.phase=...
bind.units=...
bind.layer_count=...
bind.layer_size=...
bind.value2=...
bind.visible=...
visible_when=...
```

Aliases such as `bind`, `min_bind`, `max_bind`, `unit_count_bind`,
`radial_phase_bind`, `reserve_bind` and `visible_bind` are also accepted.

## Preset contract

A visual preset declares a natural canvas and consumes only generic channels:

```bvh
hud reusable_style -=)
enabled true
canvas_width 108
canvas_height 108

node meter -=)
type bar
bind numbar.value
min_bind numbar.min
max_bind numbar.max
overlay_bind numbar.overlay
pos top_left + 0,0
size 108,108
kind radial_ring
...
(=-
```

The reusable channels visible inside a preset are:

```text
numbar.value             numbar.overlay
numbar.min               numbar.overlay_min
numbar.max               numbar.overlay_max
numbar.mid               numbar.segments
numbar.state             numbar.phase
numbar.units             numbar.layer_count
numbar.layer_size        numbar.value2
```

A preset must not contain an ECG node. ECG remains a separate base-HUD service.

## Runtime binding sources included in Blank3D

Actor-independent:

```text
player.health             player.health_max
player.alive              player.dead
gameplay.threat           ecg.bpm
damage.flash_ms           camera.first_person
weapon.muzzle_flash
```

Current GFO owner:

```text
self.id                   self.health
self.health_max           self.alive
self.dead                 self.weapon.loaded
self.weapon.capacity      self.weapon.reserve
```

Target/enemy convenience bindings (the current demo host maps the player to the nearest live enemy and an NPC to the player):

```text
target.health             target.health_max
target.alive              enemy.health
enemy.health_max          enemy.alive
boss.health               boss.health_max
boss.alive
```

Player weapon aliases:

```text
weapon.loaded
weapon.capacity
weapon.reserve
```

The resolver is a host callback. Future systems can add numeric sources without
changing GBar89, BigVaderHudder, the INI parser or the presets.

## Loading and lifetime

- No heap allocation is used.
- The INI and every referenced preset are parsed on the first request.
- Active GFO calls reuse cached instances; files are not reopened every frame.
- Each actor/preset instance owns independent lag, smoothing, easing, phase and
  state animation.
- When a GFO stops requesting an orchestrator, its fixed slots are released
  after the frame, allowing destroyed actors to return their HUD capacity.
- Instances are sorted by orchestrator `z`, not by preset-local node `z`.
- Nodes inside each preset keep their own local `z` order.

Fixed limits:

```c
B3D_NUMBAR_MAX_INSTANCES = 32
B3D_BIGHUD_MAX_NODES     = library-defined fixed capacity
```

## Main files

```text
src/blank3d_numbar.h
src/blank3d_numbar.c
objects/player.gfo
config/hud/gameplay.ini
config/hud/gameplay.bighud
config/hud/presets/*.bhud
config/hud/examples/*.ini
```
