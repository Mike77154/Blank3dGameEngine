CC ?= cc
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Werror -Iinclude -Isrc/core

SOURCES = \
  src/core/p2d89_geom.c \
  src/core/p2d89_aliases.c \
  src/core/p2d89_registry.c \
  src/modules/pixels/basic/pixels_basic.c \
  src/modules/lines/basic/lines_basic.c \
  src/modules/lines/curves/lines_curves.c \
  src/modules/quadrilaterals/squares/squares.c \
  src/modules/quadrilaterals/rectangles/rectangles.c \
  src/modules/triangles/basic/triangles.c \
  src/modules/triangles/incomplete/triangle_incomplete.c \
  src/modules/quadrilaterals/rhombi/rhombi.c \
  src/modules/polygons/regular/regular_polygons.c \
  src/modules/curves/round/round_shapes.c \
  src/modules/curves/organic/organic_shapes.c \
  src/modules/stars/regular/regular_stars.c \
  src/modules/stars/sparks/sparks.c \
  src/modules/chevrons/basic/chevrons.c \
  src/modules/brackets/basic/brackets.c \
  src/modules/symbols/basic/symbols.c \
  src/modules/bars/basic/bars_basic.c \
  src/modules/arrows/basic/arrows.c \
  src/modules/controls/basic/controls.c \
  src/modules/lines/incomplete/incomplete_figures.c \
  src/modules/polygons/irregular/irregular_polygons.c \
  src/modules/spirals/basic/spirals_basic.c \
  src/modules/spirals/angular/spirals_angular.c \
  src/modules/ornaments/floral/ornaments_floral.c \
  src/modules/ornaments/geometric/ornaments_geometric.c \
  src/modules/technical_symbols/drafting/technical_drafting.c \
  src/modules/technical_symbols/electrical/technical_electrical.c \
  src/modules/map_symbols/navigation/map_navigation.c \
  src/modules/map_symbols/terrain/map_terrain.c \
  src/modules/map_symbols/infrastructure/map_infrastructure.c \
  src/modules/gizmos/transform/gizmos_transform.c \
  src/modules/gizmos/selection/gizmos_selection.c \
  src/modules/gizmos/nodes/gizmos_nodes.c \
  src/modules/polygons/weird/concave/weird_concave.c \
  src/modules/polygons/weird/star_polygons.c \
  src/modules/curves/mathematical/curves_mathematical.c \
  src/modules/spirals/extended/spirals_extended.c \
  src/modules/ornaments/floral_extra/ornaments_floral_extra.c \
  src/modules/ornaments/geometric_extra/ornaments_geometric_extra.c \
  src/modules/technical_symbols/mechanical/technical_symbols_mechanical.c \
  src/modules/technical_symbols/logic/technical_symbols_logic.c \
  src/modules/map_symbols/services/map_symbols_services.c \
  src/modules/gizmos/advanced/gizmos_advanced.c \
  src/modules/polygons/weird_extra/polygons_weird_extra.c \
  src/modules/diagrams/flowchart/diagrams_flowchart.c \
  src/modules/diagrams/process/diagrams_process.c \
  src/modules/connectors/basic/connectors_basic.c \
  src/modules/connectors/markers/connectors_markers.c \
  src/modules/patterns/hatch/patterns_hatch.c \
  src/modules/patterns/tiles/patterns_tiles.c \
  src/modules/graph_symbols/nodes/graph_symbols_nodes.c \
  src/modules/graph_symbols/edges/graph_symbols_edges.c \
  src/modules/annotations/callouts/annotations_callouts.c \
  src/modules/weather_symbols/basic/weather_symbols_basic.c \
  src/modules/weather_symbols/fronts/weather_symbols_fronts.c \
  src/modules/architecture/basic/architecture_basic.c \
  src/modules/controls/widgets/controls_widgets.c

all: test_modular list_modules

test_modular: $(SOURCES) tests/test_modular.c
	$(CC) $(CFLAGS) $(SOURCES) tests/test_modular.c -o $@

list_modules: $(SOURCES) examples/list_modules.c
	$(CC) $(CFLAGS) $(SOURCES) examples/list_modules.c -o $@

test: test_modular
	./test_modular

clean:
	rm -f test_modular list_modules
