# TileCell89 API

`tc89_add_atlas()` computes rows/columns from image size, cell size, margins and
spacing. `tc89_atlas_rect_xy()` and `tc89_atlas_rect_index()` resolve cells.

Named cells are lightweight references to atlas coordinates. Maps store named
cell IDs in caller-owned fixed storage. `TC89_EMPTY_CELL` represents an empty
map slot.

`tc89_collect_strip()` is a convenience for constructing animation frame
rectangles from a grid without making TileCell89 itself a timeline/player.
