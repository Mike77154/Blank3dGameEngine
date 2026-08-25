# katana89 v1.1

Low-poly Japanese sword mesh generator for small game engines.

## Protocol

- ISO C89 / `-std=c89`
- fixed-point Q24.8 coordinates (`256 == 1 cm`)
- no `malloc`, `realloc`, `free`, heap ownership, `float`, or `double`
- caller-provided/static vertex and triangle buffers
- no renderer dependency
- deterministic output
- CC0

## Collection

- **33 curated presets**
- 6 blade sections: shinogi, hira, shobu, unokubi, kiriha, kata-shinogi
- 4 kissaki silhouettes: ko, chu, o, shobu
- 7 tsuba choices: maru, nadekaku, mokko, aoi, octagonal, cross, none
- 4 tsuka profiles: straight, rikko, ha-agari, tachi-like
- optional hamon ribbon, bohi cue, menuki, sukashi cue, dense wrap, and saya
- separate build ranges for blade, guard, handle, fittings, and saya

The historical names describe **visual inspiration**, not museum-perfect replicas.
The library deliberately exaggerates a few sub-millimetre details so the shape
survives low resolution, untextured rendering, and distant gameplay cameras.

## Historical proportion basis

The presets cluster around museum examples rather than fantasy sword ratios:

- Met katana: 70.6 cm cutting edge, 1.4 cm curvature.
- British Museum examples: 60.3–71.4 cm cutting edges and roughly 1.0–2.4 cm curvature.
- Tokyo National Museum/e-Museum example: 69.5 cm blade, 17.9 cm tang, 3.3 cm tip, 2.7 cm curvature.
- Met tsuba examples commonly measure about 2 7/8–3 1/4 inches (roughly 7.3–8.3 cm) high.
- Kambun-style blades are represented with shallow curvature and strong taper toward the point.

Sources are listed in `docs/BIBLIOGRAPHY.md`.

## Basic use

```c
static km89_vertex vertices[KM89_MAX_VERTICES];
static km89_triangle triangles[KM89_MAX_TRIANGLES];
km89_mesh mesh;
km89_build_info parts;

km89_mesh_init(&mesh,
               vertices, KM89_MAX_VERTICES,
               triangles, KM89_MAX_TRIANGLES);

km89_build_preset(0, &mesh, &parts);
```

`parts` returns contiguous triangle/vertex ranges for animation, selective draw,
material remapping, sockets, collision proxies, or exporting.

## Build

```sh
make
make generate
```

Tested with strict C89 flags. For the user's target toolchain:

```sh
gcc -std=c89 -O2 -Wall -Wextra -pedantic -Iinclude \
    src/katana89.c src/katana89_obj.c examples/generate_all.c \
    -o generate_all.exe
```

## Renderer/provider bridge

The library only emits indexed triangles and integer coordinates. Your engine can:

1. convert Q24.8 to its native fixed-point format;
2. map material IDs to its own textures/shaders;
3. transform each returned part range independently;
4. replace the OBJ helper entirely.

No camera, math3d, allocator, filesystem, texture, or GPU API is owned by katana89.


## v1.1 realism rebuild

- Historical kissaki proportions (ko/chu/o no longer needle-long).
- 25 nonuniform blade stations with denser topology near the point.
- Funbari-like base taper and separate ha/mune silhouette control.
- Eight-point shinogi cross-section and tapered thickness.
- Oblique yokote, wavy hamon and bohi termination before the kissaki.
- True open nakago-ana in every tsuba.
- Rounded-rectangular tsuka, crossed tsukamaki, tapered habaki, seppa and fitted collars.
- Rebuilt saya profile.
