# SpriteAsset89 + SpriteVerbs89 + AssetRoute89 integration

Blank3D now vendors the three sprite/asset libraries as separate, provider-driven
modules and joins them through `Blank3DSpriteRuntime89`.

## Runtime graph

```text
RPYL / DDSL2 actions
        |
        v
   SpriteVerbs89
        |
        v
   SpriteAsset89 <-------- RenList89 / Aseprite JSON importers
        |
        +---- lazy logical-name lookup ----> AssetRoute89
        |                                      |
        |                              POSIX / Win32 provider
        |                                      |
        +--------------------------------------+
        |
        v
Blank3DImageAssets ----> imgcc0 ----> Blank3D OpenGL image backend
        |
        +---- existing SpritePlane89 / HUD / muzzle image consumers
```

The important boundary is that none of the three vendored cores owns the OS,
OpenGL, or Blank3D gameplay policy. `AssetRoute89` gets filesystem access from a
provider. `SpriteAsset89` gets image/render services from providers.
`SpriteVerbs89` only translates semantic verbs into `SpriteAsset89` operations.

## Vendored layout

```text
vendor/
  assetroute89/
  spriteasset89/
  spriteverbs89/
```

Their upstream examples, docs, adapters and tests are retained. Blank3D's root
Makefile owns the final dependency graph, so a vendor does not need to assume
that another vendor is a sibling directory.

## Automatic asset roots

At startup `Blank3DSpriteRuntime89` tries these roots when they exist:

```text
images
assets/images
game/images
config/images

audio
assets/audio
game/audio
config/audio
```

They are recursive. Additional roots can be declared from RPYL:

```text
asset_root image "config/crosshair/assets" 1
asset_root image "game/characters" 1
asset_root audio "game/audio" 1
asset_scan
```

`image`/`images`, `sprite`/`sprites`, `audio`, `sound`, `music`, and `data`
are accepted kind words by the runtime helper. Sprite lazy-loading currently
requests the `image` kind.

## Ren'Py-like logical names

AssetRoute89 indexes recognized files and strips the extension for logical
lookup. Therefore, after scanning `config/crosshair/assets`, this:

```text
sprite_show hud_face ring
```

can resolve `ring` to `config/crosshair/assets/ring.tga` without a hard-coded
path in the verb. The route is then materialized lazily as a SpriteAsset89
static asset and decoded through Blank3D's existing imgcc0 image registry.

Explicit aliases are also supported:

```text
asset_alias image player_portrait "game/ui/portraits/player.png"
sprite_show portrait player_portrait
```

Explicit aliases are normalized case-insensitively by the integrated
AssetRoute89 core.

## SpriteAsset declarations

### One static image

```text
sprite_static logo "game/ui/logo.png" 1000
sprite_show title logo
sprite_pos title 64 32
```

### Sprite sheet / GameMaker-style grid

```text
sprite_grid hero run "game/images/hero_run.png" 64 64 8 4 80 loop
sprite_play player_sprite hero:run
sprite_pos player_sprite 320 180
```

Arguments are:

```text
sprite_grid <asset> <clip> <request> <frame_w> <frame_h>
            <frame_count> <columns> <duration_ms> [loop]
```

Loop modes accepted by the bridge are `loop` (forward), `none`/`once`,
`reverse`, and `pingpong`/`ping-pong`.

### RenList89 / Ren'Py-like list

The vendored parser accepts declarations such as:

```text
image hero idle:
    "assets/sprites/hero_idle_0.png"
    pause 0.10
    "assets/sprites/hero_idle_1.png"
    pause 0.15
    repeat
```

Import from RPYL with:

```text
sprite_renlist "game/sprites/hero.sprite89"
sprite_play hero_view hero:idle
```

### Aseprite JSON

```text
sprite_aseprite hero "game/sprites/hero.json"
sprite_play hero_view hero:run
```

An optional third argument overrides the sheet path stored in the JSON.

## SpriteVerbs89 vocabulary

Blank3D accepts both the canonical verbs and aliases exported by
SpriteVerbs89:

```text
show / image_show / sprite_show
hide / image_hide / sprite_hide
play / animate / sprite_play / play_sprite
stop / sprite_stop
sprite / image / sprite_set / sprite_index
frame / sprite_frame / image_index
speed / sprite_speed / image_speed
reset / sprite_reset
flip_x / sprite_flip_x
flip_y / sprite_flip_y
x / sprite_x
y / sprite_y
pos / position / sprite_pos / sprite_position
```

The Blank3D integration adds position verbs to the vendored v0.1 vocabulary so
2D instances are not locked to `(0,0)`.

Generic explicit dispatch is also available:

```text
spriteverb hero_view sprite_play hero:run
spriteverb hero_view sprite_pos 320 180
```

## DDSL2 bridge

Blank3D's DDSL2 VM reports actions to the host as a verb plus one value string.
The bridge recognizes any SpriteVerbs89 verb and interprets the first token of
the value as the target, preserving the rest as the argument. For example:

```text
If key_pressed H then sprite_show=portrait player_portrait
If key_pressed J then sprite_hide=portrait
If key_pressed K then image_speed=hero_view 0.5
```

The upstream `spriteverbs89_ddsl2` adapter is still vendored and compiled, but
Blank3D uses this host-action bridge because its existing DDSL2 integration is
VM/event based rather than a private SpriteVerbs registry.

## Existing image consumers benefit from AssetRoute89

`Blank3DImageAssets` can now attach an `AssetRoute89` instance. Resolution first
tries the route service and then falls back to the previous path/index behavior.
That means the existing SpritePlane89, HUD/crosshair and muzzle-image paths keep
their previous behavior while gaining logical-name routing when a route table is
present.

Example world plane after the root is scanned:

```text
spriteplane ring pos 0 2 -6 size 2 2 mode camera life 0 blend alpha depth 1
```

## Rendering

`Blank3DSpriteRuntime89` renders SpriteAsset89 frames through a host callback.
The Win32/OpenGL host uses `blank3d_image_gl_draw_subrect()` so grid/Aseprite
frame rectangles can address one uploaded texture without allocating a new
texture per frame. Overlay setup/restore is isolated by:

```c
blank3d_image_gl_begin_overlay(width, height);
blank3d_sprite_runtime89_render(&g.sprite_runtime);
blank3d_image_gl_end_overlay();
```

## Memory / protocol

The integration keeps the project rules:

- C89 source.
- Fixed-capacity registries.
- No heap introduced by the bridge.
- No `malloc`, `realloc`, or `free` in the new Blank3D bridge.
- Q16.16 sprite speed through SpriteVerbs89.
- Caller/static owned import buffer for RenList/Aseprite text.
- POSIX/Win32 filesystem access remains behind AssetRoute89 providers.

## Validation

Run:

```sh
make test-spriteasset89-vendor
make test-spriteverbs89-vendor
make test-assetroute89-vendor
make test-sprite-runtime89
make test-image-stack
make test-muzzle-image-pipeline
make syntax-check
make syntax-check-sprite-runtime89-win32
```

`test-sprite-runtime89` exercises the complete chain:

```text
logical name "ring"
 -> AssetRoute89 discovery
 -> SpriteVerbs89 lazy request
 -> SpriteAsset89 asset/player
 -> Blank3DImageAssets
 -> imgcc0 decode
 -> render-host callback
```

## Current boundary

AssetRoute89 can already index image and audio roots. The sprite/image route is
connected to Blank3D's live image pipeline. File-audio routing is deliberately
not forced into the current audio runtime yet: Blank3D's present audio stack has
its own synth/bus architecture, so an audio-file decoder/player provider should
be attached as a separate integration rather than making AssetRoute89 own audio
playback policy.
