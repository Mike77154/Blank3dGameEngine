# SpriteVerbs89

Semantic adapter between arbitrary DSL vocabularies and SpriteAsset89. A DSL emits verbs; SpriteVerbs89 maps aliases such as `show`, `sprite_play`, `image_index`, and `image_speed` to one sprite runtime.

The Aseprite adapter imports exported sprite-sheet JSON rectangles, per-frame duration, and frame tags into SpriteAsset89 clips. It accepts both hash/array-style exports by scanning the common frame objects.
