# Blank3D Base HUD: modern vector ammo + active reload

Version: v3.27.6

## Base ammo HUD

The gameplay base recipe `config/hud/presets/ammo_cartridges_gold.bhud` now uses
GProj2D89 cartridge geometry as the visible ammo language. GBar remains the slot,
layout and fill-state compositor. Segment/background fill is transparent so the
GProj cartridge itself is the dominant visual.

Loaded rounds use the full GProj silhouette. Missing rounds retain the same
geometry with the recipe's `color_unit_empty` ghost treatment. Reserve remains a
small numeric secondary readout.

## Active reload

The HUD does not calculate reload timing. The numbar resolver reads the existing
GWeapon89 user/profile state:

- `reload_active`
- `reload_elapsed_ms`
- resolved `reload_ms`
- resolved active-reload window start/end
- `active_reload_result`

Gameplay bindings expose these to the ammo recipe. The ammo node opts in with
`active_reload_visualizer true`. During reload, the HUD draws a muted timing rail,
the resolved good window, and a miniature GProj cartridge as the progress cursor.
Success/failure only affects feedback color; timing authority remains GWeapon89.

## Important fix found by visual QA

`numbar.state` was initially used to transport reload-active state. GBar89 treats
state value 1 as its LOW state and therefore selected the default yellow LOW fill.
This produced yellow segment blocks behind otherwise-correct GProj cartridges.
Reload telemetry was moved to neutral generic channels (`layer_count` for active,
`layer_size` for result) that this recipe does not bind into GBar layers.

## Validation

- `make test-gproj-bvhud-visual` PASS
- `make test-bighud` PASS
- `make syntax-check` PASS
- Visual fixture: `tests/gproj_bvhud_visual_modern.png`
