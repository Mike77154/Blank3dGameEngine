# Blank3D SpriteAsset89 / SpriteVerbs89 / AssetRoute89 examples.
# Documentation only: startup.rpy does not include this file automatically.

label spriteasset89_examples:
    # Discover a directory once. Logical names are extensionless.
    asset_root image "config/crosshair/assets" 1
    asset_scan

    # Ren'Py-like lazy logical image: "ring" resolves to ring.tga.
    sprite_show hud_face ring
    sprite_pos hud_face 48 48

    # Direct path declaration.
    sprite_static title_card "config/crosshair/assets/bullseye.tga" 1000
    sprite_show title title_card
    sprite_pos title 160 80

    # Generic semantic dispatcher; aliases are resolved by SpriteVerbs89.
    spriteverb title image_speed 0.5
    spriteverb title flip_x

    # Existing SpritePlane89 also benefits from the route-aware image registry.
    spriteplane ring pos 0 2 -6 size 2 2 mode camera life 0 blend alpha depth 1

    # GameMaker-like sheet example (requires an actual sheet at this path):
    # sprite_grid hero run "game/images/hero_run.png" 64 64 8 4 80 loop
    # sprite_play hero_view hero:run
    # sprite_pos hero_view 320 180

    # RenList89 / Aseprite imports:
    # sprite_renlist "game/sprites/hero.sprite89"
    # sprite_aseprite hero "game/sprites/hero.json"
