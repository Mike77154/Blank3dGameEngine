# Reticle fragments

Every formal reticle under `../presets/` is now an INI assembly. The preset file stores identity/metadata and includes one or more geometry/component INIs.

`shared/` contains geometry fragments that are byte-equivalent across multiple presets. In v0.3, 21 shared fragments are reused by 54 of the 192 migrated presets. Unique geometry remains in `../components/` and may be decomposed further later without changing the loader.

The loader appends included `[shape...]` sections in include order, so a recipe can be assembled from arbitrary pieces:

```ini
[recipe]
include=../fragments/common/fine_center.ini
include=../fragments/common/mil_ticks.ini
include=../fragments/common/bdc_ladder.ini

[preset]
name=my_new_scope
shape_count=37
```
