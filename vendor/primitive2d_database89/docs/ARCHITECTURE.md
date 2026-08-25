# Architecture

`core` owns the provider ABI, path-command IR, fixed-point geometry helpers, lookup and dispatch. It does not own catalog recipes.

Each leaf submodule owns:

- its `P2D89_ShapeInfo[]` metadata rows;
- one emitter that converts its named recipes to the common command IR;
- no renderer/backend dependency.

The stable provider IR remains:

`POINT / MOVE / LINE / QUAD / CUBIC / CLOSE`

Compound symbols may emit multiple subpaths. Rendering policy (stroke/fill, cap/join, even-odd vs nonzero) stays metadata rather than becoming OpenGL/GDI/SVG calls.

v0.3.0 added semantic flags for `SELF_INTERSECTING`, `COMPOUND_SYMBOL`, and `DIRECTIONAL`. v0.4.0 adds `REPEATABLE_TILE`, `MARKER`, `DIAGRAM_NODE`, and `EDITOR_HELPER`. This lets a backend, editor or DSL inspect intended use without learning the recipe implementation.

## Star polygons vs radial stars

`p2d89_emit_star(points, inner_radius, rotation, provider)` alternates outer and inner radii.

`p2d89_emit_star_polygon(vertices, step, rotation, provider)` traverses a regular vertex ring by a fixed skip step: `{5/2}`, `{7/3}`, `{17/5}`, etc. These are true self-intersecting star polygons and are catalogued under `polygons/weird_star`.

## v0.4 taxonomy principle

The database now separates *geometry vocabulary* from *placement/composition semantics*. A marker, repeatable hatch tile, diagram node, editor helper, or weather/front segment is still emitted through the same POINT/MOVE/LINE/QUAD/CUBIC/CLOSE ABI. New flags describe intended reuse without adding renderer opcodes.
