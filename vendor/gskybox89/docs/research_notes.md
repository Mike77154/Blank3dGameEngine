# Research notes

Design references checked before building:

- Khronos OpenGL Wiki: cubemap textures are six square 2D images, accessed with a 3D direction vector.
- Microsoft Direct3D 9 docs: cubic environment maps have six faces, each face covering a 90 degree horizontal and vertical field of view.
- LearnOpenGL cubemap/skybox guidance: a cube centered at the origin can use local vertex positions as direction vectors for cubemap sampling.
- Khronos community discussion: drawing sky after scene depth can take advantage of depth testing/early rejection on covered pixels.

Implementation decision:

```txt
GSKYBOX89_LAYER_SCREEN: fullscreen triangle fallback/background hook
GSKYBOX89_LAYER_CUBE6 : classic six-face skybox, 12 triangles, default 8-corner indexed fast path
GSKYBOX89_LAYER_DOME  : tiny fixed-table hemisphere overlay for horizon/atmosphere tint
```


## Cube improvement v1.1

The cube path now matches the low-cost mesh model more directly:

```txt
8 shared physical cube corners
6 faces
2 triangles per face
12 triangles total
0 heap
```

This keeps the GPU triangle count unchanged from the ideal minimum for a textured cube while reducing CPU-side fixed-point rotation work compared with regenerating each face independently.
