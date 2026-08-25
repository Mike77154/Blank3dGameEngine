# Runtime reticle recipes

The reticle catalog is no longer a compiled C table.

```text
active.ini
   -> catalog.ini
      -> presets/<formal preset>.ini
         -> components/<geometry component>.ini
            -> optional fragments/shared/*.ini
```

- `active.ini` selects a named preset.
- `catalog.ini` maps names to formal preset recipes.
- `presets/` contains identity, family, focal plane, zoom/theme metadata and declared shape count.
- `components/` contains the geometry assembly for each formal preset.
- `fragments/` contains reusable vector pieces; v0.3 deduplicates 21 identical geometry groups used by 54 presets.

All 192 migrated presets use `[recipe] include=...`. Included shapes are appended in order, so components can be split into finer common primitives without touching the C loader.

The current runtime result is still 192 presets / 3,953 shapes / parity hash `15957882094601755844`.
