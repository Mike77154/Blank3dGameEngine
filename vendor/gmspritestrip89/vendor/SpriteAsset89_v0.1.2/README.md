# SpriteAsset89

C89, fixed-capacity, no-heap sprite asset/runtime registry. It does not decode images and does not own a renderer. Both are providers.

Core responsibilities: logical sprite assets, sources, frames, clips, playback and renderer/image provider contracts.

Adapters include a caller-buffer-owned imgcc0 0.6 provider and a reference RGBA8 software blitter.
