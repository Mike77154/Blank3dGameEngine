# StaticSprite89 API

`ss89_set_image()` accepts a logical name or explicit path. Resolution is a host
responsibility. `ss89_set_source_rect()` is optional; leaving it disabled means
"use the full image". Transform values are Q16.16 and intentionally carry no
screen/world semantics.

`ss89_sample()` returns a view into the descriptor without allocating memory.
The returned `image_request` pointer remains owned by `SS89_StaticSprite`.
