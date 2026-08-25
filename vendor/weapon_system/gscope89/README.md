# gscope89 split bundle

The original monolithic `gscope89` responsibilities are now four independent,
renderer-neutral and collision-neutral C89 libraries.

```txt
gscope89_split/
├─ include/gscope89_common.h
├─ gtelescopiczoom89/
│  ├─ include/gtelescopiczoom89.h
│  └─ src/gtelescopiczoom89.c
├─ gsway89/
│  ├─ include/gsway89.h
│  └─ src/gsway89.c
├─ gsniperhud89/
│  ├─ include/gsniperhud89.h
│  └─ src/gsniperhud89.c
├─ gaimquery89/
│  ├─ include/gaimquery89.h
│  └─ src/gaimquery89.c
├─ examples/demo_split.c
├─ config/
└─ Makefile
```

## Modules

### gtelescopiczoom89

Owns only optical state:

- fixed-point/LUT FOV solver
- telescopic zoom factor
- smooth zoom enter/exit
- scoped sensitivity interpolation

It does not know about sway, rendering, weapons or collision.

### gsway89

Owns only procedural aim displacement:

- independent yaw/pitch drift
- slow breathing wave
- heartbeat/pulse component
- hold-breath transition
- finite breath reserve
- exhaustion and recovery
- movement and stress multipliers

The output is `yaw_out_deg_x1000` and `pitch_out_deg_x1000`. The host engine
applies those offsets through its own fixed-point camera math.

### gsniperhud89

Owns only semantic HUD/overlay commands:

- circular mask/blackout
- configurable reticle gap and arms
- center dot, range ticks and vignette
- optic telemetry
- breath reserve/exhaustion telemetry
- target identity and range telemetry
- world impact-position telemetry

It never draws directly. The engine receives `gsh89_cmd` callbacks.

### gaimquery89

Owns only center-ray query state:

- center ray from camera position/forward
- maximum distance
- target ID and target kind
- target part/material/flags
- hit distance
- impact position and normal
- miss endpoint

The collision world remains owned by the engine through a callback.

## Dependency shape

```txt
                 gscope89_common.h
             /          |          \
            v           v           v
 telescopiczoom89    gsway89    gaimquery89
            \           |           /
             \          v          /
                 gsniperhud89
            (only receives copied telemetry)
```

There are no runtime dependencies among the four libraries. They only share
small ABI types from `gscope89_common.h`. The HUD receives snapshots copied by
the host, so no circular dependency exists.

## Build

```sh
make
./demo_split
```

Strict check:

```sh
gcc -std=c89 -pedantic -Wall -Wextra -Iinclude \
  gtelescopiczoom89/src/gtelescopiczoom89.c \
  gsway89/src/gsway89.c \
  gsniperhud89/src/gsniperhud89.c \
  gaimquery89/src/gaimquery89.c \
  examples/demo_split.c -o demo_split
```

## Engine integration order

```txt
1. gtz89_update()        -> camera FOV + input sensitivity
2. gsw89_update()        -> yaw/pitch camera offsets
3. engine updates basis  -> final camera.forward
4. gaq89_query_center()  -> target/range/impact
5. copy snapshots        -> gsh89_telemetry
6. gsh89_emit_*()        -> renderer/HUD callbacks
```

## Restrictions retained

- C89
- no malloc/realloc/free
- no owned heap
- no float/double
- integer/fixed-point runtime
- renderer agnostic
- collision agnostic
- static lookup tables only

## v0.2: 192 reticles moved to INI

The vector reticle catalog is now data-driven. See `config/reticles/`: `active.ini` selects a name, `catalog.ini` maps names to formal preset files, and `presets/` contains all 192 migrated reticles. `modules/gscopepresets89/src/gscopepresets89.c` no longer contains compiled reticle geometry.

## v0.3: composable reticles + provider-aware bundle

Every one of the 192 formal reticle INIs is now an assembly: preset metadata stays in `config/reticles/presets/`, while vector geometry is included from component/fragment INIs. Repeated full geometry was deduplicated into 21 shared fragments used by 54 presets. The runtime parity hash is unchanged.

`gscopeprovider89` and `gscopebundle89` add optional named providers. A master recipe can route `recipe`, `preset`, `vector`, `primitive`, `raster`, `bars`, `paint`, `hud`, `telemetry`, `asset` and `zoom` independently to either the bundled implementation or an external host provider. `auto:<name>` tries the provider and falls back internally; `external:<name>` requires the provider.

See `PROVIDER_ARCHITECTURE.md`, `RECIPE_ARCHITECTURE.md`, `config/providers/`, and `examples/provider_host_example.c`.

## v0.4: frozen golden master + INI animation

The original pre-INI 192-reticle source is frozen as a test-only golden master.
`make golden_master` regenerates both legacy and INI command streams and 320x320
fixed-point raster frames, then compares them byte-for-byte. All 192 currently
pass with zero command or pixel differences. See `GOLDEN_MASTER.md`.

`gscopeanim89` adds recursive INI animation sets. The HUD master selects an
animation set through the same catalog/use mechanism as scope, zoom and sway.
The bundled golden-safe default provides `fire`, `aim_enter`, `aim_exit` and
`damage` events; `fire` scales the reticle out and settles it back in 120 ms.
Animation is also a provider domain, with internal/auto/external routing.
See `ANIMATION_ARCHITECTURE.md` and `config/animations/`.
