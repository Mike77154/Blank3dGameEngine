# Blank3D namespace cutover

The upstream SpritePlane89 package exports lower-case identifiers with the `sp89_` prefix.
Blank3D already vendors `shotpumpkin89`, which independently exports lower-case `sp89_*`
identifiers. Linking both unchanged creates an ABI/symbol collision (`sp89_init` is the first
visible conflict).

The Blank3D vendored SpritePlane89 copy therefore applies a mechanical lower-case API namespace
cutover:

- upstream `sp89_*` -> Blank3D vendor `sprpl89_*`
- uppercase `SP89_*` constants/macros are unchanged
- data layout, fixed-point representation, behavior, limits, and algorithms are unchanged

This cutover is limited to the vendored SpritePlane89 copy and its Blank3D adapters/tests. It does
not modify `shotpumpkin89` and does not alter SpritePlane89 semantics.
