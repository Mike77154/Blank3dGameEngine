# BigTIFF SubIFD tree notes

This branch adds a real SubIFD tree writer/parser for BigTIFF.

## What it writes

- root images stay in the main IFD chain
- child images are referenced through TIFF tag 330 (`SubIFDs`)
- child IFDs are **not** linked into the main `NextIFD` chain
- when there are multiple child nodes, the writer stores an array of child IFD offsets
- when there is one child node, the writer stores the single offset inline in the tag value field

## Suggested uses

- reduced-resolution pyramids (`NewSubfileType = 1` on the reduced nodes)
- related layers that should hang off a parent image without becoming top-level pages

## Public API

- `tifx_parse_bigtiff_node_memory()`
- `tifx_bigtiff_subifd_count_memory()`
- `tifx_write_bigtiff_tree_buffer_size()`
- `tifx_write_bigtiff_tree_memory()`

## Limits

- tree traversal is explicit by index path
- maximum depth is `TIFX_MAX_SUBIFD_DEPTH`
- maximum children per node is `TIFX_MAX_SUBIFDS`
- writer emits tree-style SubIFDs only
