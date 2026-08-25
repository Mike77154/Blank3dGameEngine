# Extending

1. Append a stable shape ID to `P2D89_ShapeId`; never renumber old IDs.
2. Put the recipe in the narrowest physical leaf (`map_symbols/terrain`, `gizmos/nodes`, etc.).
3. Add its `P2D89_ShapeInfo` row beside the emitter.
4. Reuse the common path IR and fixed-point helpers; do not add backend calls.
5. If the family is new, add a new `P2D89_ModuleId`, declare its `P2D89_Submodule`, and append the descriptor to the core registry.
6. Prefer a generator when a family is mathematical (`regular_polygon`, `star`, `star_polygon`, `spiral`) and keep named presets as convenient database entries.
7. Run `make test`, strict GCC/Clang builds, and the legacy stream comparison before release.

Adding shapes does not change the provider ABI.
