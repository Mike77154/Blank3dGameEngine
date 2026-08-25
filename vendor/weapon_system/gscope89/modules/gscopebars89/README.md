# gscopebars89

Bridge from arbitrary engine values to scope HUD bars.

The engine can provide channels as an array or callback. Included render modes:
linear fill, segmented fill, arc meter, tick meter and moving marker. Every bar has configurable foreground, background, outline, layer, blend mode and normalized placement.

The library owns no gameplay state and does not depend on a specific health/stamina/bar implementation.

## Build

`make` builds `libgscopebars89.a` using the vendored painter header. The runnable demo is available as `make demo` inside the complete bundle.
