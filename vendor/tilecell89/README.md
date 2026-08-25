# TileCell89

A fixed-capacity C89 atlas/grid/tile-cell and tile-map description library.

TileCell89 turns an opaque image request plus grid geometry into deterministic
source rectangles. It also supports named cells, fixed tile maps and extraction
of horizontal/vertical/diagonal strips suitable for animation consumers.

It does not load images or draw tiles. The image request can therefore be a
logical name resolved later by any asset system.
