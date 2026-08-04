# Design references

The implementation was informed by these public references:

- Khronos glTF 2.0 specification, mesh primitive topology, vertex attributes,
  winding, normals, tangents, and morph-target concepts:
  https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html
- Microsoft DirectX Tool Kit `GeometricPrimitive`, a practical catalog of common
  built-in shapes with normals and texture coordinates:
  https://github.com/microsoft/DirectXTK/wiki/GeometricPrimitive
- Unity primitive documentation, especially the capsule definition and UV
  behavior of sphere, cylinder, capsule, quad, and plane:
  https://docs.unity3d.com/2023.1/Documentation/Manual/PrimitiveObjects.html
- Unity mesh-data documentation for tangents, UV channels, and vertex colors:
  https://docs.unity3d.com/2022.1/Documentation/Manual/AnatomyofaMesh.html

`gprimitive89` is an independent C89 implementation and does not copy source code
from those projects.
