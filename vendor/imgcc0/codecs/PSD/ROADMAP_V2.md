# ROADMAP V2

## Near-term hardening after v1.7

1. Better feather semantics than the current deterministic rect/bounds ramps
2. Knockout behavior beyond boolean parse/write
3. More corpus cases:
   - disabled vector mask + inverted fill
   - mixed raster mask + vector mask interactions
   - linked/unlinked fallback cases for old files without `lmgm` / `vmgm`
   - clipped groups with non-normal clipped blend keys under `clbl`
4. ZIP robustness polish:
   - broader interoperability corpus from external PSDs
   - explicit ZIP/ZIP+prediction mismatch diagnostics surfaced through higher-level APIs
5. Optional polish:
   - expose vector flattening tolerances in public compose options
   - add stricter diagnostics for malformed path record sequences

## Major v2 items

- PSB write
- 16/32 bpc write
- Lab / Indexed / Duotone write
- descriptor-rich tagged blocks
- editable type layers
- smart objects
- exact vector masks / vector masks complejas
- layer effects
- slices / timeline / vanishing point
- modern adjustment layers
