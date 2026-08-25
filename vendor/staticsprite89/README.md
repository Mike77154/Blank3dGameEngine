# StaticSprite89

A tiny C89, no-heap, renderer-agnostic static-sprite descriptor.

StaticSprite89 does **not** decode images, touch a filesystem, create GPU state,
or decide whether coordinates are screen/world coordinates. It stores an image
request plus source rectangle, Q16.16 transform, tint, flags and user tag, then
exposes a read-only sample for a host renderer.

This makes it useful as the one-frame endpoint for higher layers such as image
sequencers, tile-cell animation, HUDs, billboards or engine-specific sprite
providers.

Build/test:

```sh
make test
```
