# DSL bridge contract

A DSL does not decode a PNG and does not parse Aseprite. It calls `sv89_execute(target, verb, argument)`.

Examples:

- DDSL2-like: target=`player`, verb=`sprite_play`, arg=`hero:run`
- GameMaker-like: target=`player`, verb=`image_index`, arg=`3`
- RenPy-like: target=`eileen`, verb=`show`, arg=`eileen happy`

Aseprite JSON becomes SpriteAsset89 data at content-load time; tags become clip names.
