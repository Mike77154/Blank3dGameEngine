# SpriteAsset89 API notes

- `sa89_add_source`: registers a path/URI.
- `sa89_add_asset`: creates a logical sprite name.
- `sa89_add_frame`: references a source image/frame + rectangle + duration.
- `sa89_add_clip`: groups a contiguous frame range as forward/reverse/pingpong/non-looping animation.
- `sa89_player_*`: instance playback state.
- image provider: loading/decoding/cache boundary.
- render provider: drawing boundary.
- no renderer, filesystem, imgcc0 or DSL knowledge exists in the core.
