# Perception INI Configuration

Blank3D loads sensory configuration automatically from the INI that already
belongs to each enemy archetype:

```text
config/entities/zombie.ini
config/entities/gunner_enemy.ini
config/entities/hopper_enemy.ini
config/entities/dive_enemy.ini
config/entities/air_lunger_enemy.ini
config/entities/ground_lancer_enemy.ini
```

## Compact section

```ini
[perception]
truth_profile=strict
range=30
eye_shape=cone
horizontal_fov=100
vertical_fov=70
hearing_range=24
require_line_of_sight=true
```

- `truth_profile`: `retro`, `active`, `proximity` or `strict`.
- `range`: sets both `GEDER truth_range` and NPC Eyes `view_range`.
- `eye_shape`: `cone`, `sphere`, `box` or `frustum`.
- `horizontal_fov` and `vertical_fov`: eye volume in degrees.
- `hearing_range`: EnlightenerAI hearing radius.
- `require_line_of_sight`: enables the CCS/SICOL world raycast.

## Split sections

Use this form when truth acceptance and visual range must differ:

```ini
[truth]
profile=strict
range=40

[eyes]
shape=cone
range=30
horizontal_fov=100
vertical_fov=70
require_line_of_sight=true

[hearing]
range=24
```

## Spawn syntax

The normal spawn stays small:

```text
zombie 1 pos -8 0 24 hp 30
```

The engine derives `config/entities/zombie.ini` from the archetype. An instance
may override the automatic file:

```text
zombie 1 pos -8 0 24 hp 30 perceptionini config/perception/blind_zombie.ini
```

Legacy inline fields remain accepted after the INI is loaded, so they act as
last-write overrides rather than the primary configuration route.

## C89 implementation

```text
src/blank3d_perception_ini.h
src/blank3d_perception_ini.c
tests/test_perception_ini.c
```

The loader uses fixed-size line buffers, caller-owned state and no dynamic
allocation.
