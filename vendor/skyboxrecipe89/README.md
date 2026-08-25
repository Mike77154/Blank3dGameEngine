# SkyboxRecipe89

A C89/no-heap renderer-agnostic sky recipe composer. The host supplies a text
loader. Recipes are INI files and may include other recipe INIs recursively.

```ini
[recipe]
include=../components/cube_geometry.ini
include=../sources/night_cross.ini
include=../layers/atmosphere.ini

[skybox]
radius=96
```

Includes are resolved relative to the including file, have a fixed maximum
depth of 8, reject cycles, and are evaluated before the local file so local
settings always override subrecipes.

Supported source descriptions are six independent faces, one cube atlas/cross/
strip/grid image, a single fullscreen image, a single dome/panorama image, and
named six-face families using axis, Source/Valve, or word suffix conventions.
The library only produces requests and normalized UV rectangles; decoding,
asset routing and rendering belong to the host.
