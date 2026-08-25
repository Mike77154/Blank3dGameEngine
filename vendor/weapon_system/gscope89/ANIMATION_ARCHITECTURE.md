# gscopeanim89 v0.4

`gscopeanim89` adds event-driven animation without mutating the preset catalog.
A preset remains golden-master data; an animation samples a temporary pose and
transforms caller-owned scratch shapes immediately before rendering.

## Same recipe pattern

HUD master:

```ini
[recipe]
select=animation

[animation]
catalog=../catalogs/animations.ini
use=reticle_default
```

Catalog:

```ini
[presets]
reticle_default=../animations/reticle_default.ini
fire_only=../animations/fire_scale.ini
reticle_lively=../animations/reticle_lively.ini
```

Animation set:

```ini
[recipe]
include=fire_scale.ini
include=aim_transition.ini
include=damage_shake.ini
```

A clip and channels:

```ini
[animation.fire_kick]
trigger=fire
mode=oneshot
duration_ms=120

[channel.fire_expand]
clip=fire_kick
target=root
property=scale
from=1000
to=1140
start_ms=0
duration_ms=35
ease=out_quad

[channel.fire_return]
clip=fire_kick
target=root
property=scale
from=1140
to=1000
start_ms=35
duration_ms=85
ease=in_quad
```

## Runtime

```c
gsa89_ctx anim;
gsa89_pose pose;
gsv89_shape scratch[256];

gsa89_load_doc(&anim, &hud_recipe);
gsa89_trigger(&anim, "fire");
gsa89_tick(&anim, dt_ms);
gsa89_sample(&anim, &pose);

gscb89_emit_animated_preset(&bundle, &painter, preset, 0,
                             &pose, scratch, 256, 255);
```

Current animatable properties:

- uniform `scale` (`1000` = identity);
- `offset_x` / `offset_y` in normalized reticle units (`10000` = radius);
- `alpha` (`1000` = identity).

Targets can be `root`/`all` or a vector part such as `part:center`,
`part:posts`, `part:bdc`, `part:wind`, `part:labels`, etc.

Current easing modes: `linear`, `in_quad`, `out_quad`, `in_out_quad`, `step`.
Current clip modes: `oneshot`, `loop`.

## Golden safety

Inactive clips contribute identity. `reticle_default` contains only event-driven
clips (`fire`, `aim_enter`, `aim_exit`, `damage`), so simply enabling the animation
system does not change any idle golden-master pixels. `reticle_lively` additionally
includes `breath_sway.ini` and intentionally moves the idle reticle.

## Provider route

Animation is provider domain 11 in provider ABI 2:

```ini
[providers]
animation=auto:host
```

A host may transform the shape batch and return `GPR89_HANDLED`, or return
`GPR89_FALLBACK` and let `gscopeanim89` perform the fixed-point transform.
