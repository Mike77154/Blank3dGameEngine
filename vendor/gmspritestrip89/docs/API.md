# API notes

## Definition

- `gmss89_define_strip(...)` — explicit GameMaker strip registration
- `gmss89_define_strip_auto(...)` — infer `N` from `_stripN`
- `gmss89_define_strip_from_path(...)` — infer clean asset name + `N` from path basename

## Naming

Input:

- `spr_x_walk_strip14.png`

Output logical asset:

- `spr_x_walk`

## Providers

The wrapper does not own decoding or rendering. It forwards to SpriteAsset89 providers:

- `GMSS89_ImageProvider`
- `GMSS89_RenderProvider`
