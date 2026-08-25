# GeneralVideoConfigC89

Renderer/output presentation manager. It never owns a native window. It stores internal render resolution and maps that logical image into an output rectangle.

Scaling: native, fit/letterbox, stretch, integer, overscan, fixed 1x..16x. Filtering: nearest/linear. Also tracks aspect policy, output centering and vsync intent.

The renderer is supplied through `gvc89_provider`.

Protocol89: ISO C89, fixed/caller-owned state, no heap, no float/double, no explicit 64-bit types.
