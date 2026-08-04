# FCASING89 integration notes

FCASING89 is intentionally shooter-agnostic. It does not know whether the shot came from the player, an ally, an enemy, a turret, a cutscene weapon, or a script. The caller sends a spawn event containing:

- origin
- forward/right/up basis
- profile id
- visual importance
- optional flags

The module decides simulation versus fake visuals from distance and budget, not faction or actor class.

## Recommended call order

```c
fcasing89_begin_frame(&casings);

/* Any weapon system may call this. */
fcasing89_emit(&casings, &spawn);

fcasing89_update(&casings, dt_ms);

render_count = fcasing89_collect_render_items(&casings, render_items, max_items);
while (fcasing89_pop_event(&casings, &event)) {
    /* Bounce audio hooks live here. */
}
```

## Runtime budgets

Suggested defaults for a PS1-ish / software-leaning C89 runtime:

- max active casings: 96
- max simulated: 48
- max fake: 48
- max spawned per frame: 8 to 12
- update only Q8 integer math
- no rigidbody per casing
- freeze/sleep casings after 1 or 2 bounces
- recycle the oldest/least important/farthest casing if full

## Renderer hook

Each render item gives:

- position
- rotation
- alpha
- render_model_id
- profile_id
- state

The renderer can map `render_model_id` to a tiny mesh, billboard, atlas sprite, or instanced draw path.

## Audio hook

Bounce sounds are emitted as events instead of direct callbacks so the module stays standalone. The audio system can decide volume, pan, priority, and whether to drop quiet events.

## Why no actor type?

A casing is a visual consequence of a weapon socket, not a gameplay object. If the module knows `player`, `ally`, or `enemy`, it becomes harder to reuse in demos, cutscenes, turrets, replay systems, network prediction, and editor previews. Use `importance` and camera distance instead.


## Optional provider call order

Bind once after `fcasing89_init`:

```c
fcasing89_provider_init(&provider);
/* Assign user, mask, and callbacks. */
fcasing89_set_provider(&casings, &provider);
```

During `fcasing89_update`, simulated casings use this order:

1. provider gravity or internal profile gravity
2. provider rotate or internal `rotation + spin_delta`
3. provider move or internal `position + velocity_delta`
4. provider collision or internal `floor_y` collision

Scale is resolved at spawn and copied to each render item. Each provider
callback can return zero to use the built-in implementation for that request.

See `provider_mode.md` for the complete callback contract.
