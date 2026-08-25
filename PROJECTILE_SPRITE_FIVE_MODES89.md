# Blank3D projectile sprite five-mode bridge

Blank3D projectile/world billboards can now obtain their animation from five
independent sprite-authoring backends. The weapon only declares that it wants a
billboard and which visual recipe it wants; gameplay/physics does not own image
loading or animation policy.

```text
weapon/projectile
      |
      v
Blank3DProjectileSprite89
      |
      +-- static    -> StaticSprite89
      +-- sequence  -> ImageSequencer89
      +-- renlist   -> RenList89 -> ImageSequencer89
      +-- tilecell  -> TileCell89 -> ImageSequencer89
      +-- gmstrip   -> GMSpritestrip89 -> SpriteAsset89
      |
      v
Blank3DImageAssets -> AssetRoute89 -> imgcc0 -> billboard renderer
```

The five vendor cores remain agnostic. `blank3d_projectile_sprite89` is the host
bridge that translates their sampled frame output into Blank3D image requests
and source rectangles.

## Common INI keys

All modes use the same presentation/effect layer:

```ini
projectile_visual_billboard=1
projectile_sprite_mode=<static|sequence|renlist|tilecell|gmstrip>
projectile_visual_loop=<once|forward|reverse|pingpong|hold>
projectile_visual_phase_step=7

projectile_visual_width_scale=1.0
projectile_visual_height_scale=1.0
projectile_visual_glow=1
projectile_visual_glow_scale=1.30
projectile_visual_glow_alpha=105
projectile_visual_core=1
projectile_visual_core_scale=0.72
projectile_visual_core_alpha=150
projectile_visual_light=1
projectile_visual_light_intensity=1.20
projectile_visual_light_radius=4.00
projectile_visual_light_r=255
projectile_visual_light_g=112
projectile_visual_light_b=32
```

Glow, hot core, dynamic light and billboard orientation are orthogonal to the
sprite source mode.

## 1. StaticSprite89

One logical image request, sampled forever as a single frame:

```ini
projectile_sprite_mode=static
projectile_visual_image=plasma_orb
projectile_visual_billboard=1
```

The request may be an AssetRoute89 logical name or an explicit path supported by
the existing Blank3D image registry.

## 2. ImageSequencer89

Use a loose-image/source-rect sequence recipe resolved as DATA through
AssetRoute89:

```ini
projectile_sprite_mode=sequence
projectile_visual_recipe=plasma_pulse
projectile_visual_loop=forward
```

Minimal `.iseq89` authoring syntax accepted by the host bridge:

```text
sequence pulse
loop forward
frame "plasma_001" 30
frame "plasma_002" 35
rect "shared_fx_sheet" 0 128 128 128 40
```

`frame` uses an opaque image request and duration in milliseconds. `rect` adds a
source rectangle from an image request. The sequence is compiled to a compact
fixed-capacity runtime clip; loose source files are not packed or copied.

## 3. RenList89

Use the vendored Ren'Py/ATL-like agnostic authoring parser:

```ini
projectile_sprite_mode=renlist
projectile_visual_recipe=flame_fx
projectile_visual_animation=projectile
projectile_visual_clip=burn
```

Example `.rnlist`:

```text
image projectile burn:
    source sequence
    billboard view
    blend add
    fps 30
    "fire_001"
    pause 0.033
    frame "fire_002" for 45
    "fire_003"
    repeat
```

RenList89 does not interpret host-specific properties such as `billboard` or
`blend`; it keeps them as authoring metadata. The adapter converts the selected
animation/clip to ImageSequencer89 timing data. If no animation name is supplied,
the first parsed animation is used. The default clip name is `default`.

## 4. TileCell89 atlas/grid/cell mode

Use one image as an atlas and derive temporal frames from cells:

```ini
projectile_sprite_mode=tilecell
projectile_visual_image=flame_atlas
projectile_visual_frame_width=128
projectile_visual_frame_height=128
projectile_visual_columns=8
projectile_visual_rows=7
projectile_visual_frame_count=50
projectile_visual_frame_ms=33
projectile_visual_margin_x=0
projectile_visual_margin_y=0
projectile_visual_spacing_x=0
projectile_visual_spacing_y=0
projectile_visual_start_x=0
projectile_visual_start_y=0
projectile_visual_step_x=1
projectile_visual_step_y=0
```

The actual image dimensions are read through Blank3DImageAssets. TileCell89 owns
the atlas/grid cell geometry. With the default `(step_x=1, step_y=0)`, sampling
continues row-major through the atlas. Custom start/step values can select a
vertical or diagonal cell walk. The standalone TileCell89 vendor also retains
its named-cell, strip and fixed tile-map APIs for other consumers; the projectile
bridge currently consumes its atlas/cell geometry as a temporal clip.

The current flamethrower FireLoop is configured through this mode, so its
50-frame 128x128 atlas is now a real TileCell89 client instead of a private
hard-coded atlas implementation.

## 5. GMSpritestrip89

GameMaker-style horizontal strip names can infer their subimage count directly
from the routed filename:

```text
fireball_strip8.png
spark_strip12.png
```

Weapon recipe:

```ini
projectile_sprite_mode=gmstrip
projectile_visual_image=fireball_strip8
projectile_visual_frame_ms=33
projectile_visual_loop=forward
```

The bridge supplies GMSpritestrip89 with an image provider backed by
Blank3DImageAssets. `gmss89_define_strip_auto()` parses `_stripN`, obtains the
actual image width/height through the provider, and asks SpriteAsset89 to create
the subimages. If the routed filename has no `_stripN`,
`projectile_visual_frame_count` can provide an explicit fallback count.

GMSpritestrip89 is vendored intact under `vendor/gmspritestrip89`, including its
upstream nested SpriteAsset89 dependency. Blank3D's shared top-level
SpriteAsset89 core was upgraded from v0.1.0 to v0.1.2 because v0.1.2 contains
the GameMaker-strip API required by GMSpritestrip89; Blank3D's existing adapters
were retained.

## Asset routing

Image requests continue to use the existing image route/registry. Recipe files
are resolved as DATA requests. Blank3D registers data roots for sprite recipes,
including:

```text
config/weapons/visuals
config/sprites
assets/sprites
game/sprites
```

This allows a weapon to say:

```ini
projectile_visual_recipe=flame_fx
projectile_visual_image=flame_atlas
```

without embedding filesystem paths in the gameplay module.

## Runtime/fallback rules

The compiled per-weapon clip stores at most 128 sampled frames and uses fixed
capacity. Large parser/vendor contexts are reusable static scratch storage and
are not retained per projectile. No heap ownership was added to the bridge.

If a recipe cannot be resolved/compiled or an image cannot be loaded, the
projectile billboard sample fails cleanly and Blank3D keeps the existing
procedural projectile-mesh fallback. Physics, collision and `expandiblefire89`
remain authoritative and are not replaced by sprite animation.

## Validation

The integration gate exercises all five modes with actual routed assets:

```text
StaticSprite89       -> image request                    PASS
ImageSequencer89     -> loose timed images               PASS
RenList89            -> .rnlist -> ImageSequencer89      PASS
TileCell89           -> real atlas source rectangles     PASS
GMSpritestrip89      -> _strip8 inference/subimages      PASS
```

Run:

```sh
make test-sprite-microvendors89
make test-projectile-sprite-modes89
make test-spriteasset89-vendor
make test-sprite-runtime89
make test-flamethrower
make test-image-stack
make test-muzzle-image-pipeline
make test-casing-physics
make syntax-check
```
