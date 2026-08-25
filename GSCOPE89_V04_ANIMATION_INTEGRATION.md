# gscope89 v0.4 animation integration

Blank3D now vendors `gscope89_recipe_ini_v0_4` as the sole sniper-scope stack.
The v0.4 bundle keeps the v0.3 recipe/provider architecture and adds
`gscopeanim89`, provider ABI 2 animation routing, and golden-master validation.

## Runtime chain

```text
config/weapons/sniper.ini
  -> scope_recipe=config/scope/hud/sniper_re5_psg1.ini
     -> scope=re5_psg1
        -> vector_reticle=re5_psg1_game_scope
     -> zoom=sniper8x
     -> sway=sniper_default
     -> animation=reticle_default
        -> fire_scale.ini
        -> aim_transition.ini
        -> damage_shake.ini
```

The base reticle remains immutable catalog data. `gscopeanim89` samples a
temporary pose and transforms caller-owned scratch shapes immediately before
`gscopebundle89` emits them. No heap allocation is introduced.

## Blank3D event bridge

Blank3D supplies real gameplay edges to the animation recipe:

- entering the telescopic view -> `aim_enter`
- leaving the telescopic view -> `aim_exit`
- accepted player sniper shot -> `fire`
- damage received while scoped -> `damage`
- `g.frame_ms` -> `gsa89_tick()` (clamped to 1..1000 ms per engine frame)

`blank3d_sniper_trigger_event()` also exposes arbitrary recipe trigger names to
the host without coupling the animator to weapon/event enums.

## Rendering/provider route

`blank3d_sniper_emit()` uses `gscb89_emit_animated_preset()` whenever an
animation set is loaded. The animation domain follows the same provider rules
as the rest of gscope89:

```ini
[providers]
animation=auto:host
```

A provider can transform the shape batch and return `GPR89_HANDLED`, or return
`GPR89_FALLBACK` and let the fixed-point internal `gscopeanim89` transform run.
The existing paint/vector/primitive pipeline remains unchanged afterward.

## Golden-safe default

Blank3D selects `reticle_default`, which contains only event-driven clips.
Therefore idle geometry remains byte/pixel equivalent to the frozen pre-INI
reticle catalog. `reticle_lively` remains available in INI for an intentional
always-on breathing offset, but is not selected by default because Blank3D
already applies camera sway through `gsway89`.
