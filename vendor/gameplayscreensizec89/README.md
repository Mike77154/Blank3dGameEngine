# GameplayScreenSizeC89

Engine-agnostic logical gameplay screen manager. It owns CameraSizeDraw, SceneScreenSize, logical camera position and optional scene clamping. It has no renderer or OS dependency.

The engine/camera implementation is supplied through `gpss89_provider`.

Protocol89: ISO C89, fixed/caller-owned state, no heap, no float/double, no explicit 64-bit types.
