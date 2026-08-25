# Module tree

```text
primitive2d_database89/
├── include/primitive2d_database89.h
└── src/
    ├── core/
    │   ├── p2d89_geom.c
    │   ├── p2d89_aliases.c
    │   ├── p2d89_registry.c
    │   └── p2d89_internal.h
    └── modules/
        ├── pixels/basic/
        ├── lines/basic/
        ├── lines/curves/
        ├── lines/incomplete/
        ├── bars/basic/
        ├── quadrilaterals/squares/
        ├── quadrilaterals/rectangles/
        ├── quadrilaterals/rhombi/
        ├── triangles/basic/
        ├── triangles/incomplete/
        ├── polygons/regular/
        ├── polygons/irregular/
        ├── polygons/weird/
        │   ├── concave/
        │   └── star_polygons.c
        ├── curves/round/
        ├── curves/organic/
        ├── stars/regular/
        ├── stars/sparks/
        ├── spirals/
        │   ├── basic/
        │   └── angular/
        ├── ornaments/
        │   ├── floral/
        │   └── geometric/
        ├── technical_symbols/
        │   ├── drafting/
        │   └── electrical/
        ├── map_symbols/
        │   ├── navigation/
        │   ├── terrain/
        │   └── infrastructure/
        ├── gizmos/
        │   ├── transform/
        │   ├── selection/
        │   └── nodes/
        ├── chevrons/basic/
        ├── brackets/basic/
        ├── symbols/basic/
        ├── arrows/basic/
        └── controls/basic/
```

There are 57 physical registry submodules. New families can be appended without changing the provider command ABI.


## v0.4.0 added branches

- `curves/mathematical/` — 23 shapes
- `spirals/extended/` — 10 shapes
- `ornaments/floral_extra/` — 13 shapes
- `ornaments/geometric_extra/` — 10 shapes
- `technical_symbols/mechanical/` — 20 shapes
- `technical_symbols/logic/` — 18 shapes
- `map_symbols/services/` — 18 shapes
- `gizmos/advanced/` — 18 shapes
- `polygons/weird_extra/` — 20 shapes
- `diagrams/flowchart/` — 24 shapes
- `diagrams/process/` — 18 shapes
- `connectors/basic/` — 18 shapes
- `connectors/markers/` — 18 shapes
- `patterns/hatch/` — 18 shapes
- `patterns/tiles/` — 18 shapes
- `graph_symbols/nodes/` — 20 shapes
- `graph_symbols/edges/` — 12 shapes
- `annotations/callouts/` — 20 shapes
- `weather_symbols/basic/` — 20 shapes
- `weather_symbols/fronts/` — 8 shapes
- `architecture/basic/` — 18 shapes
- `controls/widgets/` — 22 shapes
