# gscope89 INI recipe architecture v0.4

The rule remains deliberately small: **an INI can call other INIs**. C supplies parsing/rendering/providers; content and assembly stay outside the executable.

## Full chain

```text
HUD master INI
   |
   +-- provider policy INI
   +-- mask/telemetry component INIs
   +-- scope selector
   |      |
   |      +-- reticle catalog.ini
   |              |
   |              +-- formal preset.ini
   |                     |
   |                     +-- geometry component.ini
   |                            |
   |                            +-- optional shared fragment INIs
   |
   +-- zoom catalog -> zoom preset.ini
   +-- sway catalog -> sway preset.ini
   +-- animation catalog -> animation set.ini -> animation fragment INIs
```

Example formal reticle:

```ini
[recipe]
include=../components/038_svd_pso1_dragunov_geometry.ini

[preset]
id=38
name=svd_pso1_dragunov
family=historical
flags=pure_vector|historical|rangefinder|bdc
focal_plane=second
recommended_zoom_x100=800
calibration_zoom_x100=800
default_theme=black
shape_count=22
```

The geometry component can itself be an assembly:

```ini
[recipe]
include=../fragments/shared/some_shared_geometry.ini
```

or contain its own shapes directly.

## Catalog -> selection -> recipe

Selector:

```ini
[reticle]
catalog=catalog.ini
use=svd_pso1_dragunov
```

Catalog:

```ini
[presets]
svd_pso1_dragunov=presets/038_svd_pso1_dragunov.ini
```

Nothing in the renderer needs to know the meaning of `svd_pso1_dragunov`.

## Provider policy is also just a recipe

```ini
[recipe]
include=../providers/plug_and_play.ini

[scope]
catalog=../catalogs/scopes.ini
use=pso1
```

`plug_and_play.ini` can say:

```ini
[providers]
vector=auto:host
primitive=auto:host
paint=auto:host
zoom=auto:host
```

If `host` is registered and handles an operation, it owns that operation. If it does not exist or returns fallback, the bundled C89 implementation runs.

## Expansion

Adding preset 193 or 500 still needs no C geometry changes:

1. create its formal preset INI;
2. include whichever component/fragment INIs compose it;
3. add one line to the catalog;
4. select it by name.

A host can also provide the preset domain and source the same named preset from an external HUD/vector database without using the internal catalog at all.

## Legacy ABI

The original 192 enum IDs remain so old code keeps compiling. Runtime discovery is driven by catalogs/providers, not by the enum.

## HUD scope preset can select a formal vector reticle

The HUD-level scope recipe and the 192-reticle catalog are now connected by name. For example `config/presets/scopes/pso1.ini` contains:

```ini
[vector_reticle]
use=svd_pso1_dragunov
```

After the HUD master resolves `use=pso1`, the provider-aware facade can call:

```c
preset = gscb89_preset_from_recipe(&bundle, &recipe, "vector_reticle");
```

The semantic `gsniperhud89` crosshair fields remain available as a lightweight/legacy internal fallback, while the formal vector reticle is the expandable path for full geometry.

## Animation selection follows the same rule

```ini
[recipe]
select=animation

[animation]
catalog=../catalogs/animations.ini
use=reticle_default
```

The selected animation set can include `fire_scale.ini`, `aim_transition.ini`,
`damage_shake.ini`, or any future animation recipe. Inactive animation evaluates
to identity, so the default set preserves the golden-master idle output.
