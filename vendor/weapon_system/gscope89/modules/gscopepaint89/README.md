# gscopepaint89

Renderer-neutral painter used by the rest of the scope suite.

- C89
- integers and Q16.16 fixed point only
- no dynamic allocation
- no renderer dependency
- configurable RGBA, per-command outline, thickness, layer and blend mode
- line, rectangle, triangle, circle, ellipse, arc, sprite, glyph, clip, scope mask and vignette commands

The engine provides one callback and converts commands to its own software/OpenGL/Direct3D renderer.
